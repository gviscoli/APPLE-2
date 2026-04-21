# Apple IIe → LLM: Architettura e Sfide Tecniche

**Un chatbot su CPU 6502 @ 1 MHz che dialoga con Amazon Nova via AWS Bedrock**

> *"Quarant'anni separano la macchina dalla mente. Un proxy li unisce."*

---

## Indice

1. [Panoramica del sistema](#1-panoramica-del-sistema)
2. [Stack tecnologico e vincoli hardware](#2-stack-tecnologico-e-vincoli-hardware)
3. [Architettura a livelli](#3-architettura-a-livelli)
4. [Memory map Apple IIe](#4-memory-map-apple-iie)
5. [Strato 1 — chatbot.c: il client Apple IIe](#5-strato-1--chatbotc-il-client-apple-iie)
6. [Strato 2 — rest_lib.c: il layer HTTP/TCP](#6-strato-2--rest_libc-il-layer-httptcp)
7. [Strato 3 — llm_proxy.py: il proxy Flask](#7-strato-3--llm_proxypy-il-proxy-flask)
8. [Strato 4 — Amazon Bedrock e il modello Nova](#8-strato-4--amazon-bedrock-e-il-modello-nova)
9. [Il flusso dati end-to-end](#9-il-flusso-dati-end-to-end)
10. [Sfide tecniche principali](#10-sfide-tecniche-principali)
11. [Tabella riepilogativa dei vincoli e soluzioni](#11-tabella-riepilogativa-dei-vincoli-e-soluzioni)
12. [Possibili evoluzioni](#12-possibili-evoluzioni)

---

## 1. Panoramica del sistema

Questo progetto mette in comunicazione un **Apple IIe del 1983** — dotato di CPU MOS 6502 a 1 MHz e 64 KB di RAM — con i moderni **Large Language Model** ospitati su cloud AWS. Il risultato è un chatbot funzionante che, dal punto di vista dell'utente, appare come una semplice applicazione testuale sullo storico schermo verde da 40 colonne.

Il salto temporale e tecnologico è di quarant'anni: nel 1983 Internet era ancora ARPANET, i microcomputer non conoscevano TCP/IP, e l'intelligenza artificiale generativa era fantascienza. Oggi, grazie a una scheda di rete **Uthernet II** (chip W5100) e a un proxy intermedio in Python, l'Apple IIe riesce a porre domande a un LLM e ricevere risposte nel giro di pochi secondi.

### Diagramma architetturale

![End-to-End Architecture](diag_01_architecture.png)

Il sistema si articola in quattro zone distinte collegate in sequenza:

- **Apple IIe** (1983): il client hardware, compilato con cc65 per target `apple2`, usa la libreria ip65 per il networking via Uthernet II.
- **LAN locale**: la comunicazione tra Apple IIe e proxy avviene in HTTP/1.0 plain-text sulla porta 8080, senza crittografia.
- **Flask Proxy** (`llm_proxy.py`): il traduttore tra i due mondi, responsabile di TLS, auth AWS, sanificazione Unicode e troncatura delle risposte.
- **AWS Bedrock** (Amazon Nova): il modello LLM in cloud, raggiunto via HTTPS con autenticazione AWS Signature V4.

---

## 2. Stack tecnologico e vincoli hardware

### Hardware Apple IIe

| Componente | Specifica |
|---|---|
| CPU | MOS Technology 6502, 8-bit, 1 MHz |
| RAM | 64 KB (di cui ~40 KB disponibili al programma) |
| Stack hardware 6502 | **256 byte fissi** in pagina 1 (`$0100–$01FF`) |
| Schermo testo | 40 × 24 colonne, solo ASCII 7-bit |
| Scheda di rete | Uthernet II (slot 1–7), chip W5100 |
| Compilatore | cc65 (cross-compiler C per 6502), target `apple2` |
| Libreria di rete | ip65 — stack TCP/IP scritto in Assembly 6502 |

### Vincoli fondamentali

Il 6502 è un processore a **8 bit** con bus indirizzi a 16 bit. Ogni ottimizzazione nel codice non è eleganza stilistica, ma necessita assoluta:

- **Zero Page** (`$0000–$00FF`): area di memoria ad accesso veloce, condivisa tra ip65 e cc65 per variabili critiche e puntatori.
- **Stack hardware** di soli 256 byte: ogni chiamata a funzione consuma 2 byte (indirizzo di ritorno). Catene profonde causano stack overflow fisico e crash silenziosi.
- **Assenza di FPU, MMU, DMA**: tutto viene gestito dalla CPU, inclusa la copia dei pacchetti di rete dal chip W5100.
- **RAM effettiva**: 64 KB totali includono ROM firmware, pagine grafiche e video. Il programma dispone di circa 32–40 KB per codice, dati e heap.

---

## 3. Architettura a livelli

### Diagramma dei layer con sfide

![Layer Stack](diag_04_layers.png)

Il sistema è composto da cinque strati software distinti — quattro sul client Apple IIe, uno in cloud — ciascuno con responsabilità e sfide tecniche proprie.

```
┌────────────────────────────────────────────────────────────┐
│  AWS BEDROCK (cloud us-east-1) — Amazon Nova               │
│  API HTTPS + SigV4 · invoke_model sincrono · maxTokens 150 │
├────────────────────────────────────────────────────────────┤
│  LAYER 4 — llm_proxy.py  (Python / Flask)                  │
│  HTTP/1.0 ↔ HTTPS · clean_for_apple2() · troncatura        │
├────────────────────────────────────────────────────────────┤
│  LAYER 3 — chatbot.c  (C / cc65)                           │
│  UI 40 col · read_line · build_json · print_wrapped        │
├────────────────────────────────────────────────────────────┤
│  LAYER 2 — rest_lib.c  (C / cc65)                          │
│  HTTP/1.0 builder · TCP polling loop · header parser       │
├────────────────────────────────────────────────────────────┤
│  LAYER 1 — ip65  (Assembly 6502)                           │
│  dhcp_init · tcp_connect · tcp_send · ip65_process         │
├────────────────────────────────────────────────────────────┤
│  HARDWARE — Apple IIe (1983)                               │
│  6502 @ 1 MHz · 64 KB RAM · Uthernet II (W5100)            │
└────────────────────────────────────────────────────────────┘
```

---

## 4. Memory map Apple IIe

### Diagramma della mappa di memoria

![Memory Map](diag_02_memory.png)

La gestione della memoria sull'Apple IIe è uno dei vincoli più stringenti dell'intero progetto. Con 64 KB totali condivisi tra ROM, firmware, display buffer, stack hardware, codice e dati statici, ogni decisione di allocazione è critica.

### Zone chiave per questo progetto

**Zero Page (`$0000–$00FF`)** — 256 byte
Usata contemporaneamente da ip65 (variabili di stato TCP, puntatori ai buffer) e da cc65 (software stack pointer, variabili temporanee del runtime C). La coesistenza richiede che le due librerie non si sovrappongano nelle loro assegnazioni di Zero Page.

**Stack hardware 6502 (`$0100–$01FF`)** — 256 byte fissi
Il vincolo più critico. Il 6502 non ha meccanismo di stack overflow: se il puntatore di stack supera `$0100`, scrive in Zero Page sovrascrivendo variabili critiche senza alcun segnale di errore. La catena massima di chiamate nel progetto è di 5 livelli (10 byte), ampiamente sicura.

**Buffer statici (`$B400–$BEFF`)** — ~2.8 KB
`response_buf` (2 KB), `request_buf` (512 B) e `input_buf`/`json_buf` (~290 B) sono dichiarati `static` e allocati dal linker cc65 a compile-time. Non usano `malloc()` deliberatamente: su un sistema senza MMU, un `malloc` fallito non è gestibile in modo affidabile.

**Codice programma (`$8000–$AFFF`)** — ~12 KB
Il binario compilato di `chatbot.c` + `rest_lib.c`. cc65 con ottimizzazioni `-O` produce codice 6502 compatto; senza ottimizzazioni lo stesso sorgente richiederebbe il 30–40% di spazio in piu'.

**Software stack cc65 (`$0800–$7FFF`)** — zona libera
cc65 usa un secondo stack "software" per le variabili locali delle funzioni C, gestito tramite un puntatore in Zero Page. Questo stack non ha il limite fisico di 256 byte del hardware stack, ma consuma memoria heap.

---

## 5. Strato 1 — chatbot.c: il client Apple IIe

### Responsabilita'

`chatbot.c` e' il **punto di ingresso** e l'unico modulo che interagisce direttamente con l'utente. Include:

- Il banner ASCII art mostrato una sola volta all'avvio (`show_ascii_banner`)
- La schermata di benvenuto (`show_welcome`)
- Il loop principale di chat: input → build JSON → invio → ricezione → display
- La costruzione del payload JSON (`build_json`)
- Il word wrap a 40 colonne (`print_wrapped`)
- La lettura input con gestione backspace e strip del bit 7 (`read_line`)

### Il banner ASCII art

Il banner viene mostrato **una sola volta** prima di `show_welcome()`, con `cgetc()` in attesa di un keypress prima di procedere. Ogni riga e' calcolata per rispettare esattamente le 40 colonne disponibili sul display Apple IIe:

```
*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
*                                     *
*  =================================  *
*  |      APPLE ][ - CPU: 6502     |  *
*  |                               |  *
*  | (powered by Viscoli Giuseppe) |  *
*  =================================  *
*                                     *
*     ~~ LA MACCHINA DEL TEMPO ~~     *
*           (PCH.CHATBOT.BIN)         *
*                                     *
* [1983] --> Internet --> LLM [2026]  *
*                                     *
*   Una vecchia tecnologia che        *
*   dialoga con l'AI del futuro!      *
*                                     *
*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
```

La freccia `[1983] --> Internet --> LLM [2026]` sintetizza in una riga l'intera architettura del progetto.

### Gestione dell'input (read_line)

```c
c = cgetc() & 0x7F;   /* strip high bit — firmware Apple IIe imposta bit 7 */
```

Il firmware Apple IIe imposta tradizionalmente il **bit 7** dei caratteri letti da tastiera (es. `'A'` = `0xC1` invece di `0x41`). Il mascheramento esplicito con `0x7F` rende il codice robusto su tutte le varianti del firmware e su emulatori che potrebbero comportarsi diversamente.

Il backspace viene gestito manualmente con la sequenza `BS / SPACE / BS` per cancellare visivamente il carattere a schermo senza richiedere supporto dal firmware.

### Costruzione del JSON (build_json)

Apple IIe non ha librerie JSON. La funzione costruisce manualmente la stringa `{"prompt":"..."}` con escaping esplicito di `"` e `\`:

```c
if (prompt[pi] == '"' || prompt[pi] == '\\') {
    out[oi++] = '\\';
}
out[oi++] = prompt[pi++];
```

Il parametro `out_size` e' di tipo `uint8_t` (massimo 255). Con `JSON_BUF_SIZE = 160` il limite e' rispettato, ma se si aumentasse il buffer oltre 255 byte il tipo andrebbe cambiato in `uint16_t`.

### Word wrap (print_wrapped)

Il display Apple IIe in modalita' testo standard va a capo troncando il carattere alla colonna 40 senza rispettare i confini di parola. `print_wrapped()` scansiona la stringa parola per parola e inserisce `\n` prima che una parola superi il margine:

```
Input  (68 ch): "L'intelligenza artificiale e' una tecnologia del futuro ormai presente."
Output (40 col):
  L'intelligenza artificiale e' una
  tecnologia del futuro ormai
  presente.
```

---

## 6. Strato 2 — rest_lib.c: il layer HTTP/TCP

### Responsabilita'

`rest_lib.c` e' il **layer di comunicazione**. Non interagisce con l'utente: riceve istruzioni da `chatbot.c` e restituisce dati grezzi. Gestisce:

- La connessione TCP via ip65
- La costruzione delle richieste HTTP/1.0 (GET e POST)
- Il polling bloccante per la ricezione dei dati
- Il parsing minimale dell'header HTTP

### Il modello di I/O: polling, non interrupt

Su un sistema operativo moderno l'I/O di rete e' gestito tramite interrupt e scheduler. Su Apple IIe con ip65 il modello e' completamente diverso:

```c
while (s_recv_remaining > 0 && poll_count < POLL_MAX_ITERATIONS) {
    ip65_process();   /* legge i pacchetti in arrivo, chiama tcp_recv_cb */
    poll_count++;
    if (ip65_error != 0) break;
}
```

`ip65_process()` e' una funzione Assembly che legge i registri del chip W5100 sulla Uthernet II e, se ci sono dati disponibili, invoca la callback `tcp_recv_cb`. Il loop gira in **busy-wait**: la CPU non fa nulla di utile nel frattempo, ma su un sistema senza sistema operativo non esiste alternativa. Con `POLL_MAX_ITERATIONS = 10000` e ~1–5 ms per iterazione, il timeout massimo e' tra 10 e 50 secondi.

### La callback TCP e __fastcall__

```c
static void __fastcall__ tcp_recv_cb(const uint8_t *data, int len) { ... }
```

Il modificatore `__fastcall__` di cc65 usa una convenzione di chiamata ottimizzata: l'**ultimo parametro** viene passato nei registri A (byte basso) e X (byte alto) invece che nello stack, risparmiando cicli di push/pop. Questa firma deve corrispondere **esattamente** a quella attesa da ip65 in Assembly. Un disallineamento non produce errori di compilazione, ma corruzione silenziosa dei dati a runtime.

### Perche' HTTP/1.0 e non HTTP/1.1?

HTTP/1.1 introduce `keep-alive` e `chunked transfer encoding`, che richiederebbero parsing molto piu' complesso. HTTP/1.0 e' radicalmente piu' semplice:

- Il server chiude la connessione dopo aver inviato la risposta
- ip65 rileva la chiusura TCP e imposta `ip65_error = 0x84`
- Il polling loop esce, e i dati ricevuti sono la risposta completa

Questo comportamento — tecnicamente un "errore" — e' in realta' il **meccanismo di fine-risposta** scelto deliberatamente.

### Parsing dell'header HTTP

Volutamente minimale per risparmiare RAM e cicli:

```c
sscanf((char *)out_buf, "HTTP/1.%*d %hu", &http_status);   /* estrae status code */
body_start = strstr((char *)out_buf, "\r\n\r\n");           /* trova fine header  */
body_start += 4;                                            /* salta il separatore */
```

Nessun parsing dei singoli header. La dimensione del body e' calcolata sottraendo la posizione del body dalla lunghezza totale ricevuta.

---

## 7. Strato 3 — llm_proxy.py: il proxy Flask

### Ruolo architetturale

Il proxy e' il **traduttore di protocolli** tra i due mondi. Senza di esso, l'Apple IIe non potrebbe comunicare con AWS Bedrock per quattro ragioni:

1. **TLS/HTTPS**: implementare TLS su 6502 e' impossibile per mancanza di RAM e cicli.
2. **JSON complesso**: l'API Bedrock usa strutture JSON annidate con header di autenticazione AWS Signature V4.
3. **Unicode**: le risposte Bedrock contengono caratteri non-ASCII incompatibili con il display Apple IIe.
4. **Credenziali IAM**: non devono mai essere esposte sul client.

### Configurazione tramite .env

```python
INFERENCE_MODE = os.getenv("INFERENCE_MODE", "cloud").lower()  # "cloud" | "local"
MODEL_ID = LLM_MODEL_CLOUD if INFERENCE_MODE == "cloud" else LLM_MODEL_LOCAL
```

La variabile `INFERENCE_MODE` permette di commutare tra **AWS Bedrock** e un modello locale (es. Ollama) senza modificare il codice client C. Utile per sviluppo e testing senza costi API.

### La funzione clean_for_apple2

```python
def clean_for_apple2(text: str) -> str:
    replacements = {
        'a': "a'", 'e': "e'", 'i': "i'", 'o': "o'", 'u': "u'",
        # ... varianti maiuscole e minuscole
    }
    for char, replacement in replacements.items():
        text = text.replace(char, replacement)
    return "".join(c for c in text if ord(c) < 128)
```

Opera in due passaggi: prima sostituisce le lettere accentate italiane con equivalenti ASCII (`e'` invece di `e` con accento, conservando la leggibilita'), poi rimuove tutto cio' che rimane con `ord(c) >= 128` (emoji, simboli, caratteri CJK).

### Il system prompt come vincolo hardware

```python
system_prompt = (
    "Sei un assistente per un computer Apple IIe del 1983. "
    "Rispondi in ITALIANO. Sii estremamente sintetico. "
    "La tua risposta NON deve superare i 240 caratteri. "
    "Non usare markdown (niente grassetto o tabelle)."
)
```

I 240 caratteri e il divieto di markdown non sono preferenze stilistiche: sono **vincoli dettati dall'hardware**. Il divieto di markdown evita che `**`, `#` o `|` inquinino il testo visualizzato. La richiesta di sintesi e' necessaria perche' ogni carattere aggiuntivo e' RAM occupata su un sistema con 64 KB totali.

### Il doppio sistema di troncatura

La risposta viene limitata a due livelli indipendenti:

**Livello 1 — LLM** (`maxTokens: 150`): la generazione viene fermata dopo 150 token (~100–200 caratteri in italiano).

**Livello 2 — Python** (post-processing): taglio di sicurezza che non spezza le parole a meta':

```python
if len(text) > MAX_RESPONSE_CHARS:
    truncated = text[:MAX_RESPONSE_CHARS]
    last_space = truncated.rfind(' ')
    text = truncated[:last_space] if last_space != -1 else truncated
```

### Dual-endpoint GET e POST

Il proxy espone `/api/data` su entrambi i metodi:

- **GET** con `?prompt=...`: usato da `chatbot.c` per il test iniziale di connettivita', senza consumare crediti API Bedrock se il prompt e' assente.
- **POST** con body `{"prompt":"..."}`: usato per le conversazioni reali.

---

## 8. Strato 4 — Amazon Bedrock e il modello Nova

**Amazon Nova** e' la famiglia di modelli fondazionali di Amazon disponibile tramite AWS Bedrock. Il proxy usa `invoke_model` in modalita' sincrona (senza streaming): la risposta arriva in un unico blocco JSON dopo che il modello ha terminato la generazione.

### Perche' non lo streaming?

Le API Bedrock supportano lo streaming token per token. Sarebbe teoricamente piu' elegante, ma richiederebbe:

1. Una connessione TCP persistente dal proxy all'Apple IIe durante tutta la generazione
2. Logica di frammentazione e reassembly dei token su ip65
3. Gestione di timeout molto piu' complessa

Con `invoke_model` sincrono la risposta arriva in blocco unico: piu' semplice per entrambi gli estremi della catena.

### Parametri di inferenza

```python
"inferenceConfig": {
    "maxTokens": 150,
    "temperature": 0.4
}
```

La `temperature` a `0.4` garantisce risposte deterministiche e concise, riducendo il rischio che il modello produca testo prolisso o creativo che sfori i limiti di carattere.

---

## 9. Il flusso dati end-to-end

### Sequence diagram

![Sequence Diagram](diag_03_sequence.png)

Il diagramma mostra i 9 passaggi principali di un singolo turno di conversazione, dall'input dell'utente alla visualizzazione della risposta sul display Apple IIe.

### Traccia testuale

```
UTENTE digita: "chi ha inventato il computer?"
       │
       ▼ [chatbot.c] read_line() & 0x7F
  input_buf = "chi ha inventato il computer?"
       │
       ▼ [chatbot.c] build_json()
  json_buf = {"prompt":"chi ha inventato il computer?"}
       │
       ▼ [rest_lib.c] do_post("/api/data", json_buf, ...)
  Costruisce HTTP/1.0 request:
      POST /api/data HTTP/1.0\r\n
      Host: 192.100.1.211\r\n
      Content-Type: application/json\r\n
      Content-Length: 45\r\n
      Connection: close\r\n
      \r\n
      {"prompt":"chi ha inventato il computer?"}
       │
       ▼ [ip65] tcp_connect() + tcp_send() → LAN
       │
       ▼ [llm_proxy.py] POST /api/data
  parse JSON → call_nova(prompt)
       │
       ▼ [boto3] bedrock.invoke_model() → HTTPS → AWS
       │
       ▼ [Amazon Nova] genera risposta
  → "Charles Babbage progetto' la Macchina Analitica
     nel 1837. Alan Turing defini' le basi teoriche.
     Il primo computer elettronico fu l'ENIAC (1945)."
       │
       ▼ [llm_proxy.py] clean_for_apple2() + troncatura + .upper()
  → "CHARLES BABBAGE PROGETTO' LA MACCHINA ANALITICA..."
       │
       ▼ [ip65] tcp_recv_cb() — polling loop — ip65_error=0x84 (EOF)
       │
       ▼ [rest_lib.c] parsing header → http_response_body
       │
       ▼ [chatbot.c] print_wrapped(40 col)

SCHERMO APPLE IIe:
 ──────────────────────────────────────
 BOT:
 ──────────────────────────────────────
 CHARLES BABBAGE PROGETTO' LA MACCHINA
 ANALITICA NEL 1837. ALAN TURING
 DEFINI' LE BASI TEORICHE. IL PRIMO
 COMPUTER ELETTRONICO FU L'ENIAC.
 ──────────────────────────────────────
```

---

## 10. Sfide tecniche principali

### 10.1 — Stack overflow a 256 byte

Il 6502 usa uno stack hardware fisso di soli 256 byte in pagina 1. La catena massima del progetto:

```
main() → send_message() → do_post() → tcp_do_request() → ip65_process()
  2B           2B               2B              2B               2B  = 10 byte totali
```

10 byte su 256 disponibili e' sicuro. Il rischio diventa reale con ricorsione o callback profonde. Le variabili locali C usano il software stack di cc65, non quello hardware.

### 10.2 — Compatibilita' ASCII a 7 bit

Il display hardware Apple IIe legge solo 7 bit per carattere. La soluzione e' duplice:

- **Lato proxy**: `clean_for_apple2()` normalizza l'output prima della trasmissione
- **Lato client**: `cgetc() & 0x7F` normalizza l'input dalla tastiera

### 10.3 — Rilevamento della fine risposta HTTP/1.0

Con HTTP/1.0 il server segnala la fine del body chiudendo la connessione TCP. ip65 interpreta questa chiusura come `ip65_error = 0x84`, che fa uscire il polling loop. Questo e' il comportamento **corretto e atteso**, ma il codice deve non trattare `0x84` come un errore fatale dopo aver ricevuto dati validi.

### 10.4 — Firma __fastcall__ e convenzioni di chiamata

La callback `tcp_recv_cb` con `__fastcall__` deve corrispondere **esattamente** a cio' che ip65 si aspetta in Assembly. Un disallineamento non produce errori di compilazione, ma corruzione silenziosa dei dati a runtime — uno dei bug piu' difficili da diagnosticare su un sistema senza debugger.

### 10.5 — Assenza di memoria di conversazione

L'implementazione e' **stateless**: ogni messaggio e' un POST isolato senza history. Il modello LLM non ha contesto dei turni precedenti. Per implementare la memoria si dovrebbe accumulare la history nel client e includerla nel payload JSON, ma questo richiederebbe buffer molto piu' grandi e una logica di gestione del context window impraticabile con la RAM disponibile.

### 10.6 — Latenza percepita e feedback visivo

Con `POLL_MAX_ITERATIONS = 10000`, il sistema puo' attendere fino a ~50 secondi senza dare feedback visivo. Una miglioria sarebbe stampare un carattere `.` ogni N iterazioni del loop di polling — ma richiede di esporre il loop da `rest_lib.c` o ristrutturare il codice.

### 10.7 — Sicurezza delle credenziali AWS

Le credenziali AWS non compaiono mai nel codice C e non vengono trasmesse all'Apple IIe. Vivono esclusivamente nel `.env` sul server proxy o in `~/.aws/credentials`. Il proxy funge da **security boundary**: l'Apple IIe non conosce il modello usato, non puo' aggirare il rate limiting, e non ha accesso diretto ad AWS.

---

## 11. Tabella riepilogativa dei vincoli e soluzioni

| Vincolo | Origine | Soluzione adottata |
|---|---|---|
| Stack hardware 256 byte | 6502 fisico | Variabili `static`, no ricorsione, call chain max 5 livelli |
| RAM totale ~40 KB disponibili | 6502 / Apple IIe | Buffer statici, no `malloc()`, dimensioni conservative |
| Solo ASCII 7-bit a schermo | Hardware display | `clean_for_apple2()` nel proxy + `& 0x7F` nel client |
| No TLS / HTTPS | ip65 / risorse CPU | Proxy intermedio che termina TLS lato server |
| No librerie JSON | Ambiente bare-metal | `build_json()` manuale con escaping esplicito |
| Modello di I/O polling | ip65 / no OS | Loop `ip65_process()` con counter e exit su `ip65_error` |
| Risposta potenzialmente lunga | LLM by nature | `maxTokens: 150` + troncatura post-processing nel proxy |
| Fine risposta HTTP ambigua | HTTP/1.0 + ip65 | Chiusura connessione = `ip65_error 0x84` = EOF |
| Caratteri accentati italiani | UTF-8 vs ASCII | Tabella di sostituzione `accento → lettera + apostrofo` |
| Credenziali AWS esposte | Sicurezza | Proxy come boundary — client non vede mai le credenziali |
| Schermo 40 colonne | Hardware fisso | `print_wrapped()` con word-boundary detection |
| Nessuna memoria conversazione | RAM insufficiente | Architettura stateless, ogni turno e' indipendente |
| High bit tastiera Apple IIe | Firmware variabile | `cgetc() & 0x7F` esplicito in `read_line()` |
| TLS lato client impossibile | CPU 1 MHz / RAM | Proxy Flask con boto3 gestisce tutta la catena HTTPS |

---

## 12. Possibili evoluzioni

**Indicatore di attesa visivo**
Stampare un carattere progressivo (`.` o una rotella `|/-\`) ogni N iterazioni del loop di polling, per segnalare che il sistema e' in attesa e non bloccato.

**Modalita' Ollama (local inference)**
La variabile `INFERENCE_MODE=local` e' gia' predisposta. Connettere il proxy a un modello locale (Llama 3, Mistral, Granite) eliminera' la dipendenza da AWS e i costi di API, mantenendo identica l'interfaccia verso l'Apple IIe.

**History conversazionale limitata**
Mantenere gli ultimi N turni in un buffer circolare nel client e includerli nel payload JSON, pur rispettando il limite di `request_buf = 512 byte`. Con messaggi brevi si potrebbero mantenere 2–3 turni di contesto.

**Porta seriale come alternativa a TCP**
Per ambienti senza Uthernet II, il proxy potrebbe essere raggiunto tramite porta seriale RS-232. Il layer `rest_lib.c` andrebbe adattato, ma `chatbot.c` rimarrebbe invariato.

**Streaming con frammentazione**
Implementare la ricezione streaming token-per-token richiederebbe una connessione TCP persistente e logica di reassembly su ip65, ma permetterebbe di visualizzare la risposta in tempo reale sul display Apple IIe.

---

*Progetto di Viscoli Giuseppe — Apple IIe (1983) + Amazon Nova (2024–2026)*
*`PCH.CHATBOT.BIN` — compilato con cc65 per target apple2*

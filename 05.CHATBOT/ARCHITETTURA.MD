## Introduzione

Questo progetto mette in comunicazione un **Apple IIe del 1983** — dotato di CPU MOS 6502 a 1 MHz e 64 KB di RAM — con i moderni **Large Language Model** ospitati su cloud AWS / Azure ed eventualmente anche localmente attraverso l'utilizzo di OLLAMA. Il risultato è un chatbot funzionante che, dal punto di vista dell'utente, appare come una semplice applicazione testuale sullo storico schermo verde da 40 colonne di un Apple ][.

Il salto temporale e tecnologico è di quarant'anni: nel 1983 Internet era ancora ARPANET, i microcomputer non conoscevano TCP/IP, e l'intelligenza artificiale generativa era fantascienza. Oggi, grazie a una scheda di rete **Uthernet II** (chip W5100) e a un proxy intermedio scritto in Python, l'Apple ][ riesce a porre domande a un LLM e ricevere risposte nel giro di pochi secondi.

[👉 [Guarda il video per approfondimenti]](https://www.youtube.com/watch?v=nImY2d2jits)

Da dove siamo partiti:

<img src="/00.ASSETS/images/05.Chatbot/Chat/IMG_4030.JPEG" width="400" alt="Scermata iniziale"/>

Dove siamo arrivati (🙃)

<img src="/00.ASSETS/images/05.Chatbot/Chat/IMG_6528.JPEG" width="400" alt="Scermata iniziale"/>

<img src="/00.ASSETS/images/05.Chatbot/Chat/IMG_6526.JPEG" width="400" lt="Esempio"/>

---

## Indice

1. [Panoramica](#panoramica)
2. [Diagrammi](#diagrammi)
3. [Confronto tra le due versioni](#confronto-tra-le-due-versioni)
4. [Stack tecnologico](#stack-tecnologico)
5. [Struttura dei file](#struttura-dei-file)
6. [Lato client — Versione Bufferizzata](#lato-client--versione-bufferizzata-05chatbot)
   - [chatbot.c — UI e logica conversazione](#chatbotc--ui-e-logica-conversazione)
   - [rest_lib.c — Layer HTTP/TCP (bufferizzato)](#rest_libc--layer-httptcp-bufferizzato)
7. [Lato client — Versione Streaming](#lato-client--versione-streaming-05chatbot)
   - [chatbot_streaming.c — UI streaming real-time](#chatbot_streamingc--ui-streaming-real-time)
   - [rest_lib_streaming.c — Chunked decoder](#rest_lib_streamingc--layer-httptcp-con-chunked-decoder)
8. [Protocollo di comunicazione](#protocollo-di-comunicazione)
   - [Versione Bufferizzata — HTTP/1.0](#versione-bufferizzata--http10)
   - [Versione Streaming — HTTP/1.1 Chunked](#versione-streaming--http11-chunked)
9. [Lato server — Versione Bufferizzata](#lato-server--versione-bufferizzata-100rest-proxy)
   - [llm_proxy.py — Gateway Flask (porta 8080)](#llm_proxypy--gateway-flask-porta-8080)
   - [Endpoint esposti](#endpoint-esposti)
   - [Pipeline di elaborazione](#pipeline-di-elaborazione-comune-a-tutti-i-backend)
   - [clean_for_apple2(text)](#clean_for_apple2text)
10. [Lato server — Versione Streaming](#lato-server--versione-streaming-100rest-proxy)
    - [llm_proxy_streaming.py — Gateway Flask Streaming (porta 8081)](#llm_proxy_streamingpy--gateway-flask-streaming-porta-8081)
    - [Endpoint streaming](#endpoint-streaming)
    - [Generator per ogni backend](#generator-per-ogni-backend)
11. [ip65 — Network stack Assembly 6502](#ip65--network-stack-assembly-6502)
12. [Flusso dati end-to-end](#flusso-dati-end-to-end)
13. [Vincoli hardware e soluzioni di design](#vincoli-hardware-e-soluzioni-di-design)
14. [Limiti di dimensione buffer](#limiti-di-dimensione-buffer)
15. [Sequenza di avvio](#sequenza-di-avvio)

---



# Architettura del Sistema PCH.CHATBOT

**Un chatbot su CPU 6502 @ 1 MHz che dialoga con Amazon Nova via AWS Bedrock**

Un Computer Apple IIe del 1983 (MOS 6502 @ 1 MHz, 64 KB RAM) che dialoga con un LLM Cloud (Amazon Nova, Anthropic Claude, OpenAI o Ollama locale) attraverso un proxy Python intermedio.

Il sistema esiste in **due varianti** con architetture distinte per il protocollo di comunicazione e l'esperienza utente.

> *"Quarant'anni separano la macchina dalla mente. Un proxy li unisce."*

---

## Panoramica

```
┌─────────────────────┐    LAN HTTP/1.0 o HTTP/1.1 chunked    ┌──────────────────────┐       HTTPS        ┌──────────────────────┐
│   Apple IIe (6502)  │ ──────────────────────────────────►   │  Proxy Python/Flask  │ ─────────────────► │   LLM Backend        │
│   pch_chatbot.bin   │ ◄──────────────────────────────────   │  :8080 / :8081       │ ◄───────────────── │  Nova · Claude · LLM │
│   Uthernet II       │     plain-text, no TLS                │  192.100.1.211       │   API REST/HTTPS   │                      │
└─────────────────────┘                                       └──────────────────────┘                    └──────────────────────┘
  cartella 05.CHATBOT                                          cartella 100.REST-PROXY
```

**Motivazione architetturale:** il chip 6502 non può eseguire TLS né HTTP/1.1 chunked encoding. Il proxy risolve entrambi i problemi, isola le credenziali AWS dall'hardware e astrae i diversi backend LLM.

---

## Diagrammi

> Le immagini seguenti si trovano in `/00.ASSETS/images/05.Chatbot/ApplicationArchitecture/`

### Architettura completa del sistema

![Architecture Overview](/00.ASSETS/images/05.Chatbot/ApplicationArchitecture/architecture_overview.png)

### Sequence Diagram — Versione Bufferizzata

![Sequence Buffered](/00.ASSETS/images/05.Chatbot/ApplicationArchitecture/buffered.png)

### Sequence Diagram — Versione Streaming

![Sequence Streaming](/00.ASSETS/images/05.Chatbot/ApplicationArchitecture/streaming.png)

---

## Confronto tra le due versioni

| Aspetto | Versione Bufferizzata | Versione Streaming |
|---------|----------------------|--------------------|
| **File client** | `chatbot.c` + `rest_lib.c` | `chatbot_streaming.c` + `rest_lib_streaming.c` |
| **File proxy** | `llm_proxy.py` | `llm_proxy_streaming.py` |
| **Porta** | 8080 | 8081 |
| **Protocollo** | HTTP/1.0 | HTTP/1.1 + Transfer-Encoding: chunked |
| **EOF detection** | TCP close (`ip65_error=0x84`) | Chunk terminatore `0\r\n\r\n` |
| **Dimensione risposta** | Limitata a 2048 byte buffer | Illimitata (stream continuo) |
| **Latenza percepita** | Tutto-o-niente (attesa LLM) | Token per token (TTFT) |
| **Visualizzazione** | `print_wrapped()` word-wrap 40 col | `stream_putchar()` diretta a schermo |
| **Header "BOT:"** | Dopo ricezione completa | Prima di inviare la richiesta |
| **Pager** | Non necessario (risposta breve) | Sì — pausa ogni 20 righe |
| **Complessità I/O** | Bassa (buffer accumulate) | Alta (state machine chunked decoder) |
| **maxTokens LLM** | 150 | Nessun limite forzato |
| **Troncatura** | 250 caratteri (proxy) | Nessuna (stream continuo) |

---

## Stack tecnologico

| Livello | Bufferizzata | Streaming | Tecnologia |
|---------|-------------|-----------|------------|
| 5 — LLM Cloud | ✓ | ✓ | Amazon Nova, Claude, Ollama |
| 4 — Gateway | `llm_proxy.py` :8080 | `llm_proxy_streaming.py` :8081 | Python 3, Flask, boto3 |
| 3 — Rete LAN | HTTP/1.0 plain-text | HTTP/1.1 chunked | TCP/IP su Ethernet |
| 2 — Applicazione | `chatbot.c` | `chatbot_streaming.c` | C (cc65 cross-compiler) |
| 1 — Rete Apple IIe | `rest_lib.c` + ip65 | `rest_lib_streaming.c` + ip65 | C + Assembly 6502 |
| 0 — Hardware | ✓ (condiviso) | ✓ (condiviso) | Uthernet II (W5100) |

---

## Struttura dei file

```
05.CHATBOT/
├── chatbot.c               # UI bufferizzata, loop chat, word-wrap
├── chatbot_streaming.c     # UI streaming, stampa token per token
├── rest_lib.c              # HTTP builder e TCP polling (bufferizzato)
├── rest_lib_streaming.c    # HTTP/1.1 chunked decoder state machine
├── pch_chatbot.bin         # binario 6502 compilato
└── build/
    └── pch_chatbot.dsk     # immagine disco Apple IIe

100.REST-PROXY/
├── llm_proxy.py            # proxy Flask bufferizzato (porta 8080)
├── llm_proxy_streaming.py  # proxy Flask streaming (porta 8081)
└── README.MD

```

---

## Lato client — Versione Bufferizzata (05.CHATBOT)

### chatbot.c — UI e logica conversazione

Gestisce interazione utente e coordina le richieste. Non usa `malloc()`: tutti i buffer sono statici, dimensionati a compile-time.

**Costanti principali:**
```c
#define SCREEN_COLS    40    // display hardware fisso
#define INPUT_MAX     120    // max caratteri input utente
#define JSON_BUF_SIZE 160    // max payload JSON in uscita
```

**Funzioni chiave:**

| Funzione | Ruolo |
|----------|-------|
| `show_ascii_banner()` | Banner ASCII art 40 colonne, attende keypress |
| `show_welcome()` | Istruzioni d'uso |
| `initialize_network()` | Cerca Uthernet II negli slot 1-7, avvia DHCP |
| `read_line(buf, max)` | Legge input tastiera con backspace; `cgetc() & 0x7F` stripping bit 7 |
| `build_json(prompt, out, size)` | Costruisce `{"prompt":"..."}` con escaping manuale di `"` e `\` |
| `send_message(msg)` | Chiama `do_post()`, stampa risposta |
| `print_wrapped(text, cols)` | Word-wrap a 40 colonne senza spezzare parole |

**Loop principale:**
```c
for (;;) {
    read_line(input_buf, INPUT_MAX);
    if (input_buf[0] == 'Q') break;
    send_message(input_buf);          // POST → proxy → LLM
}
```

### rest_lib.c — Layer HTTP/TCP (bufferizzato)

Astrae la comunicazione TCP sopra ip65. Opera in modalità **busy-wait polling** (nessun interrupt disponibile).

**Configurazione hardcoded:**
```c
#define PROXY_HOST          "192.100.1.211"
#define PROXY_PORT          8080
#define RESPONSE_BUF_SIZE   2048
#define POLL_MAX_ITERATIONS 10000u    // timeout ~10-50 secondi
```

**Flusso di una richiesta POST:**
1. `do_post(path, json_body)` costruisce l'header HTTP nel buffer statico da 512 byte
2. `tcp_do_request()` esegue:
   - `parse_dotted_quad()` → converte IP stringa in `uint32_t`
   - `tcp_connect(ip, port, tcp_recv_cb)` via ip65
   - `tcp_send(request_buf, len)` via ip65
   - Loop polling: `ip65_process()` richiama `tcp_recv_cb()` ad ogni pacchetto ricevuto
   - Uscita dal loop quando `ip65_error = 0x84` (chiusura connessione TCP dal server = EOF HTTP/1.0)
3. Parsing HTTP: `sscanf(buf, "HTTP/1.%*d %hu", &status)` + `strstr(buf, "\r\n\r\n")`
4. Ritorna puntatore al body

**Callback ricevitore:**
```c
static void __fastcall__ tcp_recv_cb(const uint8_t *data, int len) {
    // __fastcall__ richiesto da ip65: passa args in registri 6502
    copy_len = min(len, s_recv_remaining);
    memcpy(s_recv_ptr, data, copy_len);
    s_recv_ptr       += copy_len;
    s_recv_remaining -= copy_len;
}
```

---

## Lato client — Versione Streaming (05.CHATBOT)

### chatbot_streaming.c — UI streaming real-time

Differenze rispetto alla versione bufferizzata:

- Stampa il prompt `BOT:\n---...---\n` **prima** di inviare la richiesta HTTP, così l'utente vede subito il feedback
- Non usa `print_wrapped()`: i token arrivano già formattati e vengono stampati direttamente
- Usa `do_post_stream()` invece di `do_post()` — la funzione ritorna solo a stream completato
- Il pager è integrato nello strato `rest_lib_streaming.c`

```c
// La sequenza nel loop principale è diversa:
printf("BOT:\n");
printf("---...---\n");        // header immediato, prima della risposta LLM
do_post_stream("/api/data", json_buf);   // stampa i caratteri mentre arrivano
printf("\n---...---\n\n");
```

### rest_lib_streaming.c — Layer HTTP/TCP con chunked decoder

La parte più complessa del progetto: implementa un decodificatore HTTP/1.1 chunked encoding come **state machine** in Assembly/C bare-metal.

**Configurazione:**
```c
#define PROXY_HOST          "192.100.1.211"
#define PROXY_PORT_STREAM   8081
#define IDLE_TIMEOUT        20000u    // iterazioni senza dati → timeout
```

**State machine chunked decoder — 5 stati:**

```
              ┌──────────────────────────────────────────────────────────┐
              │           Chunked Transfer-Encoding Decoder              │
              └──────────────────────────────────────────────────────────┘

  Ricezione     ┌─────────┐  \r\n   ┌──────────────┐
  hex size  ──► │ CK_SIZE │ ──────► │ CK_SIZE_LF   │
                └─────────┘         └──────────────┘
                                           │ \n (LF)
                                           ▼
              ┌─────────────┐        ┌──────────┐
              │  CK_DONE    │◄──────-│ CK_DATA  │ ──► stream_putchar()
              │  (size=0)   │  size  └──────────┘     (stampa a schermo)
              └─────────────┘  = 0        │ count==0
                                          ▼
                                   ┌──────────────┐  \r\n  ┌──────────────┐
                                   │ CK_TRAIL_CR  │ ──────► │ CK_TRAIL_LF  │
                                   └──────────────┘         └──────────────┘
                                                                    │ torna a CK_SIZE
                                                                    ▼
                                                             (prossimo chunk)
```

**Callback streaming:**
```c
static void __fastcall__ tcp_stream_cb(const uint8_t *data, int len) {
    // Per ogni byte ricevuto, avanza la state machine
    // In stato CK_DATA: chiama stream_putchar() che stampa direttamente
    // Gestisce pager: incrementa line_count, pausa a 20 righe
}

static void stream_putchar(char c) {
    putchar(c);
    if (c == '\n') {
        line_count++;
        if (line_count >= PAGE_LINES) {
            printf("-- PIU': PREMI UN TASTO --");
            cgetc();
            line_count = 0;
        }
    }
}
```

**Gestione fine stream:**
- Il proxy Flask invia `0\r\n\r\n` come ultimo chunk
- La state machine entra in `CK_DONE`
- Il loop di polling esce quando `ip65_error = 0x84` (TCP close) oppure quando `chunk_state == CK_DONE`

---

## Protocollo di comunicazione

### Versione Bufferizzata — HTTP/1.0

**Richiesta (Apple IIe → Proxy):**
```
POST /api/data HTTP/1.0\r\n
Host: 192.100.1.211\r\n
Content-Type: application/json\r\n
Content-Length: 43\r\n
Connection: close\r\n
\r\n
{"prompt":"chi ha inventato il computer?"}
```

**Risposta (Proxy → Apple IIe):**
```
HTTP/1.0 200 OK\r\n
Content-Type: text/plain\r\n
\r\n
CHARLES BABBAGE INVENTO' LA MACCHINA ANALITICA NEL 1837.
```

**Perché HTTP/1.0:** la chiusura TCP da parte del server funge da delimitatore di fine risposta (`ip65_error = 0x84`), evitando il parsing del `Content-Length` o del chunked encoding.

---

### Versione Streaming — HTTP/1.1 Chunked

**Richiesta (Apple IIe → Proxy):**
```
POST /api/data HTTP/1.0\r\n          ← Il client usa ancora HTTP/1.0 in richiesta
Host: 192.100.1.211:8081\r\n
Content-Type: application/json\r\n
Content-Length: 43\r\n
Connection: close\r\n
\r\n
{"prompt":"che cos'e' una rete neurale?"}
```

**Risposta (Proxy → Apple IIe) — Chunked Transfer Encoding:**
```
HTTP/1.1 200 OK\r\n
Transfer-Encoding: chunked\r\n
Content-Type: text/plain; charset=utf-8\r\n
\r\n
3\r\n                ← chunk 1: 3 byte
UNA\r\n
5\r\n                ← chunk 2: 5 byte
 RETE\r\n
8\r\n                ← chunk 3: 8 byte
 NEURALE\r\n
...
0\r\n                ← chunk terminatore (size = 0)
\r\n
```

**Perché Chunked TE:** Flask + generator Python emettono automaticamente chunked encoding quando la risposta è un `Response(stream_with_context(...))`. Il client 6502 deve decodificarlo manualmente con la state machine.

---

## Lato server — Versione Bufferizzata (100.REST-PROXY)

### llm_proxy.py — Gateway Flask (porta 8080)

**Dipendenze:**
```python
flask        # framework HTTP
boto3        # AWS SDK → Bedrock
python-dotenv   # caricamento .env
requests     # per backend Ollama
anthropic    # SDK Anthropic
```

**Configurazione (.env):**
```bash
INFERENCE_MODE=cloud          # "cloud" | "anthropic" | "ollama"
LLM_MODEL_CLOUD=eu.amazon.nova-lite-v1:0
LLM_MODEL_ANTHROPIC=eu.anthropic.claude-haiku-4-5-20251001-v1:0
LLM_MODEL_OLLAMA=llama3.2
OLLAMA_BASE_URL=http://localhost:11434
AWS_DEFAULT_REGION=eu-central-1
AWS_ACCESS_KEY_ID=<your_key>
AWS_SECRET_ACCESS_KEY=<your_secret>
```

### Endpoint esposti

**GET `/api/data?prompt=<testo>`** — test e query via query string  
**POST `/api/data`** con body `{"prompt":"<testo>"}` — uso principale da chatbot.c

### Pipeline di elaborazione (comune a tutti i backend)

```
prompt
  │
  ▼
Backend selection (INFERENCE_MODE)
  ├─ cloud      → call_nova()       → bedrock.invoke_model()
  ├─ anthropic  → call_anthropic()  → anthropic.messages.create()
  └─ ollama     → call_ollama()     → POST http://localhost:11434/api/generate
  │
  ▼
result text (stringa)
  │
  ▼
" ".join(text.split())          # normalizza spazi multipli
  │
  ▼
clean_for_apple2(text)          # accenti UTF-8 → ASCII apostrofato
  │
  ▼
tronca a MAX 250 caratteri      # all'ultimo spazio (no parole spezzate)
  │
  ▼
.upper()                        # maiuscolo (display Apple IIe)
  │
  ▼
risposta plain-text (body HTTP)
```

### clean_for_apple2(text)

Converte Unicode in ASCII 7 bit compatibile con il display hardware:

```python
replacements = {
    'à': "a'", 'è': "e'", 'ì': "i'", 'ò': "o'", 'ù': "u'",
    'á': "a'", 'é': "e'", 'í': "i'", 'ó': "o'", 'ú': "u'",
    'À': "A'", 'È': "E'",
}
# dopo le sostituzioni: filtra tutto ciò che è > ASCII 127
return "".join(c for c in text if ord(c) < 128)
```

**Esempio:**
- Input: `"Perché l'intelligenza artificiale è il futuro"`
- Output: `"PERCHE' L'INTELLIGENZA ARTIFICIALE E' IL FUTURO"`

---

## Lato server — Versione Streaming (100.REST-PROXY)

### llm_proxy_streaming.py — Gateway Flask Streaming (porta 8081)

La differenza chiave rispetto alla versione bufferizzata: usa **generator Python** e `Response(stream_with_context(...))`. Flask converte automaticamente la risposta in HTTP/1.1 chunked encoding.

### Endpoint streaming

**POST `/api/data`** — stesso path, porta diversa (8081)

```python
@app.route("/api/data", methods=["GET", "POST"])
def api_data():
    prompt = extract_prompt(request)
    
    if INFERENCE_MODE == "cloud":
        gen = stream_nova(prompt)
    elif INFERENCE_MODE == "anthropic":
        gen = stream_anthropic(prompt)
    else:
        gen = stream_ollama(prompt)
    
    return Response(stream_with_context(gen), content_type="text/plain")
    # Flask aggiunge automaticamente: Transfer-Encoding: chunked
```

### Generator per ogni backend

**Amazon Nova (Bedrock streaming):**
```python
def stream_nova(prompt):
    response = bedrock.converse_stream(
        modelId=LLM_MODEL_CLOUD,
        messages=[{"role": "user", "content": [{"text": prompt}]}],
        system=[{"text": SYSTEM_PROMPT}],
    )
    for event in response["stream"]:
        if "contentBlockDelta" in event:
            token = event["contentBlockDelta"]["delta"]["text"]
            yield clean_for_apple2(token).upper()
```

**Anthropic Claude (streaming):**
```python
def stream_anthropic(prompt):
    with anthropic_client.messages.stream(
        model=LLM_MODEL_ANTHROPIC,
        messages=[{"role": "user", "content": prompt}],
        system=SYSTEM_PROMPT,
        max_tokens=1024,
    ) as stream:
        for token in stream.text_stream:
            yield clean_for_apple2(token).upper()
```

**Ollama (streaming locale):**
```python
def stream_ollama(prompt):
    response = requests.post(
        f"{OLLAMA_BASE_URL}/api/generate",
        json={"model": LLM_MODEL_OLLAMA, "prompt": prompt, "stream": True},
        stream=True,
    )
    for line in response.iter_lines():
        if line:
            data = json.loads(line)
            token = data.get("response", "")
            if token:
                yield clean_for_apple2(token).upper()
```

**Nota importante:** nella versione streaming, `clean_for_apple2()` è applicata **per token** invece che all'intera risposta, e non c'è troncatura a 250 caratteri (la lunghezza è controllata da `max_tokens` del LLM e dallo scroll naturale del display).

---

## ip65 — Network stack Assembly 6502

Libreria di terze parti che implementa TCP/IP direttamente in Assembly 6502. Interfaccia con il chip W5100 della scheda Uthernet II tramite memoria mappata (memory-mapped I/O).

**API usata (identica in entrambe le versioni):**
```c
dhcp_init()                           // configurazione rete
tcp_connect(ip32, port, callback)     // apertura connessione
tcp_send(data, len)                   // invio dati
tcp_close()                           // chiusura connessione
ip65_process()                        // polling ricezione pacchetti
uint8_t ip65_error                    // codice errore (0x84 = EOF/close)
uint32_t parse_dotted_quad(str)       // "192.100.1.211" → uint32
```

**Compilazione (stessa per entrambe le versioni):**
```bash
cl65 -t apple2 chatbot.c -o pch_chatbot.bin -O -m chatbot.map \
    -vm ../00.LIBRERIE/IP65/lib/ip65_tcp.lib \
       ../00.LIBRERIE/IP65/drivers/ip65_apple2.lib
```

---

## Flusso dati end-to-end

### Versione Bufferizzata

```
UTENTE digita: "che cos'e' una rete neurale?"
    │
    ▼
[chatbot.c] read_line()
    → input_buf = "che cos'e' una rete neurale?"
    │
    ▼
[chatbot.c] build_json()
    → json_buf = {"prompt":"che cos'e' una rete neurale?"}
    │
    ▼
[chatbot.c] send_message()
    │
    ▼
[rest_lib.c] do_post("/api/data", json_buf)
    │
    ▼
[ip65] tcp_connect(192.100.1.211, 8080)
[ip65] tcp_send(request HTTP)
[ip65] polling loop → tcp_recv_cb() accumula risposta in buffer 2048 byte
[ip65] ip65_error=0x84 → chiusura TCP → fine risposta (EOF implicito)
    │
    ▼
── LAN HTTP/1.0 plain-text ─────────────────────────────────────────
    │
    ▼
[llm_proxy.py] POST /api/data
    → estrae prompt dal JSON
    → call_nova() / call_anthropic() / call_ollama()
    → clean_for_apple2() + troncatura 250 char + upper()
    → HTTP/1.0 200 OK + body plain-text
    │
    ▼
── HTTPS verso LLM cloud (trasparente al client) ───────────────────
    │
    ▼
[rest_lib.c] parse HTTP status + estrae body
    │
    ▼
[chatbot.c] print_wrapped(risposta, 40)
    │
    ▼
SCHERMO APPLE IIe (tutto in una volta, dopo attesa):
  ──────────────────────────────────────
  BOT:
  ──────────────────────────────────────
  UNA RETE NEURALE E' UN MODELLO DI
  CALCOLO ISPIRATO AL CERVELLO UMANO,
  CHE IMPARA DALL'ESPERIENZA.
  ──────────────────────────────────────
```

### Versione Streaming

```
UTENTE digita: "che cos'e' una rete neurale?"
    │
    ▼
[chatbot_streaming.c] read_line() + build_json()
    │
    ▼
[chatbot_streaming.c] stampa IMMEDIATA:
    → "BOT:\n──────\n"     ← appare sul display PRIMA della risposta LLM
    │
    ▼
[rest_lib_streaming.c] do_post_stream("/api/data", json_buf)
    │
    ▼
[ip65] tcp_connect(192.100.1.211, 8081)
[ip65] tcp_send(request HTTP)
[ip65] polling loop → tcp_stream_cb() per ogni byte ricevuto
    │
    ▼
── LAN HTTP/1.1 Chunked TE ─────────────────────────────────────────
    │
    ▼
[llm_proxy_streaming.py] POST /api/data
    → stream_nova() / stream_anthropic() / stream_ollama()
    → yield token per token
    → Flask → Transfer-Encoding: chunked automatico
    │
    ▼ (ogni token diventa un chunk HTTP separato)
    │
[rest_lib_streaming.c] tcp_stream_cb() per ogni byte:
    → state machine: CK_SIZE → CK_DATA → CK_TRAIL → loop
    → in CK_DATA: stream_putchar(c) → putchar(c) sul display
    → contatore righe: pausa ogni 20 righe (pager)
    │
    ▼ (chunk terminatore 0\r\n\r\n ricevuto)
[rest_lib_streaming.c] CK_DONE → uscita loop
    │
    ▼
SCHERMO APPLE IIe (token per token, real-time):
  ──────────────────────────────────────
  BOT:
  ──────────────────────────────────────
  UNA RETE NEURALE E' UN MODEL_    ← appare mentre LLM genera
  LO DI CALCOLO ISPIRATO AL CE
  RVELLO UMANO, CHE IMPARA DAL
  L'ESPERIENZA.
  -- PIU': PREMI UN TASTO --       ← pager a 20 righe
```

---

## Vincoli hardware e soluzioni di design

| Vincolo | Origine | Soluzione adottata |
|---------|---------|-------------------|
| No TLS/HTTPS | ip65 / CPU 6502 | Proxy Flask termina TLS, LAN in plain-text |
| Stack fisico 256 byte | Hardware 6502 | Call chain max 5 livelli: `main→send_message→do_post→tcp_do_request→ip65_process` |
| RAM 64 KB (~40 KB utili) | Apple IIe | Buffer statici, nessun `malloc()`, dimensioni conservative |
| Solo ASCII 7-bit a display | Hardware video | `clean_for_apple2()` proxy + `cgetc() & 0x7F` client |
| No parser JSON | Bare-metal, no stdlib | `build_json()` manuale con escaping esplicito |
| I/O non bloccante assente | ip65 / no OS | Busy-wait con `ip65_process()` fino a 10.000 iterazioni |
| Risposta LLM potenzialmente lunga | LLM verbosity | Bufferizzato: `maxTokens:150` + troncatura 250 char; Streaming: scroll naturale + pager |
| Fine risposta ambigua (bufferizzata) | HTTP/1.0 | Chiusura TCP (`ip65_error=0x84`) = EOF implicito |
| Fine risposta (streaming) | HTTP/1.1 chunked | Chunk terminatore `0\r\n\r\n` + stato `CK_DONE` |
| Credenziali AWS | Sicurezza | Solo sul proxy — il client non vede mai chiavi AWS |
| Display 40 colonne | Hardware fisso | Bufferizzato: `print_wrapped()`; Streaming: token brevi, scroll naturale |
| No memoria conversazione | RAM insufficiente | Architettura stateless — ogni turno è indipendente |
| Chunked encoding | CPU 6502 lenta | State machine ottimizzata, 5 stati, zero allocazioni |
| Latenza token variabile | Rete + LLM | Timeout idle: 20.000 iterazioni senza dati → abort |

---

## Limiti di dimensione buffer

```
                   Bufferizzata    Streaming
INPUT_MAX          = 120 byte      120 byte   (input utente)
JSON_BUF_SIZE      = 160 byte      160 byte   (payload JSON verso proxy)
request_buf        = 512 byte      512 byte   (richiesta HTTP completa)
RESPONSE_BUF_SIZE  = 2048 byte     —          (solo versione bufferizzata)
MAX_RESPONSE_CHARS = 250 char      —          (solo proxy bufferizzato)
PAGE_LINES         = —             20 righe   (solo versione streaming)
```

---

## Sequenza di avvio

```
Versione bufferizzata:
1. Boot Apple IIe → carica pch_chatbot.bin da disco
2. show_ascii_banner()      — attende keypress
3. show_welcome()           — istruzioni
4. initialize_network()     — cerca Uthernet II slot 1-7, DHCP
5. do_get("/api/data?prompt=test")  — test connettività proxy
6. Loop chat:
     read_line() → build_json() → do_post() → print_wrapped()
7. Input 'Q' → uscita al monitor Apple IIe

Versione streaming (differenze):
5. do_get("/api/data?prompt=test")  — test connettività proxy porta 8081
6. Loop chat:
     read_line() → build_json()
     → printf("BOT:\n") immediato
     → do_post_stream() → tcp_stream_cb() → stream_putchar()
     → (pager ogni 20 righe)
7. Input 'Q' → uscita al monitor Apple IIe
```

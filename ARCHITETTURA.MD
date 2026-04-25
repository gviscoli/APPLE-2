# Architettura del Sistema PCH.CHATBOT

Un Apple IIe del 1983 (MOS 6502 @ 1 MHz, 64 KB RAM) che dialoga con Amazon Nova via AWS Bedrock attraverso un proxy Python intermedio.

---

## Panoramica

```
┌─────────────────────┐        LAN HTTP/1.0        ┌──────────────────────┐        HTTPS        ┌──────────────────────┐
│   Apple IIe (6502)  │ ─────────────────────────► │  Proxy Python/Flask  │ ──────────────────► │   AWS Bedrock        │
│   pch_chatbot.bin   │ ◄───────────────────────── │  llm_proxy.py :8080  │ ◄────────────────── │  Amazon Nova Lite    │
│   Uthernet II       │    plain-text, no TLS        │  192.100.1.211       │    us-east-1         │                      │
└─────────────────────┘                             └──────────────────────┘                     └──────────────────────┘
  cartella 05.CHATBOT                               cartella 100.REST-PROXY
```

**Motivazione architetturale:** il chip 6502 non può eseguire TLS né HTTP/1.1 chunked encoding. Il proxy risolve entrambi i problemi e isola le credenziali AWS dall'hardware.

---

## Stack tecnologico

| Livello | Componente | Tecnologia |
|---------|-----------|------------|
| 5 - LLM Cloud | Amazon Nova Lite v1 | AWS Bedrock (us-east-1) |
| 4 - Gateway | `llm_proxy.py` | Python 3, Flask, boto3 |
| 3 - Rete LAN | HTTP/1.0 plain-text | TCP/IP su Ethernet |
| 2 - Applicazione | `chatbot.c` | C (cc65 cross-compiler) |
| 1 - Rete Apple IIe | `rest_lib.c` + ip65 | C + Assembly 6502 |
| 0 - Hardware | Uthernet II (W5100) | Slot espansione Apple IIe |

---

## Struttura dei file

```
05.CHATBOT/
├── chatbot.c           # UI, loop chat, word-wrap
├── rest_lib.c          # HTTP builder, TCP polling
├── pch_chatbot.bin     # binario 6502 compilato
└── build/
    └── pch_chatbot.dsk # immagine disco Apple IIe

100.REST-PROXY/
├── llm_proxy.py        # proxy Flask in produzione
├── proxy.py            # template minimalista
└── README.MD
```

---

## Lato client: Apple IIe (05.CHATBOT)

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
    send_message(input_buf);          // POST → proxy → Nova
}
```

### rest_lib.c — Layer HTTP/TCP

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

### ip65 — Network stack Assembly 6502

Libreria di terze parti che implementa TCP/IP direttamente in Assembly 6502. Interfaccia con il chip W5100 della scheda Uthernet II tramite memoria mappata (memory-mapped I/O).

**API usata:**
```c
dhcp_init()                           // configurazione rete
tcp_connect(ip32, port, callback)     // apertura connessione
tcp_send(data, len)                   // invio dati
tcp_close()                           // chiusura connessione
ip65_process()                        // polling ricezione pacchetti
uint8_t ip65_error                    // codice errore (0x84 = EOF/close)
uint32_t parse_dotted_quad(str)       // "192.100.1.211" → uint32
```

**Compilazione:**
```bash
cl65 -t apple2 chatbot.c -o pch_chatbot.bin -O -m chatbot.map \
    -vm ../00.LIBRERIE/IP65/lib/ip65_tcp.lib \
       ../00.LIBRERIE/IP65/drivers/ip65_apple2.lib
```

---

## Protocollo di comunicazione

### Richiesta (Apple IIe → Proxy)

```
POST /api/data HTTP/1.0\r\n
Host: 192.100.1.211\r\n
Content-Type: application/json\r\n
Content-Length: 43\r\n
Connection: close\r\n
\r\n
{"prompt":"chi ha inventato il computer?"}
```

### Risposta (Proxy → Apple IIe)

```
HTTP/1.0 200 OK\r\n
Content-Type: text/plain\r\n
\r\n
CHARLES BABBAGE INVENTO' LA MACCHINA ANALITICA NEL 1837.
```

**Perché HTTP/1.0:** la chiusura della connessione TCP da parte del server funge da delimitatore di fine risposta (`ip65_error = 0x84`), evitando il parsing del `Content-Length` o del chunked encoding di HTTP/1.1.

---

## Lato server: Proxy Python (100.REST-PROXY)

### llm_proxy.py — Gateway Flask

**Dipendenze:**
```python
flask       # framework HTTP
boto3       # AWS SDK → Bedrock
python-dotenv  # caricamento .env
```

**Configurazione (.env):**
```bash
AWS_DEFAULT_REGION=us-east-1
LLM_MODEL_CLOUD=amazon.nova-lite-v1:0
INFERENCE_MODE=cloud    # "cloud" | "local"
```

### Endpoint esposti

**GET `/api/data?prompt=<testo>`** — test e query via query string  
**POST `/api/data`** con body `{"prompt":"<testo>"}` — uso principale da chatbot.c

### Funzione call_nova(prompt)

Pipeline di elaborazione:

```
prompt
  │
  ▼
bedrock.invoke_model(
  modelId="amazon.nova-lite-v1:0",
  body={
    "system": [{"text": "Sei un assistente per Apple IIe..."}],
    "messages": [{"role": "user", "content": [{"text": prompt}]}],
    "inferenceConfig": {"maxTokens": 150, "temperature": 0.4}
  }
)
  │
  ▼
result["output"]["message"]["content"][0]["text"]
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
risposta plain-text
```

### clean_for_apple2(text)

Converte Unicode in ASCII a 7 bit compatibile con il display hardware:

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

## Flusso dati end-to-end

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
[ip65] polling loop → tcp_recv_cb() accumula risposta
[ip65] ip65_error=0x84 → chiusura TCP → fine risposta
    │
    ▼
── LAN (HTTP/1.0 plain-text) ──────────────────────────
    │
    ▼
[llm_proxy.py] POST /api/data
    → estrae prompt dal JSON
    │
    ▼
[boto3] bedrock.invoke_model("amazon.nova-lite-v1:0")
    │
    ▼
── HTTPS (AWS Signature V4) ────────────────────────────
    │
    ▼
[Amazon Nova] genera risposta in italiano
    → "Una rete neurale è un modello di calcolo..."
    │
    ▼
[llm_proxy.py] post-processing
    → clean_for_apple2 + troncatura 250 car + upper()
    → "UNA RETE NEURALE E' UN MODELLO DI CALCOLO..."
    │
    ▼
── LAN (HTTP/1.0 response plain-text) ─────────────────
    │
    ▼
[rest_lib.c] parse HTTP status + estrae body
    │
    ▼
[chatbot.c] print_wrapped(risposta, 40)
    │
    ▼
SCHERMO APPLE IIe:
  ──────────────────────────────────────
  BOT:
  ──────────────────────────────────────
  UNA RETE NEURALE E' UN MODELLO DI
  CALCOLO ISPIRATO AL CERVELLO UMANO,
  CHE IMPARA DALL'ESPERIENZA.
  ──────────────────────────────────────
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
| Risposta LLM potenzialmente lunga | LLM | `maxTokens:150` + troncatura a 250 char all'ultimo spazio |
| Fine risposta ambigua | HTTP/1.0 | Chiusura TCP (`ip65_error=0x84`) = EOF implicito |
| Credenziali AWS | Sicurezza | Solo sul proxy — il client non vede mai chiavi AWS |
| Display 40 colonne | Hardware fisso | `print_wrapped()` con rilevamento word-boundary |
| No memoria conversazione | RAM insufficiente | Architettura stateless — ogni turno è indipendente |

---

## Limiti di dimensione buffer

```
INPUT_MAX      = 120 byte   (input utente)
JSON_BUF_SIZE  = 160 byte   (payload JSON verso proxy)
request_buf    = 512 byte   (richiesta HTTP completa)
RESPONSE_BUF_SIZE = 2048 byte  (risposta HTTP completa)
MAX_RESPONSE_CHARS = 250 char  (risposta testuale finale)
```

---

## Sequenza di avvio

```
1. Boot Apple IIe → carica pch_chatbot.bin da disco
2. show_ascii_banner()      — attende keypress
3. show_welcome()           — istruzioni
4. initialize_network()     — cerca Uthernet II slot 1-7, DHCP
5. do_get("/api/data?prompt=test")  — test connettività proxy
6. Loop chat:
     read_line() → build_json() → do_post() → print_wrapped()
7. Input 'Q' → uscita al monitor Apple IIe
```

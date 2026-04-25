import os
import json
import boto3
import requests

from flask import Flask, request, jsonify
from dotenv import load_dotenv, find_dotenv

app = Flask(__name__)

path_env = find_dotenv()
load_dotenv(path_env)
print(f"File .env caricato da: {path_env}")

AWS_DEFAULT_REGION  = os.getenv("AWS_DEFAULT_REGION")
LLM_MODEL_CLOUD     = os.getenv("LLM_MODEL_CLOUD")
LLM_MODEL_ANTHROPIC = os.getenv("LLM_MODEL_ANTHROPIC", "anthropic.claude-3-haiku-20240307-v1:0")
LLM_MODEL_OLLAMA    = os.getenv("LLM_MODEL_OLLAMA", "llama4")
OLLAMA_BASE_URL     = os.getenv("OLLAMA_BASE_URL", "http://localhost:11434")
INFERENCE_MODE      = os.getenv("INFERENCE_MODE", "cloud").lower()

# ---------------------------------------------------------------
# Client AWS Bedrock
# Assicurati di avere le credenziali AWS configurate:
#   - variabili d'ambiente AWS_ACCESS_KEY_ID / AWS_SECRET_ACCESS_KEY
#   - oppure ~/.aws/credentials
#   - oppure IAM role se gira su EC2
# ---------------------------------------------------------------
bedrock = boto3.client(
    service_name="bedrock-runtime",
    region_name=AWS_DEFAULT_REGION        
)

system_prompt = (
    "Sei un assistente virtuale che verrà utilizzato da un computer Apple IIe del 1983 ma oggi siamo nel 2026. "
    "ricordati che nel 1983 i sistemi di Intelligenza Artificiale non esistevano ancora"
    "prima di rispondere verifica le informazioni su internet e dai solo risposte concise, dirette e senza giri di parole. "
    "Rispondi in ITALIANO. Sii estremamente sintetico. "
    "La tua risposta NON deve superare i 240 caratteri. "
    "Non usare markdown (niente grassetto o tabelle)."
)

if INFERENCE_MODE == "ollama":
    print(f"Modalità INFERENCE: OLLAMA locale (modello {LLM_MODEL_OLLAMA}, URL {OLLAMA_BASE_URL})")
    MODEL_ID = LLM_MODEL_OLLAMA
elif INFERENCE_MODE == "anthropic":
    print(f"Modalità INFERENCE: ANTHROPIC via Bedrock (modello {LLM_MODEL_ANTHROPIC})")
    MODEL_ID = LLM_MODEL_ANTHROPIC
elif INFERENCE_MODE == "cloud":
    print(f"Modalità INFERENCE: CLOUD Nova (modello {LLM_MODEL_CLOUD})")
    MODEL_ID = LLM_MODEL_CLOUD
else:
    raise ValueError(f"INFERENCE_MODE non valido: '{INFERENCE_MODE}'. Usa 'cloud', 'anthropic', 'ollama' o 'local'.")


# ---------------------------------------------------------------
# Lunghezza massima risposta — Apple IIe ha poca RAM!
# ---------------------------------------------------------------
MAX_RESPONSE_CHARS = 250

#region clean_for_apple2

def clean_for_apple2(text: str) -> str:
    """
    Converte le lettere accentate in formato compatibile Apple II (ASCII 7-bit).
    Esempio: 'perché' -> "perche'"
    """
    replacements = {
        'à': "a'", 'á': "a'",
        'è': "e'", 'é': "e'",
        'ì': "i'", 'í': "i'",
        'ò': "o'", 'ó': "o'",
        'ù': "u'", 'ú': "u'",
        'È': "E'", 'À': "A'"
    }
    
    # Sostituzione manuale per precisione
    for char, replacement in replacements.items():
        text = text.replace(char, replacement)
    
    # Rimuove eventuali altri caratteri non ASCII (Emoji, simboli strani)
    # mantenendo solo i caratteri leggibili dall'Apple II
    return "".join(c for c in text if ord(c) < 128)

#endregion

#region call_nova

# ---------------------------------------------------------------
# Funzione per chiamare Amazon Nova tramite Bedrock
# ---------------------------------------------------------------

def call_nova(prompt: str) -> str:
    # Chiediamo all'LLM di essere breve e di non usare fronzoli


    body = {
        "system": [{"text": system_prompt}],
        "messages": [
            {"role": "user", "content": [{"text": prompt}]}
        ],
        "inferenceConfig": {
            "maxTokens": 150,
            "temperature": 0.4 # Più bassa per maggiore precisione sul limite
        }
    }

    try:
        response = bedrock.invoke_model(
            modelId=MODEL_ID,
            body=json.dumps(body),
            contentType="application/json",
            accept="application/json"
        )

        result = json.loads(response["body"].read())
        text = result["output"]["message"]["content"][0]["text"]

        # 1. Normalizza spazi e newline
        text = " ".join(text.split())

        # 2. Converti accenti per Apple II
        text = clean_for_apple2(text)

        # 3. Taglio di sicurezza (se l'LLM ha sforato) senza troncare parole
        if len(text) > MAX_RESPONSE_CHARS:
            truncated = text[:MAX_RESPONSE_CHARS]
            last_space = truncated.rfind(' ')
            text = truncated[:last_space] if last_space != -1 else truncated

        return text.upper() # Opzionale: l'Apple II spesso legge meglio tutto in MAIUSCOLO
    
    except Exception as e:
        print(f"Errore: {e}")
        return "ERRORE DI CONNESSIONE"

#endregion

#region call_anthropic

# ---------------------------------------------------------------
# Funzione per chiamare modelli Anthropic (Claude) tramite Bedrock
#
# Configura nel .env:
#   LLM_MODEL_ANTHROPIC=anthropic.claude-3-haiku-20240307-v1:0
#   INFERENCE_MODE=anthropic
#
# Modelli disponibili su Bedrock (esempi):
#   anthropic.claude-3-haiku-20240307-v1:0     (veloce, economico)
#   anthropic.claude-3-5-sonnet-20241022-v2:0  (bilanciato)
#   anthropic.claude-3-opus-20240229-v1:0      (più capace)
# ---------------------------------------------------------------

def call_anthropic(prompt: str) -> str:

    body = {
        "anthropic_version": "bedrock-2023-05-31",
        "max_tokens": 150,
        "temperature": 0.4,
        "system": system_prompt,
        "messages": [
            {"role": "user", "content": prompt}
        ]
    }

    try:
        response = bedrock.invoke_model(
            modelId=LLM_MODEL_ANTHROPIC,
            body=json.dumps(body),
            contentType="application/json",
            accept="application/json"
        )

        result = json.loads(response["body"].read())
        text = result["content"][0]["text"]

        text = " ".join(text.split())
        text = clean_for_apple2(text)

        if len(text) > MAX_RESPONSE_CHARS:
            truncated = text[:MAX_RESPONSE_CHARS]
            last_space = truncated.rfind(' ')
            text = truncated[:last_space] if last_space != -1 else truncated

        return text.upper()

    except Exception as e:
        print(f"[call_anthropic] Errore: {e}")
        return "ERRORE ANTHROPIC BEDROCK"

#endregion

#region call_ollama

# ---------------------------------------------------------------
# Funzione per chiamare un modello llama4 su server Ollama locale
#
# Configura nel .env:
#   OLLAMA_BASE_URL=http://localhost:11434
#   LLM_MODEL_OLLAMA=llama4
#   INFERENCE_MODE=ollama
#
# Avvia Ollama prima di usare il proxy:
#   ollama serve
#   ollama pull llama4
# ---------------------------------------------------------------

def call_ollama(prompt: str) -> str:

    payload = {
        "model": LLM_MODEL_OLLAMA,
        "messages": [
            {"role": "system", "content": system_prompt},
            {"role": "user",   "content": prompt}
        ],
        "stream": False,
        "options": {
            "num_predict": 150,
            "temperature": 0.4
        }
    }

    try:
        response = requests.post(
            f"{OLLAMA_BASE_URL}/api/chat",
            json=payload,
            timeout=60
        )
        response.raise_for_status()

        text = response.json()["message"]["content"]

        text = " ".join(text.split())
        text = clean_for_apple2(text)

        if len(text) > MAX_RESPONSE_CHARS:
            truncated = text[:MAX_RESPONSE_CHARS]
            last_space = truncated.rfind(' ')
            text = truncated[:last_space] if last_space != -1 else truncated

        return text.upper()

    except requests.exceptions.ConnectionError:
        print(f"[call_ollama] Server Ollama non raggiungibile su {OLLAMA_BASE_URL}")
        return "ERRORE: SERVER OLLAMA NON RAGGIUNGIBILE"
    except Exception as e:
        print(f"[call_ollama] Errore: {e}")
        return "ERRORE OLLAMA"

#endregion

#region _dispatch

_MODEL_KEYWORDS = ("modello", "model", "chi sei", "who are you", "che llm", "quale ia", "quale ai", "qual è il tuo nome", "what is your name", "dimmi che modello stai usando")

def _dispatch(prompt: str) -> str:
    """Smista la chiamata alla funzione LLM in base a INFERENCE_MODE."""
    if any(kw in prompt.lower() for kw in _MODEL_KEYWORDS):
        return f"MODO:{INFERENCE_MODE.upper()} MODELLO:{MODEL_ID}"
    if INFERENCE_MODE == "anthropic":
        return call_anthropic(prompt)
    elif INFERENCE_MODE == "ollama":
        return call_ollama(prompt)
    else:
        return call_nova(prompt)

#endregion

#region get_data

# ---------------------------------------------------------------
# GET /api/data
#
# Uso diretto da browser/curl:
#   curl "http://192.100.1.211:8080/api/data?prompt=ciao"
#
# Uso da Apple IIe:
#   do_get("/api/data?prompt=ciao+come+stai", buf, size)
#
# Se non viene passato un prompt, risponde con un messaggio fisso.
# ---------------------------------------------------------------
@app.route('/api/data', methods=['GET'])
def get_data():
    prompt = request.args.get('prompt', '').strip()

    if not prompt:
        return f"HELLO FROM PROXY! Modalita': {INFERENCE_MODE.upper()}. Passa ?prompt=testo per chiamare il modello."

    print(f"[GET] Prompt ricevuto: {prompt}")

    try:
        risposta = _dispatch(prompt)
        print(f"[GET] Risposta ({INFERENCE_MODE}): {risposta}")
        return risposta
    except Exception as e:
        print(f"[GET] Errore: {e}")
        return f"ERRORE: {str(e)[:100]}", 500

#endregion

#region post_data

# ---------------------------------------------------------------
# POST /api/data
#
# Uso da curl:
#   curl -X POST http://192.100.1.211:8080/api/data \
#        -H "Content-Type: application/json" \
#        -d '{"prompt": "chi era Steve Jobs?"}'
#
# Uso da Apple IIe:
#   do_post("/api/data",
#           "{\"prompt\":\"chi era Steve Jobs?\"}",
#           buf, size)
#
# Il body JSON deve contenere il campo "prompt".
# ---------------------------------------------------------------
@app.route('/api/data', methods=['POST'])
def post_data():
    # Accetta sia JSON che form data
    if request.is_json:
        data = request.get_json(silent=True) or {}
    else:
        # Fallback: prova a parsare il body come JSON grezzo
        try:
            data = json.loads(request.data.decode('utf-8'))
        except Exception:
            data = {}

    prompt = data.get('prompt', '').strip()

    if not prompt:
        return "ERRORE: campo 'prompt' mancante nel body JSON", 400

    print(f"[POST] Prompt ricevuto: {prompt}")

    try:
        risposta = _dispatch(prompt)
        print(f"[POST] Risposta ({INFERENCE_MODE}): {risposta}")
        return risposta
    except Exception as e:
        print(f"[POST] Errore: {e}")
        return f"ERRORE: {str(e)[:100]}", 500

#endregion

#region Main

# ---------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------
if __name__ == '__main__':
    print("=" * 50)
    print(f"  Proxy Apple IIe -> LLM")
    print(f"  Modalita': {INFERENCE_MODE.upper()}")
    print(f"  Modello : {MODEL_ID}")
    print(f"  Porta   : 8080")
    print(f"  Max char: {MAX_RESPONSE_CHARS}")
    print("=" * 50)
    print()
    print("Esempi di test da PC:")
    print("  GET : curl \"http://localhost:8080/api/data?prompt=ciao\"")
    print("  POST: curl -X POST http://localhost:8080/api/data \\")
    print("             -H \"Content-Type: application/json\" \\")
    print("             -d '{\"prompt\": \"chi era Steve Wozniak?\"}'")
    print()
    app.run(host='0.0.0.0', port=8080, debug=True)

#endregion
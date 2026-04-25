import os
import json
import boto3

from flask import Flask, request, jsonify
from dotenv import load_dotenv, find_dotenv

app = Flask(__name__)

path_env = find_dotenv()
load_dotenv(path_env)
print(f"File .env caricato da: {path_env}")

AWS_DEFAULT_REGION = os.getenv("AWS_DEFAULT_REGION")
LLM_MODEL_LOCAL = os.getenv("LLM_MODEL_LOCAL")
LLM_MODEL_CLOUD = os.getenv("LLM_MODEL_CLOUD")
INFERENCE_MODE = os.getenv("INFERENCE_MODE", "cloud").lower()

# ---------------------------------------------------------------
# Client AWS Bedrock
# Assicurati di avere le credenziali AWS configurate:
#   - variabili d'ambiente AWS_ACCESS_KEY_ID / AWS_SECRET_ACCESS_KEY
#   - oppure ~/.aws/credentials
#   - oppure IAM role se gira su EC2
# ---------------------------------------------------------------
bedrock = boto3.client(
    service_name="bedrock-runtime",
    region_name="us-east-1"         # cambia se usi una region diversa
)

if INFERENCE_MODE == "local":
    print(f"Modalità INFERENCE: LOCALE (modello {LLM_MODEL_LOCAL})")
    # Qui potresti inizializzare un client per Ollama o simili  
    MODEL_ID = LLM_MODEL_LOCAL
    OLLAMA_BASE_URL = os.getenv("OLLAMA_BASE_URL")
elif INFERENCE_MODE == "cloud":
    print(f"Modalità INFERENCE: CLOUD (modello {LLM_MODEL_CLOUD})")
    MODEL_ID = LLM_MODEL_CLOUD
else:
    raise ValueError(f"INFERENCE_MODE non valido: {INFERENCE_MODE}. Usa 'local' o 'cloud'.")
    os._exit(1)


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
    system_prompt = (
        "Sei un assistente virtuale che verrà utilizzato da un computer Apple IIe del 1983 ma oggi siamo nel 2026. "
        "ricordati che nel 1983 i sistemi di Intelligenza Artificiale non esistevano ancora"
        "prima di rispondere verifica le informazioni su internet e dai solo risposte concise, dirette e senza giri di parole. "
        "Rispondi in ITALIANO. Sii estremamente sintetico. "
        "La tua risposta NON deve superare i 240 caratteri. "
        "Non usare markdown (niente grassetto o tabelle)."
    )

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
        # Nessun prompt: risposta fissa di test
        return "HELLO FROM PROXY! Passa ?prompt=testo per chiamare Nova."

    print(f"[GET] Prompt ricevuto: {prompt}")

    try:
        risposta = call_nova(prompt)
        print(f"[GET] Risposta Nova: {risposta}")
        return risposta
    except Exception as e:
        print(f"[GET] Errore Bedrock: {e}")
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
        risposta = call_nova(prompt)
        print(f"[POST] Risposta Nova: {risposta}")
        return risposta
    except Exception as e:
        print(f"[POST] Errore Bedrock: {e}")
        return f"ERRORE: {str(e)[:100]}", 500

#endregion

#region Main

# ---------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------
if __name__ == '__main__':
    print("=" * 50)
    print(f"  Proxy Apple IIe -> Amazon Nova")
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
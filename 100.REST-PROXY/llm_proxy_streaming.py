import os
import json
import boto3
import requests

from flask import Flask, request, Response, stream_with_context
from dotenv import load_dotenv, find_dotenv

app = Flask(__name__)

# Flask usa HTTP/1.1 con Transfer-Encoding: chunked per le risposte streaming.
# Il client C in rest_lib_streaming.c include il parser chunked TE per gestirlo.
# NON forzare HTTP/1.0: Werkzeug bufferizzerebbe l'intera risposta prima di
# inviarla, rendendo inutile lo streaming.

path_env = find_dotenv()
load_dotenv(path_env)
print(f"File .env caricato da: {path_env}")

AWS_DEFAULT_REGION  = os.getenv("AWS_DEFAULT_REGION")
LLM_MODEL_CLOUD     = os.getenv("LLM_MODEL_CLOUD")
LLM_MODEL_ANTHROPIC = os.getenv("LLM_MODEL_ANTHROPIC", "anthropic.claude-3-haiku-20240307-v1:0")
LLM_MODEL_OLLAMA    = os.getenv("LLM_MODEL_OLLAMA", "llama4")
OLLAMA_BASE_URL     = os.getenv("OLLAMA_BASE_URL", "http://localhost:11434")
INFERENCE_MODE      = os.getenv("INFERENCE_MODE", "cloud").lower()

bedrock = boto3.client(
    service_name="bedrock-runtime",
    region_name=AWS_DEFAULT_REGION
)

system_prompt = (
    "Sei un assistente virtuale che verrà utilizzato da un computer Apple IIe del 1983 ma oggi siamo nel 2026. "
    "ricordati che nel 1983 i sistemi di Intelligenza Artificiale non esistevano ancora. "
    "prima di rispondere verifica le informazioni su internet e dai solo risposte concise, dirette e senza giri di parole. "
    "Rispondi in ITALIANO. "
    "Non usare markdown (niente grassetto, elenchi puntati o tabelle)."
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
    raise ValueError(f"INFERENCE_MODE non valido: '{INFERENCE_MODE}'. Usa 'cloud', 'anthropic' o 'ollama'.")

#region clean_for_apple2

def clean_for_apple2(text: str) -> str:
    replacements = {
        'à': "a'", 'á': "a'",
        'è': "e'", 'é': "e'",
        'ì': "i'", 'í': "i'",
        'ò': "o'", 'ó': "o'",
        'ù': "u'", 'ú': "u'",
        'È': "E'", 'À': "A'"
    }
    for char, replacement in replacements.items():
        text = text.replace(char, replacement)
    return "".join(c for c in text if ord(c) < 128)

#endregion

#region stream_ollama

def stream_ollama(prompt: str):
    payload = {
        "model": LLM_MODEL_OLLAMA,
        "messages": [
            {"role": "system", "content": system_prompt},
            {"role": "user",   "content": prompt}
        ],
        "stream": True,
        "options": {
            "num_predict": 512,
            "temperature": 0.4
        }
    }
    try:
        with requests.post(
            f"{OLLAMA_BASE_URL}/api/chat",
            json=payload,
            stream=True,
            timeout=120
        ) as r:
            r.raise_for_status()
            for line in r.iter_lines():
                if line:
                    data = json.loads(line)
                    token = data.get("message", {}).get("content", "")
                    if token:
                        yield clean_for_apple2(token).upper()
                    if data.get("done", False):
                        break
    except requests.exceptions.ConnectionError:
        print(f"[stream_ollama] Server non raggiungibile su {OLLAMA_BASE_URL}")
        yield "ERRORE: SERVER OLLAMA NON RAGGIUNGIBILE"
    except Exception as e:
        print(f"[stream_ollama] Errore: {e}")
        yield "ERRORE OLLAMA"

#endregion

#region stream_anthropic

def stream_anthropic(prompt: str):
    body = {
        "anthropic_version": "bedrock-2023-05-31",
        "max_tokens": 512,
        "temperature": 0.4,
        "system": system_prompt,
        "messages": [{"role": "user", "content": prompt}]
    }
    try:
        response = bedrock.invoke_model_with_response_stream(
            modelId=LLM_MODEL_ANTHROPIC,
            body=json.dumps(body),
            contentType="application/json",
            accept="application/json"
        )
        for event in response.get("body", []):
            chunk = event.get("chunk")
            if chunk:
                data = json.loads(chunk["bytes"])
                if data.get("type") == "content_block_delta":
                    text = data.get("delta", {}).get("text", "")
                    if text:
                        yield clean_for_apple2(text).upper()
    except Exception as e:
        print(f"[stream_anthropic] Errore: {e}")
        yield "ERRORE ANTHROPIC"

#endregion

#region stream_nova

def stream_nova(prompt: str):
    # Usa la Converse API che ha formato streaming standardizzato
    try:
        response = bedrock.converse_stream(
            modelId=MODEL_ID,
            messages=[{"role": "user", "content": [{"text": prompt}]}],
            system=[{"text": system_prompt}],
            inferenceConfig={"maxTokens": 512, "temperature": 0.4}
        )
        for event in response.get("stream", []):
            if "contentBlockDelta" in event:
                text = event["contentBlockDelta"].get("delta", {}).get("text", "")
                if text:
                    yield clean_for_apple2(text).upper()
    except Exception as e:
        print(f"[stream_nova] Errore: {e}")
        yield "ERRORE NOVA"

#endregion

#region _stream_dispatch

_MODEL_KEYWORDS = ("modello", "model", "chi sei", "who are you", "che llm", "quale ia", "quale ai")

def _stream_dispatch(prompt: str):
    if any(kw in prompt.lower() for kw in _MODEL_KEYWORDS):
        yield f"MODO:{INFERENCE_MODE.upper()} MODELLO:{MODEL_ID}"
        return

    if INFERENCE_MODE == "anthropic":
        yield from stream_anthropic(prompt)
    elif INFERENCE_MODE == "ollama":
        yield from stream_ollama(prompt)
    else:
        yield from stream_nova(prompt)

#endregion

#region get_data

# ---------------------------------------------------------------
# GET /api/data
#
# Uso da browser/curl:
#   curl "http://192.100.1.211:8080/api/data?prompt=ciao"
#
# Uso da Apple IIe:
#   do_get_stream("/api/data?prompt=ciao+come+stai")
# ---------------------------------------------------------------
@app.route('/api/data', methods=['GET'])
def get_data():
    prompt = request.args.get('prompt', '').strip()

    if not prompt:
        return f"HELLO FROM PROXY STREAMING! Modalita': {INFERENCE_MODE.upper()}. Passa ?prompt=testo per chiamare il modello."

    print(f"[GET] Prompt: {prompt}")

    def generate():
        for token in _stream_dispatch(prompt):
            yield token

    return Response(stream_with_context(generate()), mimetype='text/plain')

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
#   do_post_stream("/api/data", "{\"prompt\":\"chi era Steve Jobs?\"}")
# ---------------------------------------------------------------
@app.route('/api/data', methods=['POST'])
def post_data():
    if request.is_json:
        data = request.get_json(silent=True) or {}
    else:
        try:
            data = json.loads(request.data.decode('utf-8'))
        except Exception:
            data = {}

    prompt = data.get('prompt', '').strip()

    if not prompt:
        return "ERRORE: campo 'prompt' mancante nel body JSON", 400

    print(f"[POST] Prompt: {prompt}")

    def generate():
        for token in _stream_dispatch(prompt):
            yield token

    return Response(stream_with_context(generate()), mimetype='text/plain')

#endregion

#region Main

if __name__ == '__main__':
    print("=" * 50)
    print(f"  Proxy Apple IIe -> LLM  [STREAMING]")
    print(f"  Modalita': {INFERENCE_MODE.upper()}")
    print(f"  Modello : {MODEL_ID}")
    print(f"  Porta   : 8081")
    print("=" * 50)
    print()
    print("Esempi di test da PC:")
    print("  GET : curl \"http://localhost:8081/api/data?prompt=ciao\"")
    print("  POST: curl -X POST http://localhost:8081/api/data \\")
    print("             -H \"Content-Type: application/json\" \\")
    print("             -d '{\"prompt\": \"chi era Steve Wozniak?\"}'")
    print()
    # debug=False e threaded=True sono importanti per lo streaming
    app.run(host='0.0.0.0', port=8081, debug=False, threaded=True)

#endregion

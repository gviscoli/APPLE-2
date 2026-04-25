


curl -v --http1.0 http://192.100.1.211:8080/api/v1/status

curl -v --http1.0 http://192.100.1.211:8080/api/data


# OLLAMA

curl http://127.0.0.1:11434/api/chat -d '{
  "model": "llama3",
  "messages": [
    { "role": "user", "content": "Ciao!" }
  ],
  "stream": false
}'
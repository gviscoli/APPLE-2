from flask import Flask, request
import requests

# TEST
# 
# curl -v --http1.0 http://192.100.1.211:8080/api/v1/status
#

app = Flask(__name__)

@app.route('/api/data', methods=['GET'])
def proxy():
    # Qui il proxy chiama l'API HTTPS vera
    
    # response = requests.get('https://api.esempio.com/v1/resource')
    
    # response = requests.get('https://jsonplaceholder.typicode.com/todos/1')
    
    response = "HELLO WORLD DA UN PROXY MODERNO!"

    # Restituiamo solo il contenuto, possibilmente filtrato per l'Apple II
    # (Magari rimuovi i campi JSON inutili per risparmiare RAM sull'Apple)
    # return response.text[:256] 
    return response 

if __name__ == '__main__':
    # Ascolta su tutte le interfacce (0.0.0.0) sulla porta 8080
    print("Proxy attivo! In attesa della chiamata dall'Apple II...")
    app.run(host='0.0.0.0', port=8080)
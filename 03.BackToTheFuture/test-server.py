# import socket

# def start_server(host='192.100.1.211', port=6502):
#     server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
#     server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
#     server_socket.bind((host, port))
#     server_socket.listen(1)
    
#     print(f"Server in ascolto su {host}:{port}...")
    
#     while True:
#         client_socket, client_address = server_socket.accept()
#         print(f"Connessione accettata da {client_address}")
        
#         try:
#             while True:
#                 data = client_socket.recv(4096)
#                 if not data:
#                     break
#                 print(f"Ricevuto: {data}")
#                 client_socket.sendall(data)  # Echo dei dati ricevuti
#         except ConnectionResetError:
#             print("Connessione interrotta dal client")
#         finally:
#             client_socket.close()
#             print("Connessione chiusa")

# if __name__ == "__main__":
#     start_server()

import os, socket, sys, time

data_len = 100 * 1024
data = os.urandom(data_len)

serv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
serv.bind(('', 6502))
serv.listen(1)

while True:
    try:
        print("Test Ready\n")
        print("Listen On Port 6502\n")
        #print >>sys.stderr, 'Listen On Port 6502'
        conn, addr = serv.accept()

        print("Test Started")
        print(f"Connected To {addr}\n")
        conn.settimeout(0.001)

        send_pos = recv_pos = 0
        watchdog = int(time.time())
        while recv_pos < data_len:
            if send_pos < data_len:
                try:
                    sent = conn.send(data[send_pos:])
                except:
                    pass
                else:
                    print(f"Send Length: {sent}\n")
                    # print >>sys.stderr, 'Send Length:', sent
                    send_pos += sent;
                    watchdog = int(time.time())

            try:
                temp = conn.recv(0x1000)
            except:
                pass
            else:
                sys.stdout.write('.')
                sys.stdout.flush()
                received = len(temp)
                # print >>sys.stderr, 'Recv Length:', received
                print(f"Recv Length: {received}\n")
                assert temp == data[recv_pos:recv_pos+received]
                recv_pos += received;
                watchdog = int(time.time())

            assert int(time.time()) < watchdog+2

    except Exception as e:
        print("\n")
        print("Test Failure !!!\n")
        print(e)
    else:
        print("\n")
        print("Test Success")
    finally:
        conn.close()
        print("Disconnected")




# import os, socket, sys, time

# data_len = 100 * 1024
# data = os.urandom(data_len)

# serv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
# serv.bind(('', 6502))
# serv.listen(1)

# while True:
#     try:
#         print("Test Ready")
#         print >>sys.stderr, 'Listen On Port 6502'
#         conn, addr = serv.accept()

#         print("Test Started")
#         print >>sys.stderr, 'Connected To', addr
#         conn.settimeout(0.001)

#         send_pos = recv_pos = 0
#         watchdog = int(time.time())
#         while recv_pos < data_len:
#             if send_pos < data_len:
#                 try:
#                     sent = conn.send(data[send_pos:])
#                 except:
#                     pass
#                 else:
#                     print >>sys.stderr, 'Send Length:', sent
#                     send_pos += sent;
#                     watchdog = int(time.time())

#             try:
#                 temp = conn.recv(0x1000)
#             except:
#                 pass
#             else:
#                 sys.stdout.write('.')
#                 sys.stdout.flush()
#                 received = len(temp)
#                 print >>sys.stderr, 'Recv Length:', received
#                 assert temp == data[recv_pos:recv_pos+received]
#                 recv_pos += received;
#                 watchdog = int(time.time())

#             assert int(time.time()) < watchdog+2

#     except:
#         print ("/n")
#         print("Test Failure !!!")
#     else:
#         print ("/n")
#         print("Test Success")
        
#     # conn.close()
#     # print >>sys.stderr, 'Disconnected'



import socket
from time import sleep

HOST = "127.0.0.1"  # The server's hostname or IP address
PORT = 2137  # The port used by the server

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.connect((HOST, PORT))
    for i in range(0, 10): 
        s.sendall(b"Hello, world 1")
        data = s.recv(1024)
        print(f"Received {data!r}")
        sleep(0.1);

import socket
import datetime
import sys


def start_server():
    # Create TCP socket
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    # Allow reuse of address
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    # Bind to all interfaces on port 23456
    server_socket.bind(('', 23456))

    # Listen for connections
    server_socket.listen(1)
    print("Server listening on port 23456...")

    try:
        while True:
            # Accept client connection
            client_socket, client_address = server_socket.accept()
            print(f"Connection established from {client_address[0]}:{client_address[1]}")

            try:
                while True:
                    # Receive data from client
                    data = client_socket.recv(1024).decode('ascii')

                    if not data:
                        break

                    # Process command
                    if data.strip() == "TIME":
                        # Get current system time
                        current_time = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S.%f")
                        response = current_time
                    else:
                        response = "Invalid command."

                    # Send response back to client
                    client_socket.send(f"{response}\n".encode('ascii'))

            except socket.error:
                print("Client connection lost")
            finally:
                client_socket.close()
                print(f"Connection closed from {client_address[0]}:{client_address[1]}")

    except KeyboardInterrupt:
        print("\nShutting down server...")
    finally:
        server_socket.close()


if __name__ == "__main__":
    start_server()
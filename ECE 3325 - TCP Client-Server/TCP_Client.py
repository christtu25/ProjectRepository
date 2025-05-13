import socket
import sys

def start_client(server_ip):
    # Create TCP socket
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    try:
        # Connect to server
        print(f"Connecting to {server_ip}....")
        client_socket.connect((server_ip, 23456))
        print("\nConnected.")

        while True:
            # Get user input
            command = input("\nClient> ")

            # Check for quit command
            if command.strip() == "QUIT":
                print(f"\nClosing connection to {server_ip}")
                break

            # Send command to server
            client_socket.send(command.encode('ascii'))

            # Receive and display server response
            response = client_socket.recv(1024).decode('ascii')
            print(f"Server> {response.strip()}")

    except ConnectionRefusedError:
        print(f"Error: Could not connect to server at {server_ip}:23456")
        sys.exit(1)
    except socket.error as e:
        print(f"Error: {e}")
        sys.exit(1)
    finally:
        client_socket.close()
        print("Exiting program.")


if __name__ == "__main__":
    # Check command line arguments
    if len(sys.argv) != 2:
        print("Usage: python ECE3325_Project_1_client.py <server_ip>")
        sys.exit(1)

    server_ip = sys.argv[1]
    start_client(server_ip)
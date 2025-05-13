## Overview
Two main components:
- A TCP server that listens for connections and processes commands
- A TCP client that connects to the server and sends user commands

## Components
TCP Server:
- Listens on port 23456
- Accepts client connections
- Processes client commands
- Supports multiple sequential client connections
- Handles graceful disconnection
- Includes error handling and connection cleanup
TCP Client:
- Connects to specified server IP address
- Sends user commands to server
- Displays server responses
- Supports graceful disconnection with 'QUIT' command
- Includes robust error handling

## Features
- Command processing system
- Real-time server response
- Clean disconnection handling
- Error handling for network issues
- Support for timestamp requests
- Command-line interface

## Technical Details
- Protocol: TCP
- Default Port: 23456
- Encoding: ASCII
- Buffer Size: 1024 bytes
- Connection Type: Sequential (one client at a time)

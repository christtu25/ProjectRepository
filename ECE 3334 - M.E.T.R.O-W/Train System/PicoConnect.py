"""
Client for train on the Pico W.
This connects to the ad hoc network hosted by the Pi, then connects to the Pi server.
Now includes RSSI reporting functionality.
"""

import network
import socket
import machine
import ubinascii
import time
import asyncio
from time import sleep

# Pico ID (train1 or train2)
train = "train1"

# Our network name and password
ssid = 'track-master'
passwd = 'iliketrains'

# Server (Ras Pi) details
server_ip = '192.168.4.1'
server_port = 5000

# For toggling the Pico W LED
led = machine.Pin('LED', machine.Pin.OUT)
led.on()
# Set up PWM and Reverser Pins
PWMPin = machine.Pin(20)
PWMCycle = machine.PWM(PWMPin)
PWMCycle.freq(100)
PWMPin1 = machine.Pin(19)
PWMCycle1 = machine.PWM(PWMPin1)
PWMCycle1.freq(100)

# Define GPIO pins
I_pin = machine.Pin(1, machine.Pin.IN, machine.Pin.PULL_UP)  # D (Data) input, with internal pull-up resistor
O_pin = machine.Pin(4, machine.Pin.OUT)  # Q (Output) pin

# State variables

machine.freq(200_000_000)

# update
# RSSI reporting interval (seconds)
RSSI_INTERVAL = 1.0
last_rssi_time = time.time()

# update
def get_rssi():
    wlan = network.WLAN(network.STA_IF)
    if wlan.isconnected():
        return wlan.status('rssi')
    return None


# Connect the Pico W to the Pi 4 network
def connect_wifi():
    wlan = network.WLAN(network.STA_IF)
    wlan.active(True)

    if wlan.isconnected():
        wlan.disconnect()
        await asyncio.sleep(1)

    wlan.connect(ssid, passwd)

    retry = 0
    while wlan.isconnected() == False and retry < 25:
        print('Connecting...')
        await asyncio.sleep(1)
        retry += 1

    if not wlan.isconnected():
        print("Could not connect to WiFi")
        return None, None

    ip = wlan.ifconfig()[0]
    mac = ubinascii.hexlify(network.WLAN().config('mac'), ':').decode()
    print(f'Connected on {ip}')
    print(f'Mac Address {mac}')
    return ip, mac


# Connect the Pico W to the server
def connect_server(mac):
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        client_socket.connect((server_ip, server_port))
        client_socket.send(f"{train},{mac}".encode('utf-8'))
        print("Server connection established")
        return client_socket
    except OSError as e:
        print(f"Server connection failed: {e}")
        client_socket.close()
        return None

# Handle incoming data from the Pi 4
async def handle_commands(client_socket):
    global last_rssi_time # update
    while True:
        try:
            
            # Check for incoming commands
            command = client_socket.recv(1024).decode('utf-8')
            cmds = command.split()
            PWM = int(cmds[0])
            if cmds[1] == "0":
                PWMCycle.duty_u16(int((PWM / 10) * 65536))
                PWMCycle1.duty_u16(0)
            elif cmds[1] == "1":
                PWMCycle.duty_u16(0)
                PWMCycle1.duty_u16(0)
            elif cmds[1] == "2":
                PWMCycle.duty_u16(0)
                PWMCycle1.duty_u16(int((PWM / 10) * 65536))

            # update
            # Send RSSI updates periodically
            current_time = time.time()
            if current_time - last_rssi_time >= RSSI_INTERVAL:
                rssi = get_rssi()
                if rssi is not None:
                    rssi_msg = f"RSSI:{train}:{rssi}"
                    try:
                        client_socket.send(rssi_msg.encode('utf-8'))
                    except:
                        print("Failed to send RSSI")
                        return False
                last_rssi_time = current_time
            await asyncio.sleep(.01)
        except OSError as e:
            print(f"Command error: {e}")
            return False

async def socketloop():
    while True:
        try:
            # Connect to the WiFi network
            wlan = network.WLAN(network.STA_IF)
            wlan.active(True)

            if wlan.isconnected():
                wlan.disconnect()
                await asyncio.sleep(1)

            wlan.connect(ssid, passwd)

            retry = 0
            while wlan.isconnected() == False and retry < 25:
                print('Connecting...')
                await asyncio.sleep(1)
                retry += 1

            if not wlan.isconnected():
                print("Could not connect to WiFi")
                ip = None
                mac = None

            ip = wlan.ifconfig()[0]
            mac = ubinascii.hexlify(network.WLAN().config('mac'), ':').decode()
            if not ip or not mac:
                print("WiFi connection error. Retry in 5...")
                await asyncio.sleep(5)
                continue

            # Then the server
            client_socket = connect_server(mac)
            if not client_socket:
                print("Server connection error. Retry in 5...")
                await asyncio.sleep(5)
                continue

            # If all fails, close the client socket and try again
            stat = asyncio.run(handle_commands(client_socket))
            if not stat:
                print("Connection lost. Disconnecting...")
                try:
                    client_socket.close()
                except:
                    pass
                print("Reconnecting in 5 seconds...")
                await asyncio.sleep(5)
                
        except Exception as e:
            print(f"Main error: {e}")
            await asyncio.sleep(5)
            
async def rotaryloop():
    last_I_value = 0  # Initial value of D (assuming pull-up resistor on D_pin)
    Q_value = 0  # Initial value of Q
    counter = 0
    current_I_value =0
    TimerCount = 0
    speed = 0
    while True:
        current_I_value = I_pin.value()
    
        if current_I_value  != last_I_value:  # This is the edge detection logic
            counter = counter + 1
             
        TimerCount = TimerCount + 1
        # Store current D value for the next cycle to detect edge change
        last_I_value = current_I_value
        if TimerCount == 10:
            speed = (1/36) * counter * 5.65487 * 2
            print("Speed:", speed, " cm/s")
            TimerCount = 0
            counter = 0
        await asyncio.sleep(0.1)  # Keep the main loop running while the timer triggers the flip-flop
    

# Main
async def main():
    await asyncio.gather(socketloop(), rotaryloop())
    print("end")



#try:
asyncio.run(main())
#except KeyboardInterrupt:
    #machine.reset()
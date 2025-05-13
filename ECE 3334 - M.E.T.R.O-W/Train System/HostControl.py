"""
Runs a server on an opened socket to interact with multiple trains over a P2P ad hoc network.
Communication protocol is TCP, to ensure the correct train is contacted.
Now includes RSSI monitoring and distance estimation.
"""

import socket
import threading
import sys
import signal
import RPi.GPIO as GPIO
import cv2 as cv
import pigpio
from time import sleep
from collections import deque
from mfrc522 import SimpleMFRC522


#Global Variables
depot1 = 0
depot2 = 0
servoturn = 0
#Configure Servo Settings
servo = 12
pwm = pigpio.pi()
pwm.set_mode(servo, pigpio.OUTPUT)
pwm.set_PWM_frequency(servo, 50)

def empty(a):
    pass

class Server:
    def __init__(self, host, port):
        self.host = host
        self.port = port
        self.clients = {}
        self.running = True
        self.server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.server_socket.bind((self.host, self.port))

        # RSSI tracking
        self.rssi_values = {'train1': deque(maxlen=10), 'train2': deque(maxlen=10)}
        self.last_rssi = {'train1': None, 'train2': None}

        # Emergency stop parameters
        self.SAFE_DISTANCE = .1  # meters
        self.WARNING_DISTANCE = 0.3  # meters
        self.emergency_stop_active = False
        self.warning_active = False
        
        # Create simple RSSI display window
       # cv.namedWindow("RSSI Values")
      #  cv.createTrackbar("Placeholder", "RSSI Values", 0, 1, empty)  # Needed to create window

    def estimate_distance(self, rssi):
        if rssi is None:
            return None
        MEASURED_POWER = -50  # RSSI at 1 meter (needs calibration)
        PATH_LOSS_EXPONENT = 4.5
        try:
            distance = 10 ** ((abs(MEASURED_POWER) - abs(rssi)) / (10 * PATH_LOSS_EXPONENT))
            return min(distance, 100)
        except:
            return None
            
    def emergency_stop(self): 
        """Initiate emergency stop for all trains"""
        if not self.emergency_stop_active:
            print("EMERGENCY STOP ACTIVATED")
            self.emergency_stop_active = True
            # Sending stop command to all trains
            stop_cmd = "0 1"  # Zero speed, whatever direction
            self.send_command('train1', stop_cmd)
            self.send_command('train2', stop_cmd)

            # Reset GUI controls
            cv.setTrackbarPos("Train Speed", "Train 1", 0)
            cv.setTrackbarPos("Train Speed", "Train 2", 0)
            cv.setTrackbarPos("Train Reverser", "Train 1", 1)
            cv.setTrackbarPos("Train Reverser", "Train 2", 1)

    def check_safety_distance(self, distance):
        """Check if trains are too close and manage emergency stop state"""
        if distance is not None:
            if distance < self.SAFE_DISTANCE:
                self.emergency_stop()
                self.warning_active = True
            elif distance < self.WARNING_DISTANCE:
                self.warning_active = True
            else:
                self.warning_active = False
                self.emergency_stop_active = False
    def update_rssi_display(self):
        while self.running:
            try:
                # Get distances
                dist1 = self.estimate_distance(self.last_rssi.get('train1'))
                dist2 = self.estimate_distance(self.last_rssi.get('train2'))

                # Format display strings
                t1_status = f"Train 1: No Signal"
                t2_status = f"Train 2: No Signal"
                dist_status = "Distance between trains: N/A"
                safety_status = "Status: Normal"
                
                if self.last_rssi.get('train1') is not None:
                    t1_status = f"Train 1: RSSI: {self.last_rssi['train1']} dBm"
                    if dist1 is not None:
                        t1_status += f" (≈{dist1:.1f}m)"

                if self.last_rssi.get('train2') is not None:
                    t2_status = f"Train 2: RSSI: {self.last_rssi['train2']} dBm"
                    if dist2 is not None:
                        t2_status += f" (≈{dist2:.1f}m)"

                if dist1 is not None and dist2 is not None:
                    train_distance = abs(dist1 - dist2)
                    dist_status = f"Distance between trains: {train_distance:.1f}m"
                    
                    # Check safety distance and update status
                    self.check_safety_distance(train_distance)

                    if self.emergency_stop_active:
                        safety_status = "Status: EMERGENCY STOP ACTIVE"
                        status_color = (0, 0, 255)  # Red
                    elif self.warning_active:
                        safety_status = "Status: WARNING - Trains Too Close"
                        status_color = (0, 165, 255)  # Orange
                    else:
                        safety_status = "Status: Normal Operation"
                        status_color = (0, 255, 0)  # Green
                # Print to console (can be removed if not needed)
                print("\n" + t1_status)
                print(t2_status)
                print(dist_status + "\n")

                sleep(1)  # Update every second

            except Exception as e:
                print(f"Display error: {e}")
                sleep(1)

    def start(self):
        self.server_socket.listen(5)

        # Start RSSI display thread
        display_thread = threading.Thread(target=self.update_rssi_display)
        display_thread.daemon = True
        display_thread.start()

        while self.running:
            try:
                self.server_socket.settimeout(1.0)
                try:
                    client_socket, addr = self.server_socket.accept()
                    client_socket.settimeout(5.0)
                    print(f"New connection from {addr}")
                    client_thread = threading.Thread(target=self.handle_client, args=(client_socket,))
                    client_thread.daemon = True
                    client_thread.start()
                except socket.timeout:
                    continue
            except Exception as e:
                print(f"Accept error: {e}")
                if not self.running:
                    break

    def handle_client(self, client_socket):
        try:
            data = client_socket.recv(1024).decode('utf-8')
            if not data:
                return
                
            train, mac = data.split(',')
            self.clients[train] = client_socket
            print(f"Client {train} connected (MAC: {mac})")

            client_socket.settimeout(None)

            while True:
                try:
                    data = client_socket.recv(1024).decode('utf-8')
                    if not data:
                        break

                    # Handle RSSI updates
                    if data.startswith('RSSI:'):
                        _, train_id, rssi_val = data.split(':')
                        rssi_val = int(rssi_val)
                        self.rssi_values[train_id].append(rssi_val)
                        self.last_rssi[train_id] = rssi_val
                    else:
                        print(f"Received from client {train}: {data}")
                except:
                    break

        finally:
            if train in self.clients:
                del self.clients[train]
            client_socket.close()
            print(f"Client {train} disconnected")

    def send_command(self, target, command):
        if target == 'all':
            for client_id, client_socket in self.clients.items():
                try:
                    client_socket.send(command.encode('utf-8'))
                    print(f"Sent '{command}' to client {client_id}")
                except:
                    print(f"Failed to send to client {client_id}")
        elif target in self.clients:
            try:
                self.clients[target].send(command.encode('utf-8'))
                print(f"Sent '{command}' to client {target}")
            except:
                print(f"Failed to send to client {target}")
        else:
            print(f"Client {target} not found")
            x = 0

    def cleanup_client(self, client_id, client_socket):
        try:
            client_socket.shutdown(socket.SHUT_RDWR)
            client_socket.close()
        except:
            pass
        if client_id in self.clients:
            del self.clients[client_id]

    def close(self):
        self.running = False
        for client_id, client_socket in list(self.clients.items()):
            self.cleanup_client(client_id, client_socket)
        try:
            self.server_socket.shutdown(socket.SHUT_RDWR)
            self.server_socket.close()
        except:
            pass
        cv.destroyAllWindows()
        print("Server shut down")
        sys.exit(0)

    def connections(self):
        print("Connected clients: ", list(self.clients.keys()))


def signal_handler(signum, frame):
    print("Shutting down server...")
    server.close()

#Reader forever loop
def rfidread():
    reader = SimpleMFRC522()
    global servoturn
    while True:
        #Code gets stuck here whenever no read- waits until read
        id, text = reader.read()
        if 697045877447 == id and depot1 == 1:
            servoturn = 1
            print(servoturn)
        elif 222049731248 == id and depot2 == 1:
            servoturn = 1
            print("ServoTurn")
        else:
            servoturn = 0
            print("Disable")
        print(id)
        print(text)


# Main
if __name__ == "__main__":
    # Open the server here
    server = Server('0.0.0.0', 5000)

    # Register the shutdown signals
    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)

    # Begin server thread
    server_thread = threading.Thread(target=server.start)
    server_thread.daemon = True
    server_thread.start()

    # Create Train Control GUI
    cv.namedWindow("Train 1")  # Create Trackbar to control speed
    cv.createTrackbar("Train Speed", "Train 1", 0, 10, empty)
    cv.createTrackbar("Train Reverser", "Train 1", 1, 2, empty)
    cv.createTrackbar("Servo Position", "Train 1", 0, 1, empty)

    # Create Train Control GUI
    cv.namedWindow("Train 2")  # Create Trackbar to control speed
    cv.createTrackbar("Train Speed", "Train 2", 0, 10, empty)
    cv.createTrackbar("Train Reverser", "Train 2", 1, 2, empty)

    #Setup Reader GPIO thread
    reader_thread = threading.Thread(target=rfidread)
    reader_thread.daemon = True
    reader_thread.start()
    
    #Configure IR Sensor Settings
    IR = 11
    GPIO.setup(IR, GPIO.IN)
    
    #Configure Button Press
    GPIO.setup(13, GPIO.IN, pull_up_down=GPIO.PUD_UP)
    GPIO.setup(15, GPIO.IN, pull_up_down=GPIO.PUD_UP)
    # Main loop
    while True:
        PWMCyc = str(cv.getTrackbarPos("Train Speed", "Train 1"))
        Reverser = str(cv.getTrackbarPos("Train Reverser", "Train 1"))
        PWMCyc1 = str(cv.getTrackbarPos("Train Speed", "Train 2"))
        Reverser1 = str(cv.getTrackbarPos("Train Reverser", "Train 2"))
        ServoPos = cv.getTrackbarPos("Servo Position", "Train 1")

        cmd = PWMCyc + " " + Reverser
        cmd1 = PWMCyc1 + " " + Reverser1
        sleep(.25)
        server.send_command("train1", cmd)
        server.send_command("train2", cmd1)
        #Turn Servo
        if servoturn == 1:
            pwm.set_servo_pulsewidth(servo, 1350)
            print("Servo Turn")
        else:
            if ServoPos == 0:
                pwm.set_servo_pulsewidth(servo, 1700)
            elif ServoPos == 1:
                pwm.set_servo_pulsewidth(servo, 1350)
        #Check IR Sensor
        if GPIO.input(IR) == 0:
            if depot1 == 1:
                cv.setTrackbarPos("Train Speed", "Train 1", 0)
            if depot2 == 1:
                cv.setTrackbarPos("Train Speed", "Train 2", 0)
            depot1 = 0
            depot2 = 0
        #Check button press
        if GPIO.input(13) == GPIO.LOW:
            depot1 = 1
            cv.setTrackbarPos("Train Speed", "Train 1", 10)
            cv.setTrackbarPos("Train Speed", "Train 2", 10)
        if GPIO.input(15) == GPIO.LOW:
            depot2 = 1
            cv.setTrackbarPos("Train Speed", "Train 1", 10)
            cv.setTrackbarPos("Train Speed", "Train 2", 10)
        # Add proper exit condition
        key = cv.waitKey(1) & 0xFF
        if key == 27:  # ESC key
            server.close()
            break

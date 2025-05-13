## METRO-W: Wireless Model Railway Control System
METRO-W (Managing Efficient Transportation Rail Operations Wirelessly) is an innovative approach to model railway control that uses wireless communication and microcontroller automation to provide an affordable alternative to traditional DCC systems.

## Key Features
- Wireless Control: Uses TCP/IP communication between a Raspberry Pi 4 central controller and Pi Pico W locomotives
- Real-time Monitoring: Speed monitoring via rotary encoders with real-time velocity feedback
- Automated Safety: RSSI-based distance monitoring with automated emergency stops when trains get too close
- Track Automation: RFID-based track switching with servo-controlled turnouts
- Dual GUI Control: Independent control interfaces for multiple locomotives using OpenCV
- Custom Hardware: 3D-printed track sections compatible with standard track infrastructure
- Battery Operation: No track power required, operates on rechargeable Li-ion batteries

## Hardware Components
- Raspberry Pi 4 Model B (Central Controller)
- Raspberry Pi Pico W (Locomotive Control)
- MFRC522 RFID Reader
- HW-201 IR Sensor
- MG995 Servo Motor
- L9110H H-Bridge Motor Drivers
- Rotary Encoders
- Custom 3D-Printed Track Sections

## Software Architecture
- Python/MicroPython for controller logic
- TCP/IP for wireless communication
- OpenCV for GUI implementation
- PWM control for motors and servos
- Asyncio for concurrent operations
- RSSI calculations for distance estimation

## Key Technical Specifications
- Network Protocol: TCP/IP over 2.4 GHz WiFi
- PWM Frequency: 100 Hz for motors, 50 Hz for servos
- RFID Frequency: 13.56 MHz
- Control Latency: <100ms
- Emergency Stop Response: <200ms
- Detection Range: RFID (3cm), IR (2-30cm)
- Power Requirements: 9V for motors, 5V for servos
- Distance Monitoring Precision: ±0.2 meters

## Performance Metrics
- Packet Delivery Success Rate: >98%
- RFID Detection Reliability: >95%
- Speed Control: 10 discrete levels
- Maximum Operating Range: 10 meters
- Network Reconnection Time: <5 seconds
- Servo Response Time: 200ms

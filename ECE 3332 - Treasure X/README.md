## Project Overview
Treasure X is a portable microcontroller-based system that uses Bluetooth Low Energy (BLE) technology and GPS tracking to create an interactive treasure hunting experience. Users locate Bluetooth beacons scattered across an area, answer trivia questions to confirm their discoveries, and ultimately reveal a hidden "treasure" location determined by the geometric median of the beacon coordinates.

## Key Features
- BLE beacon detection with real-time proximity feedback
- GPS coordinate tracking and mapping
- Interactive trivia game system
- Live feedback on user proximity to beacons
- Google Maps integration for visualization
- Portable, battery-powered design
- Custom 3D-printed enclosure

## Hardware Components
- Raspberry Pi 4 Model B
- SIM7600A-H 4G/GPS module
- LTC4056 power module
- HS-007 touch display
- 12V 5.2A rechargeable battery
- Custom 3D-printed enclosure
- 8x iBKS 105 Bluetooth beacons

## Software Components
- Python-based system architecture
- Bluetooth scanning and RSSI measurement
- GPS data retrieval and conversion
- Google Maps integration
- Interactive GUI using Tkinter
- Geometric median calculation for treasure location

## Usage
1. Power on the system
2. Start the treasure hunting application
3. Follow the on-screen prompts to locate beacons
4. Answer trivia questions to confirm beacon locations
5. Once all beacons are found, the treasure location will be revealed on Google Maps

## Software Architecture
- main.py: Main application and GUI implementation
- BLEScanner.py: Bluetooth scanning and RSSI measurement
- readGPS.py: GPS data retrieval and processing
- centerplot.py: Geometric median calculation and mapping

## Technical Details
- Operating time: 6-9 hours on battery
- Detection range: Up to -75 RSSI
- GPS accuracy: Standard GPS precision
- Display: Touch-enabled interface
- Connectivity: 4G LTE + Bluetooth Low Energy

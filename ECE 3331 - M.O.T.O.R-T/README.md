## Project Overview
The M.O.T.O.R-T (Material Operation Transport Organized Robotic-Transport) initiative is an autonomous robotic system designed to streamline material transportation and processing in foundry environments. The robot navigates along metallic tracks to transport items between distinct processing zones, integrating advanced sensing and automation capabilities.

## Key Features
- Autonomous Navigation: Uses inductive proximity sensors for precise track following
- Temperature Sensing: Implements ZTP-135SR Thermopile for real-time temperature monitoring
- Station Detection: Utilizes HC-SR04 ultrasonic sensors for station identification
- Material Handling: Features electromagnetic system for washer pickup and placement
- Safety Systems: Includes overcurrent protection and emergency stop functionality
- Real-time Monitoring: 7-segment display shows temperature and system status

## Hardware Components
- Basys-3 FPGA Development Board
- H-Bridge Motor Controller
- LM339N Comparator Circuit
- Three Inductive Proximity Sensors
- HC-SR04 Ultrasonic Sensor
- ZTP-135SR Thermopile
- Custom 3D Printed Components
- Rover 5 Chassis Platform

## Software (Verilog) Implementation
- PWM Generation for motor control
- Temperature sensing and processing
- Station detection logic
- Material handling control
- Safety monitoring systems

## Safety Features
- Hardware current protection system
- Software-based overcurrent monitoring
- Emergency stop functionality
- Real-time status monitoring
- Thermal protection systems

## MSP432 Embedded Command Interface

This project implements a command-line interface for the MSP432E401Y microcontroller, featuring extensive GPIO control, timer management, callback systems, register operations, and audio generation capabilities. The system provides a robust terminal interface with command history navigation and comprehensive error handling.

## Core System Features
- UART0 for primary terminal interface (115200 baud)
- UART7 for secondary communication channel
- Binary data mode support
- Command history with up/down arrow navigation

## GPIO Control
- Control of 4 onboard LEDs
- 2 switch inputs with interrupt capability
- Additional GPIO pins (PK5, PD4) for extended functionality
- Read, write, and toggle operations
- Real-time status monitoring

## Timer System
- Configurable periodic timer with millisecond precision
- Minimum period of 5ms
- Continuous callback mode support
- Timer-based event triggering

## Callback System
- Independent callback channels
- Configurable execution count (-1 for infinite)
- Queue-based payload system
- Overflow protection

## Ticker System
- 16 independent ticker channels
- Configurable initial delay and period
- Support for periodic command execution
- Count-based or infinite execution modes

## Register Operations
- 32 general-purpose registers
- Comprehensive arithmetic operations
- Immediate value support
- Register status monitoring

## Script System
- 64-line script storage
- Command sequencing capability
- Conditional execution support
- Script execution controls
- Remark support for documentation

## Conditional Executions
- IF-THEN-ELSE style command structure
- Comparison operations (>, =, <)
- Support for register and immediate values
- Nested condition capability

## Audio Generation
- Sine wave generation through DAC
- Configurable frequency output
- SPI-based DAC control (DAC8311)
- Audio amplifier control
- Nyquist limit protection

## Error Handling
- Comprehensive error tracking for all parsable commands.

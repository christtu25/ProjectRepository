from adafruit_servokit import ServoKit
import time

# Initialize the PCA9685 with 16 channels
print("Initializing PCA9685...")
kit = ServoKit(channels=16)
# kit = ServoKit(channels=16, address=0x40, i2c_bus=0)  # Explicitly set bus 0, if required

# Configure servo channels
PAN_SERVO = 0    # Pan servo on channel 0
TILT_SERVO = 1   # Tilt servo on channel 1

# Configure servo parameters for DS3218MG (270° servo)
# Center is at 135° (half of 270°)
CENTER_ANGLE = 135

def init_servos():
    """Initialize both servos to center position"""
    print("Moving servos to center position...")
    kit.servo[PAN_SERVO].angle = CENTER_ANGLE
    kit.servo[TILT_SERVO].angle = CENTER_ANGLE
    time.sleep(2)  # Give servos time to reach position

def move_servo(channel, angle, current_angle):
    """Move a servo smoothly to target angle"""
    # Determine direction and step through angles
    step = 1 if angle > current_angle else -1
    for a in range(int(current_angle), int(angle), step):
        kit.servo[channel].angle = a
        time.sleep(0.01)  # Small delay for smooth movement
    kit.servo[channel].angle = angle  # Final position
    return angle

def main():
    try:
        # Initialize servos to center
        init_servos()
        current_pan = CENTER_ANGLE
        current_tilt = CENTER_ANGLE
        
        while True:
            print("\nServo Test Menu:")
            print("1. Test Pan Servo")
            print("2. Test Tilt Servo")
            print("3. Center Both Servos")
            print("4. Exit")
            
            choice = input("Select option (1-4): ")
            
            if choice == '1':
                angle = float(input("Enter pan angle (0-270): "))
                if 0 <= angle <= 270:
                    print(f"Moving pan servo to {angle}°")
                    current_pan = move_servo(PAN_SERVO, angle, current_pan)
                else:
                    print("Invalid angle! Must be between 0 and 270")
                    
            elif choice == '2':
                angle = float(input("Enter tilt angle (0-270): "))
                if 0 <= angle <= 270:
                    print(f"Moving tilt servo to {angle}°")
                    current_tilt = move_servo(TILT_SERVO, angle, current_tilt)
                else:
                    print("Invalid angle! Must be between 0 and 270")
                    
            elif choice == '3':
                print("Centering both servos...")
                current_pan = move_servo(PAN_SERVO, CENTER_ANGLE, current_pan)
                current_tilt = move_servo(TILT_SERVO, CENTER_ANGLE, current_tilt)
                
            elif choice == '4':
                print("Exiting... Moving servos to center.")
                init_servos()
                break
                
            else:
                print("Invalid choice! Please select 1-4")
                
    except KeyboardInterrupt:
        print("\nEmergency stop! Centering servos...")
        init_servos()
    
    except Exception as e:
        print(f"An error occurred: {str(e)}")
        print("Attempting to center servos...")
        init_servos()

if __name__ == "__main__":
    main()

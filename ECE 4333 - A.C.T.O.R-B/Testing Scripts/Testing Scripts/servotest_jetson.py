from adafruit_servokit import ServoKit
import time

print("Initializing PCA9685...")
kit = ServoKit(channels=16)  # Default I2C bus 1, address 0x40

PAN_SERVO = 0
TILT_SERVO = 1
CENTER_ANGLE = 135

def init_servos():
    print("Moving servos to center position...")
    kit.servo[PAN_SERVO].angle = CENTER_ANGLE
    kit.servo[TILT_SERVO].angle = CENTER_ANGLE
    time.sleep(2)

def move_servo(channel, angle, current_angle):
    step = 1 if angle > current_angle else -1
    for a in range(int(current_angle), int(angle), step):
        kit.servo[channel].angle = a
        time.sleep(0.01)
    kit.servo[channel].angle = angle
    return angle

def main():
    try:
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
        init_servos()

if __name__ == "__main__":
    main()

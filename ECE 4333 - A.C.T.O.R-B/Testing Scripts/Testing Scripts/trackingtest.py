from picamera2 import Picamera2
from adafruit_servokit import ServoKit
from ultralytics import YOLO
import cv2
import time
import numpy as np

# Initialize camera
picam2 = Picamera2()
picam2.preview_configuration.main.size = (640, 480)
picam2.preview_configuration.main.format = "RGB888"
picam2.configure("preview")

# Initialize servo controller
kit = ServoKit(channels=16)
PAN_SERVO = 0
TILT_SERVO = 1

# Initialize YOLO model
model = YOLO('yolov8n.pt')

# Servo parameters
CENTER_ANGLE = 135  # Center position (270° servo)
current_pan = CENTER_ANGLE
current_tilt = CENTER_ANGLE

# Movement parameters
MOVEMENT_THRESHOLD = 30  # Minimum pixel distance from center to move
MAX_SPEED = 2.0  # Maximum degrees to move per frame
P_GAIN = 0.1  # Proportional gain for smooth movement

def init_servos():
    """Initialize servos to center position"""
    global current_pan, current_tilt
    print("Centering servos...")
    kit.servo[PAN_SERVO].angle = CENTER_ANGLE
    kit.servo[TILT_SERVO].angle = CENTER_ANGLE
    current_pan = CENTER_ANGLE
    current_tilt = CENTER_ANGLE
    time.sleep(2)

def move_servo_smooth(channel, target_angle, current_angle):
    """Move servo smoothly towards target angle"""
    # Calculate distance to move
    diff = target_angle - current_angle
    
    # Apply speed limit
    if abs(diff) > MAX_SPEED:
        if diff > 0:
            new_angle = current_angle + MAX_SPEED
        else:
            new_angle = current_angle - MAX_SPEED
    else:
        new_angle = target_angle
    
    # Ensure angle is within bounds
    new_angle = max(0, min(270, new_angle))
    
    # Move servo
    kit.servo[channel].angle = new_angle
    return new_angle

def track_person():
    global current_pan, current_tilt
    
    try:
        init_servos()
        print("Starting tracking system...")
        
        while True:
            # Capture frame
            frame = picam2.capture_array()
            height, width = frame.shape[:2]
            center_x = width // 2
            center_y = height // 2
            
            # Run detection
            results = model(frame, conf=0.5)
            
            person_detected = False
            for result in results:
                boxes = result.boxes
                for box in boxes:
                    # Only track persons (class 0 in COCO dataset)
                    if box.cls == 0:
                        person_detected = True
                        # Get coordinates
                        x1, y1, x2, y2 = map(int, box.xyxy[0])
                        
                        # Calculate center of detected person
                        person_x = (x1 + x2) // 2
                        person_y = (y1 + y2) // 2
                        
                        # Draw detection box and center point
                        cv2.rectangle(frame, (x1, y1), (x2, y2), (0, 255, 0), 2)
                        cv2.circle(frame, (person_x, person_y), 5, (0, 0, 255), -1)
                        
                        # Calculate offset from center
                        offset_x = person_x - center_x
                        offset_y = person_y - center_y
                        
                        # Only move if offset is larger than threshold
                        if abs(offset_x) > MOVEMENT_THRESHOLD:
                            # Pan movement (negative offset_x means move right)
                            pan_target = current_pan - (offset_x * P_GAIN)
                            current_pan = move_servo_smooth(PAN_SERVO, pan_target, current_pan)
                            
                        if abs(offset_y) > MOVEMENT_THRESHOLD:
                            # Tilt movement (negative offset_y means move up)
                            tilt_target = current_tilt + (offset_y * P_GAIN)
                            current_tilt = move_servo_smooth(TILT_SERVO, tilt_target, current_tilt)
                        
                        # Draw tracking info
                        cv2.putText(frame, f"X Offset: {offset_x}", (10, 30), 
                                  cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2)
                        cv2.putText(frame, f"Y Offset: {offset_y}", (10, 60), 
                                  cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2)
                        
                        # Only track first person detected
                        break
            
            if not person_detected:
                cv2.putText(frame, "No person detected", (10, 30), 
                           cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 0, 255), 2)
            
            # Display frame
            cv2.imshow("Person Tracking", frame)
            
            # Break loop on 'q' press
            if cv2.waitKey(1) & 0xFF == ord('q'):
                break
                
    except KeyboardInterrupt:
        print("\nEmergency stop!")
    except Exception as e:
        print(f"An error occurred: {str(e)}")
    finally:
        print("Cleaning up...")
        init_servos()  # Return to center
        cv2.destroyAllWindows()
        picam2.stop()

if __name__ == "__main__":
    picam2.start()
    track_person()

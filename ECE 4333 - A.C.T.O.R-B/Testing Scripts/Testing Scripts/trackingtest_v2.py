	# Declaring libraries in use
from picamera2 import Picamera2
from adafruit_servokit import ServoKit
from ultralytics import YOLO
import cv2
import time
import numpy as np

	# Initialize camera
picam2 = Picamera2()											# Declaring PiCam2 for RPi
picam2.preview_configuration.main.size = (640, 480) 							# Setting resolution
picam2.preview_configuration.main.format = "RGB888" 							# 8-bit RGB for CV compatibility
picam2.configure("preview") 										# Applying camera configurations

	# Initialize servo controller
kit = ServoKit(channels=16) 										# Initializing servo house (16 channels)
PAN_SERVO = 0 												# Pan servo as channel 0
TILT_SERVO = 1 												# Tilt servo as channel 1

	# Initialize YOLO model
model = YOLO('yolov8n.pt') 										# Nano version for faster processing

	# Servo parameters
CENTER_ANGLE = 135  											# Center position (270° servo)
current_pan = CENTER_ANGLE 										# Starting pan angle at center
current_tilt = CENTER_ANGLE 										# Starting tilt angle at center

	# Movement parameters
MOVEMENT_THRESHOLD = 30  										# Minimum pixel distance from center to move
MAX_SPEED = 2.0  											# Maximum degrees to move per frame
P_GAIN = 0.1  												# Proportional gain for smooth movement

	# Initializing function for servos upon boot
def init_servos():
    """Initialize servos to center position"""
    global current_pan, current_tilt 									# Access global positional variables for servo
    print("Centering servos...") 									# Terminal prompt
    kit.servo[PAN_SERVO].angle = CENTER_ANGLE 								# Setting pan to center
    kit.servo[TILT_SERVO].angle = CENTER_ANGLE 								# Setting tilt to center
    current_pan = CENTER_ANGLE 										# Updating current position pan variable
    current_tilt = CENTER_ANGLE 									# Updating current position tilt variable
    time.sleep(2) 											# Short halt to ensuring servos reach endpoint

	# Smoothing function for servo tracking 
def move_servo_smooth(channel, target_angle, current_angle):
    """Move servo smoothly towards target angle"""
    diff = target_angle - current_angle 								# Calculating difference between target and current angle
    if abs(diff) > MAX_SPEED: 										# Applying speed limit (degrees to move greater than angle difference)
        if diff > 0: 											# If positive, move degrees forward
            new_angle = current_angle + MAX_SPEED
        else: 												# If negative, move degrees backward
            new_angle = current_angle - MAX_SPEED	
    else: 												# If difference is small or same, move degrees to target
        new_angle = target_angle
    new_angle = max(0, min(270, new_angle)) 								# Constrain angle range (should update to appropriately signal when any angle at 270)
    kit.servo[channel].angle = new_angle 								# Move servo to new angle
    return new_angle 											# Return new angle for servo tracking position

	# Tracking function to detect person using camera/servos
def track_person():
    """Main function to detect and track a person using camera and servos"""
    global current_pan, current_tilt 									# Access global positional variables for servo
    try:
        init_servos() 											# Center servos upon boot
        print("Starting tracking system...") 								# Terminal prompt
        while True: # Declaring main loop
            frame = picam2.capture_array() 								# Capture frame in numpy array
            height, width = frame.shape[:2] 								# Get frame dimensions
            center_x = width // 2 									# Calculate center coordinates from x (flow division)
            center_y = height // 2 									# Calculate center coordinates from y (flow division)
            results = model(frame, conf=0.5) 								# Run detection model with threshold 50%
            person_detected = False 									# Tracking flag if at detected state
            for result in results: 									# Iterate results
                boxes = result.boxes 									# Declaring bounding boxes
                for box in boxes: 									# Process each object detected
                    if box.cls == 0: 									# Only track persons (class 0 in COCO dataset)
                        person_detected = True 								# If camera detected a person
                        x1, y1, x2, y2 = map(int, box.xyxy[0]) 						# Extract bounding box coordinates as int       
                        person_x = (x1 + x2) // 2 							# Calculate center of x bounding box
                        person_y = (y1 + y2) // 2 							# Calculate center of y bounding box        
                        cv2.rectangle(frame, (x1, y1), (x2, y2), (0, 255, 0), 2) 			# Drawing green rectangle around detection coordinates
                        cv2.circle(frame, (person_x, person_y), 5, (0, 0, 255), -1) 			# Drawing red dot at detection box center                        
                        offset_x = person_x - center_x 							# Calculate offset x from center
                        offset_y = person_y - center_y 							# Calculate offset y from center
                        if abs(offset_x) > MOVEMENT_THRESHOLD: 						# Only move if offset x is larger than threshold
                            pan_target = current_pan - (offset_x * P_GAIN) 				# Calculate target pan angle (left = increase, right = decrease)
                            current_pan = move_servo_smooth(PAN_SERVO, pan_target, current_pan) 	# Smoothly move pan servo to new position
                        if abs(offset_y) > MOVEMENT_THRESHOLD: 						# Only move if offset y is larger than threshold
                            tilt_target = current_tilt + (offset_y * P_GAIN) 				# Calculate target tilt angle (up = increase, down = decrease)
                            current_tilt = move_servo_smooth(TILT_SERVO, tilt_target, current_tilt) 	# Smoothly move tilt servo to new position                        
                        cv2.putText(frame, f"X Offset: {offset_x}", (10, 30), 				# Draw offset x tracking info to frame
                                  cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2)
                        cv2.putText(frame, f"Y Offset: {offset_y}", (10, 60), 				# Draw offset y tracking info to frame
                                  cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2)
                        break 										# Only track first person detected
            if not person_detected: 									# When no detection present, prompt in terminal
                cv2.putText(frame, "No person detected", (10, 30), 
                           cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 0, 255), 2)
            cv2.imshow("Person Tracking", frame) 							# Display tracking frame
            if cv2.waitKey(1) & 0xFF == ord('q'): 							# Break loop on 'q'
                break
                
        # Handling closing operations
    except KeyboardInterrupt: 										# Handling Ctrl+C
        print("\nEmergency stop!")
    except Exception as e: 										# Reporting errors
        print(f"An error occurred: {str(e)}")
    finally: 												# Shutdown operations
        print("Cleaning up...") 									# Terminal prompt on shutdown
        init_servos()  											# Returning servos to center
        cv2.destroyAllWindows() 									# Closing GUIs
        picam2.stop() 											# Halting camera

	# Start and run tracking script
if __name__ == "__main__":
    picam2.start()
    track_person()

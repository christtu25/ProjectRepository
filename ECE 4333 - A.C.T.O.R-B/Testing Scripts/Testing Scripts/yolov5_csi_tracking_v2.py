import sys
import cv2
import time
from adafruit_servokit import ServoKit
from yoloDet import YoloTRT

# GStreamer pipeline for IMX-219 CSI camera
def gstreamer_pipeline(
    capture_width=1280,
    capture_height=720,
    display_width=600,
    display_height=338,  # Maintain aspect ratio (720/1280 * 600 ≈ 338)
    framerate=30,
    flip_method=0,
):
    return (
        f"nvarguscamerasrc ! "
        f"video/x-raw(memory:NVMM), width=(int){capture_width}, height=(int){capture_height}, "
        f"format=(string)NV12, framerate=(fraction){framerate}/1 ! "
        f"nvvidconv flip-method={flip_method} ! "
        f"video/x-raw, width=(int){display_width}, height=(int){display_height}, format=(string)BGRx ! "
        f"videoconvert ! "
        f"video/x-raw, format=(string)BGR ! appsink"
    )

# Initialize YOLOv5 TensorRT model
model = YoloTRT(library="yolov5/build/libmyplugins.so", engine="yolov5/build/yolov5s.engine", conf=0.5, yolo_ver="v5")

# Initialize servo controller
print("Initializing PCA9685...")
try:
    kit = ServoKit(channels=16)  # Default I2C bus 1, address 0x40
except Exception as e:
    print(f"Error initializing PCA9685: {e}")
    sys.exit(1)

# Servo channels and parameters
PAN_SERVO = 0
TILT_SERVO = 1
CENTER_ANGLE = 135  # Center position for 270° servo
current_pan = CENTER_ANGLE
current_tilt = CENTER_ANGLE

# Tracking parameters
MOVEMENT_THRESHOLD = 30  # Minimum pixel distance from center to move
MAX_SPEED = 2.0  # Maximum degrees to move per frame
P_GAIN = 0.1  # Proportional gain for smooth movement
SAFE_MIN_ANGLE = 1  # Stay 1° from minimum (0°)
SAFE_MAX_ANGLE = 269  # Stay 1° from maximum (270°)

def init_servos():
    """Initialize servos to center position"""
    global current_pan, current_tilt
    print("Centering servos...")
    try:
        kit.servo[PAN_SERVO].angle = CENTER_ANGLE
        kit.servo[TILT_SERVO].angle = CENTER_ANGLE
        current_pan = CENTER_ANGLE
        current_tilt = CENTER_ANGLE
        time.sleep(2)
    except Exception as e:
        print(f"Error centering servos: {e}")

def move_servo_smooth(channel, target_angle, current_angle):
    """Move servo smoothly towards target angle with safety limits"""
    # Clamp target angle to safe range before processing
    target_angle = max(SAFE_MIN_ANGLE, min(SAFE_MAX_ANGLE, target_angle))
    print(f"Servo {channel} target: {target_angle:.1f}° (current: {current_angle:.1f}°)")

    # Check if at limit and target is beyond
    if current_angle <= SAFE_MIN_ANGLE and target_angle <= SAFE_MIN_ANGLE:
        print(f"Servo {channel} holding at minimum safe angle ({SAFE_MIN_ANGLE}°).")
        return current_angle
    if current_angle >= SAFE_MAX_ANGLE and target_angle >= SAFE_MAX_ANGLE:
        print(f"Servo {channel} holding at maximum safe angle ({SAFE_MAX_ANGLE}°).")
        return current_angle

    # Calculate smooth movement
    diff = target_angle - current_angle
    if abs(diff) > MAX_SPEED:
        new_angle = current_angle + (MAX_SPEED if diff > 0 else -MAX_SPEED)
    else:
        new_angle = target_angle

    # Double-check new angle is safe
    new_angle = max(SAFE_MIN_ANGLE, min(SAFE_MAX_ANGLE, new_angle))

    # Move servo with error handling
    try:
        kit.servo[channel].angle = new_angle
    except Exception as e:
        print(f"Error setting servo {channel} to {new_angle}°: {e}")
        return current_angle  # Keep current angle on error

    return new_angle

def track_person():
    global current_pan, current_tilt

    # Initialize CSI camera
    cap = cv2.VideoCapture(gstreamer_pipeline(), cv2.CAP_GSTREAMER)
    if not cap.isOpened():
        print("Error: Could not open CSI camera.")
        sys.exit(1)

    try:
        init_servos()
        print("Starting tracking system...")

        while True:
            ret, frame = cap.read()
            if not ret:
                print("Error: Failed to capture frame.")
                break

            height, width = frame.shape[:2]
            center_x = width // 2
            center_y = height // 2

            # Run YOLOv5 detection
            detections, t = model.Inference(frame)
            person_detected = False

            for obj in detections:
                if obj['class'] == 'person':  # YOLOv5 class name for person
                    person_detected = True
                    box = obj['box']
                    x1, y1, x2, y2 = map(int, box)

                    # Calculate center of detected person
                    person_x = (x1 + x2) // 2
                    person_y = (y1 + y2) // 2

                    # Draw detection box and center point
                    cv2.rectangle(frame, (x1, y1), (x2, y2), (0, 255, 0), 2)
                    cv2.circle(frame, (person_x, person_y), 5, (0, 0, 255), -1)

                    # Calculate offset from center
                    offset_x = person_x - center_x
                    offset_y = person_y - center_y

                    # Move servos if offset exceeds threshold
                    if abs(offset_x) > MOVEMENT_THRESHOLD:
                        pan_target = current_pan - (offset_x * P_GAIN)
                        current_pan = move_servo_smooth(PAN_SERVO, pan_target, current_pan)

                    if abs(offset_y) > MOVEMENT_THRESHOLD:
                        tilt_target = current_tilt + (offset_y * P_GAIN)
                        current_tilt = move_servo_smooth(TILT_SERVO, tilt_target, current_tilt)

                    # Draw tracking info
                    cv2.putText(frame, f"X Offset: {offset_x}", (10, 30),
                                cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2)
                    cv2.putText(frame, f"Y Offset: {offset_y}", (10, 60),
                                cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2)

                    # Track only the first person detected
                    break

            if not person_detected:
                cv2.putText(frame, "No person detected", (10, 30),
                            cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 0, 255), 2)

            # Display FPS
            fps = 1 / t if t > 0 else 0
            cv2.putText(frame, f"FPS: {fps:.1f}", (10, 90),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2)

            # Show frame
            cv2.imshow("Person Tracking", frame)

            # Exit on 'q'
            if cv2.waitKey(1) & 0xFF == ord('q'):
                break

    except KeyboardInterrupt:
        print("\nEmergency stop!")
    except Exception as e:
        print(f"An error occurred: {str(e)}")
    finally:
        print("Cleaning up...")
        init_servos()
        cap.release()
        cv2.destroyAllWindows()

if __name__ == "__main__":
    track_person()

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
model = YoloTRT(library="yolov5/build/libmyplugins.so", engine="yolov5/build/best.engine", conf=0.5, yolo_ver="v5")

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

    # Initialize CSI camera (IMX-219 for tracking)
    csi_cap = cv2.VideoCapture(gstreamer_pipeline(), cv2.CAP_GSTREAMER)
    if not csi_cap.isOpened():
        print("Error: Could not open CSI camera.")
        sys.exit(1)

    # Initialize USB webcam (Logitech 1080p for live feed)
    usb_cap = None
    for i in range(3):  # Try /dev/video0, /dev/video1, /dev/video2
        usb_cap = cv2.VideoCapture(i, cv2.CAP_V4L2)
        if usb_cap.isOpened():
            print(f"USB webcam opened at /dev/video{i}")
            usb_cap.set(cv2.CAP_PROP_FRAME_WIDTH, 1920)
            usb_cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 1080)
            break
        usb_cap.release()
    if usb_cap is None or not usb_cap.isOpened():
        print("Warning: Could not open USB webcam. Continuing with CSI camera only.")
        usb_cap = None

    try:
        init_servos()
        print("Starting tracking system (CSI) and USB webcam feed...")

        while True:
            # Read CSI camera frame (for tracking)
            ret_csi, frame_csi = csi_cap.read()
            if not ret_csi:
                print("Error: Failed to capture CSI frame.")
                break

            height, width = frame_csi.shape[:2]
            center_x = width // 2
            center_y = height // 2

            # Run YOLOv5 detection on CSI frame
            detections, t = model.Inference(frame_csi)
            person_detected = False

            for obj in detections:
                if obj['class'] == 'News_Anchor':  # YOLOv5 class name for person
                    person_detected = True
                    box = obj['box']
                    x1, y1, x2, y2 = map(int, box)

                    # Calculate center of detected person
                    person_x = (x1 + x2) // 2
                    person_y = (y1 + y2) // 2

                    # Draw detection box and center point
                    cv2.rectangle(frame_csi, (x1, y1), (x2, y2), (0, 255, 0), 2)
                    cv2.circle(frame_csi, (person_x, person_y), 5, (0, 0, 255), -1)

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
                    cv2.putText(frame_csi, f"X Offset: {offset_x}", (10, 30),
                                cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2)
                    cv2.putText(frame_csi, f"Y Offset: {offset_y}", (10, 60),
                                cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2)

                    # Track only the first person detected
                    break

            if not person_detected:
                cv2.putText(frame_csi, "No person detected", (10, 30),
                            cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 0, 255), 2)

            # Display FPS for CSI camera
            fps = 1 / t if t > 0 else 0
            cv2.putText(frame_csi, f"FPS: {fps:.1f}", (10, 90),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2)

            # Show CSI camera frame (tracking)
            cv2.imshow("Person Tracking (CSI)", frame_csi)

            # Read and display USB webcam frame (raw feed)
            if usb_cap is not None:
                ret_usb, frame_usb = usb_cap.read()
                if ret_usb:
                    cv2.imshow("USB Webcam Feed", frame_usb)
                else:
                    print("Warning: Failed to capture USB frame.")

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
        csi_cap.release()
        if usb_cap is not None:
            usb_cap.release()
        cv2.destroyAllWindows()

if __name__ == "__main__":
    track_person()

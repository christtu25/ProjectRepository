from picamera2 import Picamera2
import cv2
from ultralytics import YOLO
import time

# Initialize camera
picam2 = Picamera2()
picam2.preview_configuration.main.size = (640, 480)  # Smaller resolution for faster processing
picam2.preview_configuration.main.format = "RGB888"
picam2.configure("preview")
picam2.start()

# Initialize YOLOv8 model
model = YOLO('yolov8n.pt')

while True:
    # Capture frame
    frame = picam2.capture_array()
    
    # Run detection
    results = model(frame, conf=0.5)  # Confidence threshold of 0.5
    
    # Process results
    for result in results:
        boxes = result.boxes
        for box in boxes:
            # Only process if detection is a person (class 0 in COCO dataset)
            if box.cls == 0:  
                # Get coordinates
                x1, y1, x2, y2 = box.xyxy[0]
                x1, y1, x2, y2 = int(x1), int(y1), int(x2), int(y2)
                
                # Draw box
                cv2.rectangle(frame, (x1, y1), (x2, y2), (0, 255, 0), 2)
                
                # Add label
                conf = float(box.conf)
                label = f'Person {conf:.2f}'
                cv2.putText(frame, label, (x1, y1-10), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 2)

    # Display frame
    cv2.imshow("Human Detection", frame)
    
    # Break loop on 'q' press
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

cv2.destroyAllWindows()
picam2.stop()

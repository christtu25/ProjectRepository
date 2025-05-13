import os
import random
import shutil
from pathlib import Path
import yaml
import subprocess
import sys

#  PARAMETERS
dataset_dir = r"C:\Users\Abraham Philips\Downloads\yolo_augmented"
yolov5_repo = r"C:\Users\Abraham Philips\yolov5"
output_dir = os.path.join(dataset_dir, "split_dataset")
train_ratio = 0.9
epochs = 10
batch_size = 64
imgsz = 16
model_cfg = "yolov5s.yaml"
project_name = "yolo_anchor"
name = "exp1"


if os.path.exists(output_dir):
    shutil.rmtree(output_dir)  

# Splitting Dataset 
images = list(Path(dataset_dir, "images").glob("*.jpg"))
random.shuffle(images)

split_index = int(train_ratio * len(images))
train_images = images[:split_index]
val_images = images[split_index:]

print(f"Total images: {len(images)}")
print(f"Train images: {len(train_images)}")
print(f"Val images: {len(val_images)}")

def copy_split(images, split):
    image_dir = Path(output_dir, "images", split)
    label_dir = Path(output_dir, "labels", split)
    image_dir.mkdir(parents=True, exist_ok=True)
    label_dir.mkdir(parents=True, exist_ok=True)

    for img_path in images:
        label_path = Path(dataset_dir, "labels", img_path.stem + ".txt")
        if label_path.exists():
            shutil.copy(img_path, image_dir / img_path.name)
            shutil.copy(label_path, label_dir / label_path.name)

copy_split(train_images, "train")
copy_split(val_images, "val")

#  data.yaml
class_names = ["News_Anchor"]  

data_yaml = {
    "train": str(Path(output_dir, "images", "train")).replace("\\", "/"),
    "val": str(Path(output_dir, "images", "val")).replace("\\", "/"),
    "nc": len(class_names),
    "names": class_names
}

yaml_path = os.path.join(output_dir, "data.yaml")
with open(yaml_path, "w") as f:
    yaml.dump(data_yaml, f)

print(f"\n Data prepared and saved to {output_dir}")

#  Training YOLOv5
train_cmd = [
    sys.executable,
    "train.py",
    "--img", str(imgsz),
    "--batch", str(batch_size),
    "--epochs", str(epochs),
    "--data", yaml_path,
    "--cfg", os.path.join(yolov5_repo, "models", model_cfg),
    "--weights", "yolov5s.pt",
    "--name", name,
    "--project", project_name,
    "--exist-ok",
    
        
]


print("\n Starting YOLOv5 training...")
subprocess.run(train_cmd, cwd=yolov5_repo)

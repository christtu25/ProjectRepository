# ACTOR-B Project (Autonomous Camera Tracking and Object Recognition Broadcast)
### Note: Jetson Nano requires SDK Manager for flashing JetPack 4.6.6 & manual installation from source (in isolated environment) for Python 3.6.9 functionality.

## Hardware Requirements
### - CSI camera module must be connected before RPi/Jetson boot.
### - PWM module needs powered by battery and connected via SDA & SCL GPIO pins.
### - Power monitor via USB-C.
### - Dual-battery 12.6V/5A power (6+ hrs runtime).
### - Two camera GUI feeds: USB for live broadcast and CSI for detection broadcast.

# RPi4 Installation List (Prototype)
### Below contains the installed libraries in the Raspberry Pi.
### Any updates to the system-wide or python environment will appear here.
### Must be careful installing to system-wide or python environment accordingly to avoid errors in the Raspberry Pi.
## System-wide directory installations:
```bash
sudo apt install python3-opencv python3-picamera2 python3-libcamera
sudo apt-get install -y i2c-tools
```
## Environment directory installations:
```bash
bashpython3 -m venv --system-site-packages env_ml
pip3 install torch torchvision ultralytics
pip3 install adafruit-circuitpython-pca9685
pip3 install adafruit-circuitpython-servokit
pip install google-auth google-auth-oauthlib google-auth-httplib2 google-api-python-client
pip install labelImg
```
## Discarded system-wide directory installations:
```bash
sudo pip3 uninstall torch torchvision ultralytics
sudo apt remove python3-torch python3-torchvision
sudo apt autoremove
```

# Jetson Nano Installation List (Final Product)
### JetPack 4.6.6 for the Jetson Nano A02 4GB Dev Kit (32.7) includes SDK components with CUDA (CUDA Toolkit for L4T 10.2), CUDA-X AI (cuDNN on Target 8.2, TensorRT on Target 8.2), Computer Vision (OpenCV on Target 4.1.1, VisionWorks on Target 1.6, VPI on Target 1.2), NVIDIA Container, Multimedia (API 32.7), Developer Tools (Nsight Systems 2024.2), and DeepStream (6.0.1).
## Accessing SDK Manager via Windows PC & Installing JetPack 4.6.6:
```bash
# Windows Command Prompt *Admin*
    # Jetson must be connected via USB *in recovery mode*
usbipd list # Find the Jetson BusID
usbipd attach --wsl Ubuntu-18.04 --busid x-y --auto-attach # Attaching USBIPD
    # Replace x-y with the Nvidia Corp. APX BUSID
    # Once attached, keep this window open during flash and proceed below...

# WSL *Admin*
wsl # Access WSL Ubuntu-18.04 terminal - Must set as default before accessing Tegra
cd /home/chris/nvidia/nvidia_sdk/JetPack_4.6.6_Linux_JETSON_NANO_TARGETS/Linux_for_Tegra/
sudo rm -rf rootfs/*  # Clearing partial rootfs *removes previous JetPack builds*
cat /proc/sys/fs/binfmt_misc/qemu-aarch64 # Checking registered QEMU 
    # If QEMU is missing/disabled, manually register below then check QEMU status again
sudo bash -c 'echo ":qemu-aarch64:M::\x7fELF\x02\x01\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x02\x00\xb7\x00:\xff\xff\xff\xff\xff\xff\xff\x00\xff\xff\xff\xff\xff\xff\xff\xff\xfe\xff\xff\xff:/usr/bin/qemu-aarch64-static:" > /proc/sys/fs/binfmt_misc/register'
    # Download/Install JetPack 4.6.6 via CLI - Must fully enable binfmt_misc in wsl.conf before proceeding
sdkmanager --cli install --exportlogs ~/flash_logs --product Jetson --version 4.6.6 --targetos Linux --target JETSON_NANO --flash all --additionalsdk "DeepStream"

# JetPack 4.6.6 Ubuntu
    # Upon a fresh OS flash, complete the Jetson setup prompts to access the desktop and access the terminal
    # Determine the Jetson IPv4 USB0 iNET IP, then proceed as shown/set in SDK Manager to complete remaining steps
    # If SSH errors appear, enter new WSL *Admin* window and enter Jetson IP manually then try again
sudo ip link set usb0 up
sudo ip addr add 192.168.55.100/24 dev usb0
ip addr show usb0
```
### Below contains the installed libraries in the Jetson Nano.
### Any updates to the system-wide, python directory, or python environment will appear here.
### Must be careful installing to system-wide or python environment accordingly to avoid errors in the Jetson.
```bash
JetPack Installations:
Python 3.6.9​
CUDA 10.2.3​
CuDNN 8.2.1.32​
TensorRT 8.2.1.9​
DeepStream 6.0.1​
GStreamer 1.14.5​
L4T 32.7.6 (Ubuntu 18.04)​
NVIDIA Container Runtime​
VisionWorks​
Multimedia API​
Jetson.GPIO 2.0.17​
Pandas 0.22.0
```

```bash
Pytorch 1.9.0​
Torch 1.10.0​
Torchvision 0.11.1​
OpenCV 4.5.1 (w/ CUDA support)​
YOLOv5​
TensorRT conversion tools​
Adafruit Blinka 6.0.0​
Adafruit CircuitPython ServoKit 1.3.9​
Adafruit-PCA9685 1.0.1​
Typing Extension 4.1.1​
tqdm 4.64.1​
imutils 0.5.4, numpy 1.19.0, & uff 0.6.9​
I2C tools 4.0.0​
PyYAML 3.12 & pillow 8.4.0​
Cython 3.0.12 & GitPython 3.1.18
```
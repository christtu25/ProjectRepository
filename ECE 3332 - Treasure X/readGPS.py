import RPi.GPIO as GPIO
import serial
import time

ser = serial.Serial('/dev/ttyS0', 115200)
ser.flushInput()

power_key = 6

def send_at(command, back, timeout):
    rec_buff = ''
    ser.write((command + '\r\n').encode())
    time.sleep(timeout)
    if ser.inWaiting():
        time.sleep(0.01)
        rec_buff = ser.read(ser.inWaiting()).decode()
    if rec_buff != '':
        if back not in rec_buff:
            print(command + ' ERROR')
            print(command + ' back:\t' + rec_buff)
            return 0, rec_buff
        else:
            print(rec_buff)
            return 1, rec_buff
    else:
        print('GPS is not ready')
        return 0, rec_buff

def dms_to_dd(dms, direction, is_longitude=False):
    if is_longitude:
        degrees = int(dms[:3])
        minutes = float(dms[3:])
    else:
        degrees = int(dms[:2])
        minutes = float(dms[2:])
    dd = degrees + minutes / 60
    if direction in ['S', 'W']:
        dd *= -1
    return dd

def get_gps_position():
    rec_null = True
    answer = 0
    print('Start GPS session...')
    send_at('AT+CGPS=1,1', 'OK', 1)
    time.sleep(2)
    while rec_null:
        answer, rec_buff = send_at('AT+CGPSINFO', '+CGPSINFO: ', 1)
        if 1 == answer:
            if ',,,,,,' in rec_buff:
                print('GPS is not ready')
                time.sleep(1)
            else:
                # Extract data
                try:
                    data = rec_buff.split(',')
                    if len(data) >= 7:
                        latitude = data[0].split(': ')[1]
                        latitude_direction = data[1]
                        longitude = data[2]
                        longitude_direction = data[3]

                        latitude_dd = dms_to_dd(latitude, latitude_direction)
                        longitude_dd = dms_to_dd(longitude, longitude_direction, is_longitude=True)

                        print(f"Latitude: {latitude_dd}, Longitude: {longitude_dd}")
                        print(f"Google Maps URL: https://www.google.com/maps?q={latitude_dd},{longitude_dd}")
                       
                        # Return the coordinates
                        return latitude_dd, longitude_dd
                except Exception as e:
                    print(f"Error parsing GPS data: {e}")
        else:
            print('Error %d' % answer)
            send_at('AT+CGPS=0', 'OK', 1)
            return False
        time.sleep(1.5)

def power_on(power_key):
    print('SIM7600X is starting:')
    GPIO.setmode(GPIO.BCM)
    GPIO.setwarnings(False)
    GPIO.setup(power_key, GPIO.OUT)
    time.sleep(0.1)
    GPIO.output(power_key, GPIO.HIGH)
    time.sleep(2)
    GPIO.output(power_key, GPIO.LOW)
    time.sleep(20)
    ser.flushInput()
    print('SIM7600X is ready')

def power_down(power_key):
    print('SIM7600X is logging off:')
    GPIO.output(power_key, GPIO.HIGH)
    time.sleep(3)
    GPIO.output(power_key, GPIO.LOW)
    time.sleep(18)
    print('Goodbye')

if __name__ == "__main__":
    try:
        power_on(power_key)
        get_gps_position()
        power_down(power_key)
    except Exception as e:
        print(f"An error occurred: {e}")
        if ser is not None:
            ser.close()
        power_down(power_key)
        GPIO.cleanup()
    if ser is not None:
        ser.close()
        GPIO.cleanup()

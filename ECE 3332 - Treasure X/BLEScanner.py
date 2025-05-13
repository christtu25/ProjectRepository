from bluepy.btle import Scanner

# List of known beacons
beacons = ["e1:a4:47:95:63:70", "c2:7f:0a:ba:09:1d", "d6:ee:e4:3b:37:95", "f0:03:1c:3e:dc:ee", "f9:67:58:3a:0f:6d", "f1:c2:51:76:6e:54", "de:0a:7c:3a:6b:0c", "d2:df:c1:4c:1c:79"]

def scan_for_beacons():
    scanner = Scanner()
    beacons_active = 2  # Number of active beacons to find
    gps_coordinates = []  # List to store GPS coordinates
    lock = 0
    counter = 0
    rssi_tot = 0
    avg_rssi = 0
    prev_rssi = 0

    while beacons_active > 0:
        devices = scanner.scan(.5)
        for device in devices:
            if device.addr in beacons:
                print(f"DEV = {device.addr} RSSI = {device.rssi}")

                if device.rssi > -75:
                    tracked_addr = device.addr
                    print(f"Locking on to beacon with address {tracked_addr}")
                    lock = 1
                    counter = 0
                    rssi_tot = 0

                    while lock == 1:
                        devices = scanner.scan(.5)
                        for device in devices:
                            if device.addr == tracked_addr:
                                print(f"DEV = {device.addr} RSSI = {device.rssi}")
                                counter += 1
                                rssi_tot += device.rssi
                                if counter == 5:
                                    prev_rssi = avg_rssi
                                    avg_rssi = rssi_tot / 5
                                    counter = 0
                                    rssi_tot = 0
                                    print(f"The average RSSI is {avg_rssi}")
                                    if avg_rssi < prev_rssi and prev_rssi != 0:
                                        print(f"You are moving away from the beacon")
                                    else if avg_rssi >= prev_rssi and prev_rssi != 0:
                                        print(f"You are moving closer to the beacon!")

                                if avg_rssi > -50 and avg_rssi != 0:
                                    print(f"Beacon with address {device.addr} has been found, removing address...")
                                    if device.addr in beacons:
                                        beacons.remove(tracked_addr)
                                        tracked_addr = None

                                    counter = 0
                                    rssi_tot = 0
                                    lock = 0
                                    avg_rssi = 0
                                    beacons_active -= 1
                                    prev_rssi = 0
                                    print(f"Remaining beacons list {beacons}")
                                    return device.addr
                                    
                                if avg_rssi < -75 and avg_rssi != 0:
                                    print(f"Beacon with address {device.addr} is too far, disengaging lock on")
                                    counter = 0
                                    rssi_tot = 0
                                    lock = 0
                                    avg_rssi = 0
                                    prev_rssi = 0
    return None

if __name__ == "__main__":
    found_beacon = scan_for_beacons()
    if found_beacon:
        print(f"Beacon found: {found_beacon}")
    else:
        print("No beacons found.")

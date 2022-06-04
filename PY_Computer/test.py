import bluetooth

SERVICE_UUID = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"  # UART service UUID
CHARACTERISTIC_UUID_RX = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
CHARACTERISTIC_UUID_TX = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

addr = "94:B9:7E:DF:77:A6"
service_matches = bluetooth.find_service(uuid=SERVICE_UUID, address=addr)


first_match = service_matches[0]
port = first_match["port"]
name = first_match["name"]
host = first_match["host"]

print("Connecting to \"{}\" on {}".format(name, host))

sock = bluetooth.BluetoothSocket(bluetooth.RFCOMM)
sock.connect((host, port))

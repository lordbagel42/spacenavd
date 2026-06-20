import socket
import time
import struct

def send_motion(axis, val):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    data = struct.pack('<BBh', 1, axis, val)
    sock.sendto(data, ('127.0.0.1', 9999))

def send_button(btn, press):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    data = struct.pack('<BBB', 2, btn, press)
    sock.sendto(data, ('127.0.0.1', 9999))

def send_flush():
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.sendto(struct.pack('<B', 3), ('127.0.0.1', 9999))

if __name__ == "__main__":
    print("SpaceMouse Enterprise Simulator")
    time.sleep(1)
    send_motion(0, 100)
    send_flush()
    time.sleep(0.5)
    send_button(31, 1)
    send_flush()
    time.sleep(0.5)
    send_button(31, 0)
    send_flush()
    print("Done.")

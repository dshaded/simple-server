#! /usr/bin/python

import socket
from time import sleep


def crc16_arc(data: bytes) -> bytes:
    crc = 0x0000  # Initial value
    poly = 0xA001  # Reversed polynomial (0x8005 original)

    for byte in data:
        crc ^= byte  # XOR byte into low byte of crc
        for _ in range(8):  # Process 8 bits
            if crc & 0x0001:  # Check if LSB is set
                crc = (crc >> 1) ^ poly
            else:
                crc >>= 1
    return (crc & 0xFFFF).to_bytes(2, 'big')

def send(sock, cmd_id: int, data: bytes):
    head = b'CMD'
    full_data = cmd_id.to_bytes(2, 'big') + data
    checksum = crc16_arc(full_data)
    sock.send(head + full_data + checksum)


s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect(('127.0.0.1', 33721))
while True:
    send(s, 1,b'\x0BABCDE 12345')
    send(s, 2,b'\x0A')
    send(s, 3,b'\x01\x00\x10')
    sleep(5)


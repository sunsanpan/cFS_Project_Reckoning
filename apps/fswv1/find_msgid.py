#!/usr/bin/env python3
"""
Find the correct BMP280_APP_CMD_MID for your cFS configuration.

This script will try common message IDs and tell you which one works.
"""

import socket
import struct
import time

# Common message ID values to try
COMMON_MIDS = [
    0x1882,  # Default in our code
    0x1883,
    0x1884,
    0x1885,
    0x1886,
    0x1887,
    0x1888,
    0x1889,
    0x188A,
    0x188B,
    0x188C,
    0x188D,
    0x188E,
    0x188F,
]

CFS_IP = '127.0.0.1'
CFS_CMD_PORT = 1234

def send_noop(msg_id):
    """Send NOOP command (code 0) with given message ID"""
    
    # Build CCSDS primary header
    stream_id = 0x1800 | (msg_id & 0x7FF)
    sequence = 0xC000
    length = 7
    
    # Build secondary header (NOOP = command code 0)
    function_code = 0
    checksum = 0x00
    
    # Pack the message
    header = struct.pack('>HHH', stream_id, sequence, length)
    cmd_data = struct.pack('>BB', function_code, checksum)
    padding = struct.pack('>I', 0)
    message = header + cmd_data + padding + struct.pack('>H', 0)
    
    # Send via UDP
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.sendto(message, (CFS_IP, CFS_CMD_PORT))
    sock.close()

def main():
    print("=" * 60)
    print("BMP280 Message ID Finder")
    print("=" * 60)
    print()
    print("This script will send NOOP commands with different")
    print("message IDs to find the correct BMP280_APP_CMD_MID.")
    print()
    print("Watch your cFS events to see which one works!")
    print()
    print("Look for: 'BMP280: NOOP command'")
    print("Ignore:   'Invalid ground command code' or 'Invalid command'")
    print()
    print("-" * 60)
    
    for mid in COMMON_MIDS:
        print(f"Trying MID: 0x{mid:04X}... ", end='', flush=True)
        send_noop(mid)
        print("sent")
        time.sleep(0.5)
    
    print("-" * 60)
    print()
    print("Check your cFS events!")
    print()
    print("If you saw 'BMP280: NOOP command', that's the correct MID!")
    print()
    print("Then edit simple_cmd.py and bmp280_cmd_test.py:")
    print("  BMP280_CMD_MID = 0xXXXX  # Use the MID that worked")
    print()

if __name__ == '__main__':
    main()

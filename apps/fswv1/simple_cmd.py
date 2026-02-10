#!/usr/bin/env python3
"""
Simple BMP280 Command Sender (TO_LAB Style)

This script sends commands using the standard cFS TO_LAB UDP interface.
Much simpler than CCSDS packet building.

Usage:
    python3 simple_cmd.py led-on
    python3 simple_cmd.py led-off
    python3 simple_cmd.py blink
"""

import socket
import struct
import time
import sys

# Configuration
CFS_IP = '127.0.0.1'
CFS_CMD_PORT = 1234  # TO_LAB default command port

# Message ID and Command Codes
BMP280_CMD_MID = 0x1884  # Must match bmp280_app_msgids.h

CMD_CODES = {
    'noop': 0,
    'reset': 1,
    'enable': 2,
    'disable': 3,
    'led-on': 4,
    'led-off': 5,
    'toggle': 6,
    'status': 7,
}

def send_command(cmd_code, verbose=True):
    """
    Send a simple command packet to cFS.
    
    Packet format (big-endian):
    - Stream ID (2 bytes): Message ID with CCSDS flags
    - Sequence (2 bytes): Sequence count with flags
    - Length (2 bytes): Packet data length - 1
    - Command Code (1 byte): Function code
    - Checksum (1 byte): Command checksum (can be 0)
    """
    
    # Build CCSDS primary header
    stream_id = 0x1800 | (BMP280_CMD_MID & 0x7FF)  # Command packet
    sequence = 0xC000  # Standalone packet, sequence 0
    length = 1  # 2 bytes of data - 1 (cmd code + checksum only)
    
    # Build secondary header
    function_code = cmd_code
    checksum = 0x00
    
    # Pack the message (8 bytes total)
    header = struct.pack('>HHH', stream_id, sequence, length)
    cmd_data = struct.pack('>BB', function_code, checksum)
    
    message = header + cmd_data
    
    if verbose:
        print(f"Sending command code {cmd_code} to {CFS_IP}:{CFS_CMD_PORT}")
    
    # Send via UDP
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.sendto(message, (CFS_IP, CFS_CMD_PORT))
    sock.close()
    
    if verbose:
        print("✓ Sent")

def main():
    if len(sys.argv) < 2:
        print("Usage: python3 simple_cmd.py <command>")
        print("\nAvailable commands:")
        for cmd in CMD_CODES.keys():
            print(f"  {cmd}")
        print("\nSpecial commands:")
        print("  blink    - Blink LED 5 times")
        print("  test     - Test all LED commands")
        sys.exit(1)
    
    cmd = sys.argv[1].lower()
    
    if cmd == 'blink':
        print("Blinking LED 5 times...")
        for i in range(5):
            print(f"Blink {i+1}/5")
            send_command(CMD_CODES['led-on'])
            time.sleep(0.5)
            send_command(CMD_CODES['led-off'])
            time.sleep(0.5)
        print("Done!")
    
    elif cmd == 'test':
        print("Testing all LED commands...")
        commands = ['status', 'led-on', 'status', 'led-off', 'status', 'toggle', 'status']
        for c in commands:
            print(f"\n--- {c.upper()} ---")
            send_command(CMD_CODES[c])
            time.sleep(1)
        print("\nTest complete!")
    
    elif cmd in CMD_CODES:
        send_command(CMD_CODES[cmd])
    
    else:
        print(f"Unknown command: {cmd}")
        print("Run without arguments to see available commands")
        sys.exit(1)

if __name__ == '__main__':
    main()

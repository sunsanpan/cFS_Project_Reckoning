#!/usr/bin/env python3
"""
Simple UART to UDP Bridge - Binary CCSDS Mode
Receives CCSDS telemetry packets from UART and forwards to UDP
"""

import serial
import socket
import time

# ============================================================================
# CONFIGURATION - EDIT THESE VALUES
# ============================================================================

UART_DEVICE = '/dev/ttyS0'        # UART device path
UART_BAUD = 115200                # UART baud rate

UDP_HOST = '127.0.0.1'            # Destination IP address
UDP_PORT = 1235                   # Destination UDP port (OpenC3 uses 1235)

PACKET_SIZE = 56                  # CCSDS packet size in bytes (adjust if needed)

# ============================================================================

def main():
    print("="*60)
    print("UART to UDP Bridge - Binary CCSDS Mode")
    print("="*60)
    print(f"UART: {UART_DEVICE} @ {UART_BAUD} baud")
    print(f"UDP:  {UDP_HOST}:{UDP_PORT}")
    print(f"Packet Size: {PACKET_SIZE} bytes")
    print("="*60)
    
    # Open UART
    try:
        ser = serial.Serial(UART_DEVICE, UART_BAUD, timeout=1)
        print("✓ UART opened")
    except Exception as e:
        print(f"✗ UART error: {e}")
        return
    
    # Create UDP socket
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    print("✓ UDP socket created")
    
    # Flush UART buffer
    time.sleep(0.5)
    ser.reset_input_buffer()
    
    print("\nForwarding CCSDS packets... (Ctrl+C to stop)\n")
    
    packet_count = 0
    error_count = 0
    
    try:
        while True:
            # Read fixed-size binary packet
            data = ser.read(PACKET_SIZE)
            
            if len(data) == PACKET_SIZE:
                packet_count += 1
                
                # Forward binary packet to UDP
                sock.sendto(data, (UDP_HOST, UDP_PORT))
                
                # Display first few bytes in hex
                print(f"[{packet_count:6d}] {len(data)} bytes: {data[:16].hex()}")
                
            elif len(data) > 0:
                error_count += 1
                print(f"Partial packet: {len(data)} bytes (expected {PACKET_SIZE})")
                
    except KeyboardInterrupt:
        print(f"\n\nStopped.")
        print(f"Total packets: {packet_count}")
        print(f"Errors: {error_count}")
    
    finally:
        ser.close()
        sock.close()
        print("Closed.")

if __name__ == '__main__':
    main()

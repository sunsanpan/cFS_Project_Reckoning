#!/usr/bin/env python3
"""
Simple UART to UDP Bridge
Receives telemetry from UART and forwards to UDP
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
UDP_PORT = 5555                   # Destination UDP port

# ============================================================================

def main():
    print("="*60)
    print("UART to UDP Bridge")
    print("="*60)
    print(f"UART: {UART_DEVICE} @ {UART_BAUD} baud")
    print(f"UDP:  {UDP_HOST}:{UDP_PORT}")
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
    
    print("\nForwarding telemetry... (Ctrl+C to stop)\n")
    
    packet_count = 0
    
    try:
        while True:
            # Read line from UART
            line = ser.readline().decode('utf-8', errors='ignore').strip()
            
            if line:
                packet_count += 1
                
                # Forward to UDP
                sock.sendto(line.encode('utf-8'), (UDP_HOST, UDP_PORT))
                
                # Display
                print(f"[{packet_count:6d}] {line}")
                
    except KeyboardInterrupt:
        print(f"\n\nStopped. Total packets: {packet_count}")
    
    finally:
        ser.close()
        sock.close()
        print("Closed.")

if __name__ == '__main__':
    main()

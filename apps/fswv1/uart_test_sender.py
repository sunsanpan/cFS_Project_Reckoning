#!/usr/bin/env python3
"""
UART IMU Simulator and Tester
Sends test IMU data to /dev/ttyAMA0 continuously
"""

import serial
import time
import random
import sys

UART_PORT = "/dev/ttyAMA0"
BAUD_RATE = 115200

def generate_imu_data():
    """Generate realistic IMU test data."""
    ax = random.uniform(-0.1, 0.1)
    ay = random.uniform(-0.1, 0.1)
    az = random.uniform(9.7, 9.9)  # ~1g gravity
    gx = random.uniform(-0.05, 0.05)
    gy = random.uniform(-0.05, 0.05)
    gz = random.uniform(-0.05, 0.05)
    temp = random.uniform(24.0, 27.0)
    
    return f"${ax:.2f},{ay:.2f},{az:.2f},{gx:.2f},{gy:.2f},{gz:.2f},{temp:.2f},#\n"

def send_test_data_serial():
    """Send test data using pyserial."""
    try:
        ser = serial.Serial(UART_PORT, BAUD_RATE, timeout=1)
        print(f"Opened {UART_PORT} at {BAUD_RATE} baud")
        print("Sending IMU data every second...")
        print("Press Ctrl+C to stop\n")
        
        count = 0
        while True:
            data = generate_imu_data()
            ser.write(data.encode())
            count += 1
            print(f"[{count}] Sent: {data.strip()}")
            time.sleep(1)
            
    except serial.SerialException as e:
        print(f"Error opening serial port: {e}")
        print("\nTrying alternative method...")
        send_test_data_file()
    except KeyboardInterrupt:
        print("\nStopped.")
        ser.close()

def send_test_data_file():
    """Send test data by writing to device file."""
    try:
        print(f"Opening {UART_PORT} as file...")
        print("Sending IMU data every second...")
        print("Press Ctrl+C to stop\n")
        
        count = 0
        while True:
            data = generate_imu_data()
            with open(UART_PORT, 'w') as f:
                f.write(data)
            count += 1
            print(f"[{count}] Sent: {data.strip()}")
            time.sleep(1)
            
    except PermissionError:
        print(f"\nPermission denied! Try:")
        print(f"  sudo usermod -a -G dialout $USER")
        print(f"  (then log out and back in)")
        print(f"Or run with: sudo python3 {sys.argv[0]}")
    except KeyboardInterrupt:
        print("\nStopped.")
    except Exception as e:
        print(f"Error: {e}")

def send_single_packet():
    """Send a single test packet."""
    data = "$,0.05,-0.12,9.81,0.01,-0.02,0.00,25.5,#\n"
    try:
        with open(UART_PORT, 'w') as f:
            f.write(data)
        print(f"Sent: {data.strip()}")
    except Exception as e:
        print(f"Error: {e}")

def main():
    import argparse
    
    parser = argparse.ArgumentParser(description='UART IMU Simulator')
    parser.add_argument('-1', '--once', action='store_true', 
                       help='Send single packet and exit')
    parser.add_argument('-s', '--serial', action='store_true',
                       help='Use pyserial instead of file write')
    args = parser.parse_args()
    
    print("=" * 60)
    print("UART IMU Data Simulator")
    print("=" * 60)
    
    if args.once:
        send_single_packet()
    elif args.serial:
        send_test_data_serial()
    else:
        send_test_data_file()

if __name__ == '__main__':
    main()

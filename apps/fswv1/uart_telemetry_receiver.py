#!/usr/bin/env python3
"""
UART Telemetry Receiver Test Script

This script connects to the telemetry UART and displays received data.
Supports both ASCII and binary CCSDS formats.

Usage:
    python3 uart_telemetry_receiver.py [--device /dev/ttyS0] [--baud 115200] [--format ascii]
"""

import serial
import sys
import argparse
import struct
import time

def parse_ascii_telemetry(line):
    """
    Parse ASCII format telemetry line.
    Format: BMP:T=25.30,P=101325.00 IMU:Ax=0.05,Ay=-0.12,Az=9.81,Gx=0.01,Gy=-0.02,Gz=0.00,T=25.50 TS=1234567890
    """
    data = {}
    
    try:
        # Split into sections
        parts = line.split()
        
        for part in parts:
            if part.startswith('BMP:'):
                # Parse BMP280 data
                bmp_str = part[4:]  # Remove "BMP:"
                fields = bmp_str.split(',')
                for field in fields:
                    key, val = field.split('=')
                    if key == 'T':
                        data['BMP_Temperature'] = float(val)
                    elif key == 'P':
                        data['BMP_Pressure'] = float(val)
                        
            elif part.startswith('IMU:'):
                # Parse IMU data
                imu_str = part[4:]  # Remove "IMU:"
                fields = imu_str.split(',')
                for field in fields:
                    key, val = field.split('=')
                    if key == 'Ax':
                        data['Accel_X'] = float(val)
                    elif key == 'Ay':
                        data['Accel_Y'] = float(val)
                    elif key == 'Az':
                        data['Accel_Z'] = float(val)
                    elif key == 'Gx':
                        data['Gyro_X'] = float(val)
                    elif key == 'Gy':
                        data['Gyro_Y'] = float(val)
                    elif key == 'Gz':
                        data['Gyro_Z'] = float(val)
                    elif key == 'T':
                        data['IMU_Temperature'] = float(val)
                        
            elif part.startswith('TS='):
                # Parse timestamp
                data['Timestamp'] = int(part[3:])
                
        return data
        
    except Exception as e:
        print(f"Error parsing line: {e}")
        return None

def display_ascii_telemetry(data):
    """Display parsed ASCII telemetry in a nice format."""
    if not data:
        return
    
    print("\n" + "="*80)
    print("TELEMETRY DATA")
    print("="*80)
    
    if 'BMP_Temperature' in data or 'BMP_Pressure' in data:
        print("\nBMP280 Sensor:")
        if 'BMP_Temperature' in data:
            print(f"  Temperature: {data['BMP_Temperature']:7.2f} °C")
        if 'BMP_Pressure' in data:
            print(f"  Pressure:    {data['BMP_Pressure']:10.2f} Pa ({data['BMP_Pressure']/100:.2f} hPa)")
    
    if any(k.startswith('Accel_') or k.startswith('Gyro_') or k == 'IMU_Temperature' for k in data.keys()):
        print("\nIMU Sensor:")
        if 'Accel_X' in data:
            print(f"  Accelerometer: X={data.get('Accel_X', 0):6.2f}  Y={data.get('Accel_Y', 0):6.2f}  Z={data.get('Accel_Z', 0):6.2f} m/s²")
        if 'Gyro_X' in data:
            print(f"  Gyroscope:     X={data.get('Gyro_X', 0):6.2f}  Y={data.get('Gyro_Y', 0):6.2f}  Z={data.get('Gyro_Z', 0):6.2f} deg/s")
        if 'IMU_Temperature' in data:
            print(f"  Temperature:   {data['IMU_Temperature']:6.2f} °C")
    
    if 'Timestamp' in data:
        print(f"\nTimestamp: {data['Timestamp']}")
    
    print("="*80)

def receive_ascii_telemetry(ser):
    """Receive and display ASCII format telemetry."""
    print("Receiving ASCII telemetry data...")
    print("Press Ctrl+C to stop\n")
    
    packet_count = 0
    error_count = 0
    
    try:
        while True:
            line = ser.readline().decode('utf-8', errors='ignore').strip()
            
            if line:
                packet_count += 1
                data = parse_ascii_telemetry(line)
                
                if data:
                    display_ascii_telemetry(data)
                    print(f"\nPackets received: {packet_count}, Errors: {error_count}")
                else:
                    error_count += 1
                    print(f"Error parsing packet #{packet_count}: {line}")
                    
    except KeyboardInterrupt:
        print("\n\nStopped by user")
        print(f"Total packets received: {packet_count}")
        print(f"Total errors: {error_count}")

def receive_binary_telemetry(ser):
    """Receive and display binary CCSDS format telemetry."""
    print("Receiving binary CCSDS telemetry data...")
    print("Press Ctrl+C to stop\n")
    
    # Note: Adjust packet size based on actual CCSDS packet structure
    # This is a placeholder - you'll need to update based on your packet format
    CCSDS_HEADER_SIZE = 6
    PACKET_SIZE = 64  # Approximate - adjust based on sizeof(FSWV1_APP_CombinedTlm_t)
    
    packet_count = 0
    error_count = 0
    
    try:
        while True:
            data = ser.read(PACKET_SIZE)
            
            if len(data) == PACKET_SIZE:
                packet_count += 1
                
                # Parse CCSDS header (basic example)
                try:
                    print(f"\nPacket #{packet_count}:")
                    print(f"  Raw size: {len(data)} bytes")
                    print(f"  Raw data (hex): {data[:20].hex()}...")  # Show first 20 bytes
                    
                    # You would parse the actual packet structure here
                    # For now, just show it was received
                    
                except Exception as e:
                    error_count += 1
                    print(f"Error parsing packet: {e}")
            
            elif len(data) > 0:
                print(f"Partial packet received: {len(data)} bytes (expected {PACKET_SIZE})")
                error_count += 1
                
    except KeyboardInterrupt:
        print("\n\nStopped by user")
        print(f"Total packets received: {packet_count}")
        print(f"Total errors: {error_count}")

def main():
    parser = argparse.ArgumentParser(
        description='UART Telemetry Receiver for FSWV1 cFS App',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # ASCII format (default)
  python3 uart_telemetry_receiver.py
  
  # Specify device and baud rate
  python3 uart_telemetry_receiver.py --device /dev/ttyUSB0 --baud 115200
  
  # Binary CCSDS format
  python3 uart_telemetry_receiver.py --format binary
        """
    )
    
    parser.add_argument('--device', '-d', 
                       default='/dev/ttyS0',
                       help='UART device (default: /dev/ttyS0)')
    
    parser.add_argument('--baud', '-b', 
                       type=int,
                       default=115200,
                       help='Baud rate (default: 115200)')
    
    parser.add_argument('--format', '-f',
                       choices=['ascii', 'binary'],
                       default='ascii',
                       help='Telemetry format (default: ascii)')
    
    args = parser.parse_args()
    
    print("="*80)
    print("FSWV1 UART Telemetry Receiver")
    print("="*80)
    print(f"Device: {args.device}")
    print(f"Baud rate: {args.baud}")
    print(f"Format: {args.format}")
    print("="*80)
    
    try:
        # Open serial port
        ser = serial.Serial(args.device, args.baud, timeout=1)
        print(f"\nSuccessfully opened {args.device}")
        
        # Give the port a moment to stabilize
        time.sleep(0.5)
        
        # Flush any existing data
        ser.reset_input_buffer()
        
        # Receive telemetry based on format
        if args.format == 'ascii':
            receive_ascii_telemetry(ser)
        else:
            receive_binary_telemetry(ser)
            
    except serial.SerialException as e:
        print(f"\nError opening serial port: {e}")
        print("\nTroubleshooting tips:")
        print("1. Check if device exists: ls -l /dev/tty*")
        print("2. Check permissions: sudo usermod -a -G dialout $USER")
        print("3. Try running with sudo (not recommended for production)")
        sys.exit(1)
        
    except Exception as e:
        print(f"\nUnexpected error: {e}")
        sys.exit(1)
        
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()
            print("\nSerial port closed")

if __name__ == '__main__':
    main()

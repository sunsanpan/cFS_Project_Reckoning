#!/usr/bin/env python3
"""
FSWV1 Telemetry Receiver
Listens on UDP port 5555 and decodes combined BMP280 + IMU telemetry packets.
"""

import socket
import struct
import sys
from datetime import datetime

# Configuration
UDP_IP = "0.0.0.0"  # Listen on all interfaces
UDP_PORT = 5555

def decode_telemetry(data):
    """
    Decode FSWV1 combined telemetry packet.
    
    Packet structure:
    - Bytes 0-11:   cFS Telemetry Header (skip)
    - Bytes 12-15:  BMP_Temperature (float)
    - Bytes 16-19:  BMP_Pressure (float)
    - Bytes 20-23:  Accel_X (float)
    - Bytes 24-27:  Accel_Y (float)
    - Bytes 28-31:  Accel_Z (float)
    - Bytes 32-35:  Gyro_X (float)
    - Bytes 36-39:  Gyro_Y (float)
    - Bytes 40-43:  Gyro_Z (float)
    - Bytes 44-47:  IMU_Temperature (float)
    - Bytes 48-51:  Timestamp (uint32)
    """
    
    if len(data) < 52:
        print(f"Warning: Packet too short ({len(data)} bytes, expected 52+)")
        return None
    
    try:
        # Skip cFS header (first 12 bytes), extract payload
        payload = data[12:52]
        
        # Unpack data: 9 floats + 1 uint32 (big-endian)
        values = struct.unpack('>ffffffffI', payload)
        
        telemetry = {
            'bmp_temperature': values[0],
            'bmp_pressure': values[1],
            'accel_x': values[2],
            'accel_y': values[3],
            'accel_z': values[4],
            'gyro_x': values[5],
            'gyro_y': values[6],
            'gyro_z': values[7],
            'imu_temperature': values[8],
            'timestamp': values[9]
        }
        
        return telemetry
        
    except struct.error as e:
        print(f"Error unpacking data: {e}")
        return None

def print_telemetry(tlm):
    """Pretty print telemetry data."""
    
    if tlm is None:
        return
    
    print("\n" + "=" * 70)
    print(f"FSWV1 Telemetry - {datetime.now().strftime('%H:%M:%S.%f')[:-3]}")
    print("=" * 70)
    
    print("\n📡 BMP280 Sensor (I2C):")
    print(f"  Temperature: {tlm['bmp_temperature']:7.2f} °C")
    print(f"  Pressure:    {tlm['bmp_pressure']:7.2f} Pa")
    
    print("\n🚀 IMU Data (UART):")
    print(f"  Accelerometer:")
    print(f"    X: {tlm['accel_x']:7.2f}")
    print(f"    Y: {tlm['accel_y']:7.2f}")
    print(f"    Z: {tlm['accel_z']:7.2f}")
    
    print(f"  Gyroscope:")
    print(f"    X: {tlm['gyro_x']:7.2f}")
    print(f"    Y: {tlm['gyro_y']:7.2f}")
    print(f"    Z: {tlm['gyro_z']:7.2f}")
    
    print(f"  Temperature: {tlm['imu_temperature']:7.2f} °C")
    
    print(f"\n⏱️  Timestamp: {tlm['timestamp']} seconds")
    print("=" * 70)

def print_compact(tlm):
    """Print single-line compact format."""
    
    if tlm is None:
        return
    
    print(f"[{datetime.now().strftime('%H:%M:%S')}] "
          f"BMP: {tlm['bmp_temperature']:5.1f}°C {tlm['bmp_pressure']:6.1f}Pa | "
          f"ACC: {tlm['accel_x']:5.2f},{tlm['accel_y']:5.2f},{tlm['accel_z']:5.2f} | "
          f"GYR: {tlm['gyro_x']:5.2f},{tlm['gyro_y']:5.2f},{tlm['gyro_z']:5.2f} | "
          f"IMU: {tlm['imu_temperature']:5.1f}°C")

def main():
    """Main receiver loop."""
    
    print("=" * 70)
    print("FSWV1 Telemetry Receiver")
    print("=" * 70)
    print(f"Listening on {UDP_IP}:{UDP_PORT}")
    print("Press Ctrl+C to exit")
    print()
    print("Display mode: [d]etailed or [c]ompact?")
    print("(Default: detailed)")
    
    # Simple mode selection (optional)
    compact_mode = False
    # Uncomment next line for compact mode by default:
    # compact_mode = True
    
    # Create UDP socket
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((UDP_IP, UDP_PORT))
    
    print(f"\n✓ Listening for telemetry packets...")
    if compact_mode:
        print("(Compact mode)\n")
    else:
        print("(Detailed mode)\n")
    
    packet_count = 0
    
    try:
        while True:
            # Receive data
            data, addr = sock.recvfrom(1024)
            
            # Decode telemetry
            tlm = decode_telemetry(data)
            
            if tlm:
                packet_count += 1
                
                # Print based on mode
                if compact_mode:
                    print_compact(tlm)
                else:
                    print_telemetry(tlm)
            
    except KeyboardInterrupt:
        print("\n\nShutting down...")
        print(f"Total packets received: {packet_count}")
        sock.close()
        sys.exit(0)

if __name__ == '__main__':
    main()

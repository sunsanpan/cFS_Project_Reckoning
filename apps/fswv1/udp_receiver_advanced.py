#!/usr/bin/env python3
"""
FSWV1 Telemetry Receiver - Advanced Version
Features:
- Detailed and compact display modes
- CSV logging
- Statistics
- Real-time plotting (optional)
"""

import socket
import struct
import sys
import argparse
from datetime import datetime
import csv
import os

# Configuration
UDP_IP = "0.0.0.0"
UDP_PORT = 5555

class TelemetryStats:
    """Track telemetry statistics."""
    
    def __init__(self):
        self.count = 0
        self.bmp_temp_min = float('inf')
        self.bmp_temp_max = float('-inf')
        self.bmp_press_min = float('inf')
        self.bmp_press_max = float('-inf')
        self.start_time = datetime.now()
    
    def update(self, tlm):
        """Update statistics with new telemetry."""
        self.count += 1
        self.bmp_temp_min = min(self.bmp_temp_min, tlm['bmp_temperature'])
        self.bmp_temp_max = max(self.bmp_temp_max, tlm['bmp_temperature'])
        self.bmp_press_min = min(self.bmp_press_min, tlm['bmp_pressure'])
        self.bmp_press_max = max(self.bmp_press_max, tlm['bmp_pressure'])
    
    def print_stats(self):
        """Print accumulated statistics."""
        duration = (datetime.now() - self.start_time).total_seconds()
        print("\n" + "=" * 70)
        print("Statistics")
        print("=" * 70)
        print(f"Packets received: {self.count}")
        print(f"Duration: {duration:.1f} seconds")
        print(f"Rate: {self.count/duration:.2f} packets/sec")
        print(f"\nBMP280 Temperature: {self.bmp_temp_min:.2f} - {self.bmp_temp_max:.2f} °C")
        print(f"BMP280 Pressure:    {self.bmp_press_min:.2f} - {self.bmp_press_max:.2f} Pa")
        print("=" * 70)

def decode_telemetry(data):
    """Decode FSWV1 combined telemetry packet."""
    
    if len(data) < 52:
        return None
    
    try:
        payload = data[12:52]
        values = struct.unpack('>ffffffffI', payload)
        
        return {
            'bmp_temperature': values[0],
            'bmp_pressure': values[1],
            'accel_x': values[2],
            'accel_y': values[3],
            'accel_z': values[4],
            'gyro_x': values[5],
            'gyro_y': values[6],
            'gyro_z': values[7],
            'imu_temperature': values[8],
            'timestamp': values[9],
            'receive_time': datetime.now().strftime('%Y-%m-%d %H:%M:%S.%f')[:-3]
        }
        
    except struct.error:
        return None

def print_detailed(tlm):
    """Print detailed telemetry."""
    
    print("\n" + "=" * 70)
    print(f"FSWV1 Telemetry - {tlm['receive_time']}")
    print("=" * 70)
    
    print("\n📡 BMP280 Sensor:")
    print(f"  Temperature: {tlm['bmp_temperature']:7.2f} °C")
    print(f"  Pressure:    {tlm['bmp_pressure']:7.2f} Pa")
    
    print("\n🚀 IMU Data:")
    print(f"  Accelerometer: X={tlm['accel_x']:6.2f}  Y={tlm['accel_y']:6.2f}  Z={tlm['accel_z']:6.2f}")
    print(f"  Gyroscope:     X={tlm['gyro_x']:6.2f}  Y={tlm['gyro_y']:6.2f}  Z={tlm['gyro_z']:6.2f}")
    print(f"  Temperature:   {tlm['imu_temperature']:6.2f} °C")
    
    print(f"\n⏱️  Timestamp: {tlm['timestamp']} s")
    print("=" * 70)

def print_compact(tlm):
    """Print compact single-line telemetry."""
    
    print(f"[{tlm['receive_time'][11:]}] "
          f"BMP:{tlm['bmp_temperature']:5.1f}°C {tlm['bmp_pressure']:6.0f}Pa | "
          f"A:{tlm['accel_x']:5.2f},{tlm['accel_y']:5.2f},{tlm['accel_z']:5.2f} | "
          f"G:{tlm['gyro_x']:5.2f},{tlm['gyro_y']:5.2f},{tlm['gyro_z']:5.2f} | "
          f"T:{tlm['imu_temperature']:5.1f}°C")

def print_csv_line(tlm):
    """Print CSV format."""
    
    print(f"{tlm['receive_time']},"
          f"{tlm['bmp_temperature']:.2f},{tlm['bmp_pressure']:.2f},"
          f"{tlm['accel_x']:.4f},{tlm['accel_y']:.4f},{tlm['accel_z']:.4f},"
          f"{tlm['gyro_x']:.4f},{tlm['gyro_y']:.4f},{tlm['gyro_z']:.4f},"
          f"{tlm['imu_temperature']:.2f},{tlm['timestamp']}")

def log_to_csv(tlm, filename):
    """Log telemetry to CSV file."""
    
    file_exists = os.path.isfile(filename)
    
    with open(filename, 'a', newline='') as f:
        writer = csv.writer(f)
        
        # Write header if new file
        if not file_exists:
            writer.writerow([
                'Time', 'BMP_Temp', 'BMP_Press',
                'Accel_X', 'Accel_Y', 'Accel_Z',
                'Gyro_X', 'Gyro_Y', 'Gyro_Z',
                'IMU_Temp', 'Timestamp'
            ])
        
        # Write data
        writer.writerow([
            tlm['receive_time'],
            f"{tlm['bmp_temperature']:.2f}",
            f"{tlm['bmp_pressure']:.2f}",
            f"{tlm['accel_x']:.4f}",
            f"{tlm['accel_y']:.4f}",
            f"{tlm['accel_z']:.4f}",
            f"{tlm['gyro_x']:.4f}",
            f"{tlm['gyro_y']:.4f}",
            f"{tlm['gyro_z']:.4f}",
            f"{tlm['imu_temperature']:.2f}",
            tlm['timestamp']
        ])

def main():
    """Main receiver loop."""
    
    parser = argparse.ArgumentParser(description='FSWV1 Telemetry Receiver')
    parser.add_argument('-m', '--mode', choices=['detailed', 'compact', 'csv'], 
                       default='detailed', help='Display mode')
    parser.add_argument('-l', '--log', type=str, help='Log to CSV file')
    parser.add_argument('-p', '--port', type=int, default=5555, help='UDP port')
    parser.add_argument('-s', '--stats', action='store_true', help='Show statistics on exit')
    
    args = parser.parse_args()
    
    print("=" * 70)
    print("FSWV1 Telemetry Receiver")
    print("=" * 70)
    print(f"Mode: {args.mode}")
    print(f"Listening on {UDP_IP}:{args.port}")
    if args.log:
        print(f"Logging to: {args.log}")
    print("Press Ctrl+C to exit\n")
    
    # Print CSV header if in CSV mode
    if args.mode == 'csv':
        print("Time,BMP_Temp,BMP_Press,Accel_X,Accel_Y,Accel_Z,Gyro_X,Gyro_Y,Gyro_Z,IMU_Temp,Timestamp")
    
    # Create socket
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((UDP_IP, args.port))
    
    stats = TelemetryStats() if args.stats else None
    
    try:
        while True:
            data, addr = sock.recvfrom(1024)
            tlm = decode_telemetry(data)
            
            if tlm:
                # Update statistics
                if stats:
                    stats.update(tlm)
                
                # Display based on mode
                if args.mode == 'detailed':
                    print_detailed(tlm)
                elif args.mode == 'compact':
                    print_compact(tlm)
                elif args.mode == 'csv':
                    print_csv_line(tlm)
                
                # Log to file if requested
                if args.log:
                    log_to_csv(tlm, args.log)
            
    except KeyboardInterrupt:
        print("\n\nShutting down...")
        if stats:
            stats.print_stats()
        sock.close()
        sys.exit(0)

if __name__ == '__main__':
    main()

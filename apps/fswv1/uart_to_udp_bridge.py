#!/usr/bin/env python3
"""
UART to UDP Bridge for FSWV1 Telemetry

Receives telemetry from UART and forwards it to UDP port.
Supports both ASCII and binary CCSDS formats.

Usage:
    python3 uart_to_udp_bridge.py --uart /dev/ttyS0 --udp-port 5555 --format ascii
"""

import serial
import socket
import argparse
import sys
import time
import struct

class UARTtoUDPBridge:
    def __init__(self, uart_device, uart_baud, udp_host, udp_port, format_type='ascii'):
        self.uart_device = uart_device
        self.uart_baud = uart_baud
        self.udp_host = udp_host
        self.udp_port = udp_port
        self.format_type = format_type
        
        self.serial_port = None
        self.udp_socket = None
        
        self.packet_count = 0
        self.error_count = 0
        
    def connect(self):
        """Connect to UART and create UDP socket"""
        try:
            # Open UART
            print(f"Opening UART: {self.uart_device} @ {self.uart_baud} baud...")
            self.serial_port = serial.Serial(
                self.uart_device,
                self.uart_baud,
                timeout=1
            )
            print(f"✓ UART opened successfully")
            
            # Create UDP socket
            print(f"Creating UDP socket for {self.udp_host}:{self.udp_port}...")
            self.udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            print(f"✓ UDP socket created")
            
            # Flush any existing UART data
            time.sleep(0.5)
            self.serial_port.reset_input_buffer()
            
            return True
            
        except serial.SerialException as e:
            print(f"✗ UART Error: {e}")
            return False
        except Exception as e:
            print(f"✗ Error: {e}")
            return False
    
    def process_ascii_telemetry(self):
        """Process ASCII format telemetry"""
        print("\n" + "="*80)
        print("UART to UDP Bridge - ASCII Mode")
        print("="*80)
        print(f"UART: {self.uart_device} @ {self.uart_baud} baud")
        print(f"UDP:  {self.udp_host}:{self.udp_port}")
        print("="*80)
        print("\nWaiting for telemetry data... (Ctrl+C to stop)\n")
        
        try:
            while True:
                # Read line from UART
                line = self.serial_port.readline().decode('utf-8', errors='ignore').strip()
                
                if line:
                    self.packet_count += 1
                    
                    # Forward to UDP (send the ASCII string)
                    try:
                        self.udp_socket.sendto(
                            line.encode('utf-8'),
                            (self.udp_host, self.udp_port)
                        )
                        
                        # Display forwarded data
                        print(f"[{self.packet_count:6d}] {line}")
                        
                    except Exception as e:
                        self.error_count += 1
                        print(f"UDP send error: {e}")
                        
        except KeyboardInterrupt:
            print("\n\n" + "="*80)
            print("Stopped by user")
            print(f"Total packets forwarded: {self.packet_count}")
            print(f"Total errors: {self.error_count}")
            print("="*80)
    
    def process_binary_telemetry(self, packet_size=64):
        """Process binary CCSDS format telemetry"""
        print("\n" + "="*80)
        print("UART to UDP Bridge - Binary Mode")
        print("="*80)
        print(f"UART: {self.uart_device} @ {self.uart_baud} baud")
        print(f"UDP:  {self.udp_host}:{self.udp_port}")
        print(f"Packet Size: {packet_size} bytes")
        print("="*80)
        print("\nWaiting for telemetry packets... (Ctrl+C to stop)\n")
        
        try:
            while True:
                # Read fixed-size packet from UART
                data = self.serial_port.read(packet_size)
                
                if len(data) == packet_size:
                    self.packet_count += 1
                    
                    # Forward to UDP (send the binary packet)
                    try:
                        self.udp_socket.sendto(data, (self.udp_host, self.udp_port))
                        
                        # Display packet info
                        print(f"[{self.packet_count:6d}] Forwarded {len(data)} bytes: {data[:16].hex()}...")
                        
                    except Exception as e:
                        self.error_count += 1
                        print(f"UDP send error: {e}")
                        
                elif len(data) > 0:
                    self.error_count += 1
                    print(f"Partial packet received: {len(data)} bytes (expected {packet_size})")
                    
        except KeyboardInterrupt:
            print("\n\n" + "="*80)
            print("Stopped by user")
            print(f"Total packets forwarded: {self.packet_count}")
            print(f"Total errors: {self.error_count}")
            print("="*80)
    
    def run(self):
        """Main run loop"""
        if not self.connect():
            return False
        
        try:
            if self.format_type == 'ascii':
                self.process_ascii_telemetry()
            else:
                self.process_binary_telemetry()
        finally:
            self.close()
    
    def close(self):
        """Close connections"""
        if self.serial_port and self.serial_port.is_open:
            self.serial_port.close()
            print("\nUART port closed")
        
        if self.udp_socket:
            self.udp_socket.close()
            print("UDP socket closed")


def main():
    parser = argparse.ArgumentParser(
        description='UART to UDP Bridge for FSWV1 Telemetry',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # ASCII format (default)
  python3 uart_to_udp_bridge.py --uart /dev/ttyS0 --udp-port 5555
  
  # Binary format with custom packet size
  python3 uart_to_udp_bridge.py --uart /dev/ttyS0 --udp-port 5555 --format binary --packet-size 64
  
  # Forward to remote host
  python3 uart_to_udp_bridge.py --uart /dev/ttyUSB0 --udp-host 192.168.1.100 --udp-port 5555
        """
    )
    
    parser.add_argument('--uart', '-u',
                       default='/dev/ttyS0',
                       help='UART device (default: /dev/ttyS0)')
    
    parser.add_argument('--baud', '-b',
                       type=int,
                       default=115200,
                       help='UART baud rate (default: 115200)')
    
    parser.add_argument('--udp-host',
                       default='127.0.0.1',
                       help='UDP destination host (default: 127.0.0.1)')
    
    parser.add_argument('--udp-port', '-p',
                       type=int,
                       default=5555,
                       help='UDP destination port (default: 5555)')
    
    parser.add_argument('--format', '-f',
                       choices=['ascii', 'binary'],
                       default='ascii',
                       help='Telemetry format (default: ascii)')
    
    parser.add_argument('--packet-size',
                       type=int,
                       default=64,
                       help='Binary packet size in bytes (default: 64)')
    
    args = parser.parse_args()
    
    # Create and run bridge
    bridge = UARTtoUDPBridge(
        uart_device=args.uart,
        uart_baud=args.baud,
        udp_host=args.udp_host,
        udp_port=args.udp_port,
        format_type=args.format
    )
    
    bridge.run()


if __name__ == '__main__':
    main()

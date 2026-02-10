#!/usr/bin/env python3
"""
BMP280 cFS App - Command Test Script

This script sends telecommands to the BMP280 cFS application for testing.
It can send individual commands or run automated test sequences.

Usage:
    python3 bmp280_cmd_test.py --help
    python3 bmp280_cmd_test.py --led-on
    python3 bmp280_cmd_test.py --test-all
    python3 bmp280_cmd_test.py --blink 5

Requirements:
    - cFS running on localhost or specified host
    - UDP port 1234 accessible (default command port)
"""

import socket
import struct
import time
import argparse
import sys

# =============================================================================
# Configuration
# =============================================================================

# Network Configuration
CFS_HOST = '127.0.0.1'  # Change to your cFS IP if remote
CFS_CMD_PORT = 1234     # Default cFS command port (UDP)

# Message IDs (must match your bmp280_app_msgids.h)
BMP280_APP_CMD_MID = 0x1884

# Command Codes (from bmp280_app_msg.h)
CMD_NOOP = 0
CMD_RESET_COUNTERS = 1
CMD_ENABLE = 2
CMD_DISABLE = 3
CMD_LED_ON = 4
CMD_LED_OFF = 5
CMD_LED_TOGGLE = 6
CMD_LED_STATUS = 7

# cFS Message Header Configuration
CCSDS_VERSION = 0
CCSDS_TYPE = 1  # Command
CCSDS_SEC_HDR = 1
CCSDS_APID = BMP280_APP_CMD_MID & 0x7FF

# =============================================================================
# cFS Command Message Builder
# =============================================================================

class CFSCommand:
    """Build and send cFS command messages"""
    
    def __init__(self, host=CFS_HOST, port=CFS_CMD_PORT):
        self.host = host
        self.port = port
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sequence = 0
        
    def build_ccsds_header(self, msg_id, cmd_code, data_length=0):
        """
        Build CCSDS command header (8 bytes for cFS)
        
        CCSDS Primary Header (6 bytes):
        - Stream ID (2 bytes): Version, Type, Sec Hdr Flag, APID
        - Sequence (2 bytes): Sequence flags, Sequence count
        - Length (2 bytes): Packet data length - 1
        
        CCSDS Command Secondary Header (2 bytes):
        - Function Code (1 byte): Command code
        - Checksum (1 byte): Command checksum
        """
        
        # Primary Header - Stream ID (2 bytes)
        stream_id = (CCSDS_VERSION << 13) | (CCSDS_TYPE << 12) | \
                    (CCSDS_SEC_HDR << 11) | (msg_id & 0x7FF)
        
        # Sequence (2 bytes)
        sequence_flags = 0x3  # Standalone packet
        sequence = (sequence_flags << 14) | (self.sequence & 0x3FFF)
        self.sequence = (self.sequence + 1) & 0x3FFF
        
        # Length (2 bytes) - secondary header (2 bytes) + data - 1
        length = 2 + data_length - 1
        
        # Pack primary header (6 bytes, big-endian)
        primary_header = struct.pack('>HHH', stream_id, sequence, length)
        
        # Secondary Header - Function Code (1 byte) + Checksum (1 byte)
        function_code = cmd_code
        checksum = 0x00
        secondary_header = struct.pack('>BB', function_code, checksum)
        
        return primary_header + secondary_header
    
    def send_command(self, cmd_code, data=b'', verbose=True):
        """Send a command to cFS"""
        
        # Build complete message
        header = self.build_ccsds_header(BMP280_APP_CMD_MID, cmd_code, len(data))
        message = header + data
        
        if verbose:
            print(f"Sending command {cmd_code} to {self.host}:{self.port}")
            print(f"  Message length: {len(message)} bytes")
            print(f"  Hex: {message.hex()}")
        
        try:
            self.sock.sendto(message, (self.host, self.port))
            if verbose:
                print("  ✓ Command sent successfully")
            return True
        except Exception as e:
            print(f"  ✗ Error sending command: {e}")
            return False
    
    def close(self):
        """Close the socket"""
        self.sock.close()

# =============================================================================
# Command Functions
# =============================================================================

def cmd_noop(cfs):
    """Send NOOP command"""
    print("\n--- NOOP Command ---")
    print("Purpose: Test command interface, increment command counter")
    return cfs.send_command(CMD_NOOP)

def cmd_reset_counters(cfs):
    """Send Reset Counters command"""
    print("\n--- Reset Counters Command ---")
    print("Purpose: Reset command and error counters to zero")
    return cfs.send_command(CMD_RESET_COUNTERS)

def cmd_enable_sensor(cfs):
    """Send Enable Sensor command"""
    print("\n--- Enable Sensor Command ---")
    print("Purpose: Enable BMP280 sensor readings")
    return cfs.send_command(CMD_ENABLE)

def cmd_disable_sensor(cfs):
    """Send Disable Sensor command"""
    print("\n--- Disable Sensor Command ---")
    print("Purpose: Disable BMP280 sensor readings")
    return cfs.send_command(CMD_DISABLE)

def cmd_led_on(cfs):
    """Send LED ON command"""
    print("\n--- LED ON Command ---")
    print("Purpose: Turn LED on (GPIO 17 = HIGH)")
    return cfs.send_command(CMD_LED_ON)

def cmd_led_off(cfs):
    """Send LED OFF command"""
    print("\n--- LED OFF Command ---")
    print("Purpose: Turn LED off (GPIO 17 = LOW)")
    return cfs.send_command(CMD_LED_OFF)

def cmd_led_toggle(cfs):
    """Send LED Toggle command"""
    print("\n--- LED Toggle Command ---")
    print("Purpose: Toggle LED state (ON→OFF or OFF→ON)")
    return cfs.send_command(CMD_LED_TOGGLE)

def cmd_led_status(cfs):
    """Send LED Status command"""
    print("\n--- LED Status Command ---")
    print("Purpose: Query current LED state")
    return cfs.send_command(CMD_LED_STATUS)

# =============================================================================
# Test Sequences
# =============================================================================

def test_basic_commands(cfs):
    """Test basic cFS commands"""
    print("\n" + "="*60)
    print("TEST SEQUENCE: Basic Commands")
    print("="*60)
    
    tests = [
        ("NOOP", cmd_noop),
        ("Reset Counters", cmd_reset_counters),
        ("Enable Sensor", cmd_enable_sensor),
        ("Disable Sensor", cmd_disable_sensor),
    ]
    
    results = []
    for name, func in tests:
        success = func(cfs)
        results.append((name, success))
        time.sleep(0.5)
    
    print("\n" + "-"*60)
    print("Basic Commands Test Results:")
    for name, success in results:
        status = "✓ PASS" if success else "✗ FAIL"
        print(f"  {status}: {name}")
    print("-"*60)

def test_led_commands(cfs):
    """Test LED control commands"""
    print("\n" + "="*60)
    print("TEST SEQUENCE: LED Control")
    print("="*60)
    
    tests = [
        ("LED Status (Initial)", cmd_led_status),
        ("LED ON", cmd_led_on),
        ("LED Status (After ON)", cmd_led_status),
        ("LED OFF", cmd_led_off),
        ("LED Status (After OFF)", cmd_led_status),
        ("LED Toggle", cmd_led_toggle),
        ("LED Status (After Toggle)", cmd_led_status),
    ]
    
    results = []
    for name, func in tests:
        print(f"\nStep: {name}")
        success = func(cfs)
        results.append((name, success))
        time.sleep(1.0)
    
    print("\n" + "-"*60)
    print("LED Control Test Results:")
    for name, success in results:
        status = "✓ PASS" if success else "✗ FAIL"
        print(f"  {status}: {name}")
    print("-"*60)

def test_led_blink(cfs, count=5, delay=0.5):
    """Blink LED multiple times"""
    print("\n" + "="*60)
    print(f"TEST SEQUENCE: LED Blink ({count} times)")
    print("="*60)
    
    print(f"\nBlinking LED {count} times (delay={delay}s)")
    
    for i in range(count):
        print(f"\n  Blink {i+1}/{count}")
        cmd_led_on(cfs)
        time.sleep(delay)
        cmd_led_off(cfs)
        time.sleep(delay)
    
    print(f"\n✓ Blink sequence completed")

def test_rapid_toggle(cfs, count=10):
    """Rapidly toggle LED"""
    print("\n" + "="*60)
    print(f"TEST SEQUENCE: Rapid Toggle ({count} times)")
    print("="*60)
    
    print(f"\nToggling LED {count} times")
    
    for i in range(count):
        print(f"  Toggle {i+1}/{count}", end='\r')
        cmd_led_toggle(cfs)
        time.sleep(0.2)
    
    print(f"\n✓ Rapid toggle sequence completed")

def test_all(cfs):
    """Run all test sequences"""
    print("\n" + "="*60)
    print("RUNNING ALL TESTS")
    print("="*60)
    
    test_basic_commands(cfs)
    time.sleep(2)
    test_led_commands(cfs)
    time.sleep(2)
    test_led_blink(cfs, count=3, delay=0.5)
    time.sleep(2)
    
    print("\n" + "="*60)
    print("ALL TESTS COMPLETED")
    print("="*60)

# =============================================================================
# Interactive Mode
# =============================================================================

def interactive_menu(cfs):
    """Interactive command menu"""
    while True:
        print("\n" + "="*60)
        print("BMP280 cFS Command Test - Interactive Mode")
        print("="*60)
        print("\nBasic Commands:")
        print("  0. NOOP")
        print("  1. Reset Counters")
        print("  2. Enable Sensor")
        print("  3. Disable Sensor")
        print("\nLED Commands:")
        print("  4. LED ON")
        print("  5. LED OFF")
        print("  6. LED Toggle")
        print("  7. LED Status")
        print("\nTest Sequences:")
        print("  b. Blink LED (5 times)")
        print("  t. Test All LED Commands")
        print("  a. Test All Commands")
        print("  r. Rapid Toggle (10 times)")
        print("\nOther:")
        print("  q. Quit")
        print("-"*60)
        
        choice = input("\nEnter command: ").strip().lower()
        
        if choice == '0':
            cmd_noop(cfs)
        elif choice == '1':
            cmd_reset_counters(cfs)
        elif choice == '2':
            cmd_enable_sensor(cfs)
        elif choice == '3':
            cmd_disable_sensor(cfs)
        elif choice == '4':
            cmd_led_on(cfs)
        elif choice == '5':
            cmd_led_off(cfs)
        elif choice == '6':
            cmd_led_toggle(cfs)
        elif choice == '7':
            cmd_led_status(cfs)
        elif choice == 'b':
            test_led_blink(cfs)
        elif choice == 't':
            test_led_commands(cfs)
        elif choice == 'a':
            test_all(cfs)
        elif choice == 'r':
            test_rapid_toggle(cfs)
        elif choice == 'q':
            print("\nExiting...")
            break
        else:
            print(f"\nInvalid choice: {choice}")
        
        time.sleep(0.5)

# =============================================================================
# Main
# =============================================================================

def main():
    parser = argparse.ArgumentParser(
        description='Send commands to BMP280 cFS application',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s --led-on                  # Turn LED on
  %(prog)s --led-off                 # Turn LED off
  %(prog)s --blink 10                # Blink LED 10 times
  %(prog)s --test-all                # Run all tests
  %(prog)s --interactive             # Interactive mode
  %(prog)s --host 192.168.1.100      # Connect to remote cFS
        """
    )
    
    # Connection options
    parser.add_argument('--host', default=CFS_HOST,
                       help=f'cFS host IP (default: {CFS_HOST})')
    parser.add_argument('--port', type=int, default=CFS_CMD_PORT,
                       help=f'cFS command port (default: {CFS_CMD_PORT})')
    
    # Individual commands
    parser.add_argument('--noop', action='store_true',
                       help='Send NOOP command')
    parser.add_argument('--reset', action='store_true',
                       help='Send Reset Counters command')
    parser.add_argument('--enable', action='store_true',
                       help='Send Enable Sensor command')
    parser.add_argument('--disable', action='store_true',
                       help='Send Disable Sensor command')
    parser.add_argument('--led-on', action='store_true',
                       help='Send LED ON command')
    parser.add_argument('--led-off', action='store_true',
                       help='Send LED OFF command')
    parser.add_argument('--led-toggle', action='store_true',
                       help='Send LED Toggle command')
    parser.add_argument('--led-status', action='store_true',
                       help='Send LED Status command')
    
    # Test sequences
    parser.add_argument('--blink', type=int, metavar='N',
                       help='Blink LED N times')
    parser.add_argument('--rapid-toggle', type=int, metavar='N',
                       help='Rapidly toggle LED N times')
    parser.add_argument('--test-basic', action='store_true',
                       help='Test basic commands')
    parser.add_argument('--test-led', action='store_true',
                       help='Test LED commands')
    parser.add_argument('--test-all', action='store_true',
                       help='Run all test sequences')
    
    # Interactive mode
    parser.add_argument('--interactive', '-i', action='store_true',
                       help='Interactive menu mode')
    
    args = parser.parse_args()
    
    # Create CFS command interface
    cfs = CFSCommand(args.host, args.port)
    
    print("="*60)
    print("BMP280 cFS Command Test Script")
    print("="*60)
    print(f"Target: {args.host}:{args.port}")
    print(f"Message ID: 0x{BMP280_APP_CMD_MID:04X}")
    print("="*60)
    
    try:
        # Execute commands based on arguments
        executed = False
        
        if args.noop:
            cmd_noop(cfs)
            executed = True
        if args.reset:
            cmd_reset_counters(cfs)
            executed = True
        if args.enable:
            cmd_enable_sensor(cfs)
            executed = True
        if args.disable:
            cmd_disable_sensor(cfs)
            executed = True
        if args.led_on:
            cmd_led_on(cfs)
            executed = True
        if args.led_off:
            cmd_led_off(cfs)
            executed = True
        if args.led_toggle:
            cmd_led_toggle(cfs)
            executed = True
        if args.led_status:
            cmd_led_status(cfs)
            executed = True
        if args.blink:
            test_led_blink(cfs, args.blink)
            executed = True
        if args.rapid_toggle:
            test_rapid_toggle(cfs, args.rapid_toggle)
            executed = True
        if args.test_basic:
            test_basic_commands(cfs)
            executed = True
        if args.test_led:
            test_led_commands(cfs)
            executed = True
        if args.test_all:
            test_all(cfs)
            executed = True
        if args.interactive:
            interactive_menu(cfs)
            executed = True
        
        if not executed:
            # No arguments provided, show help
            parser.print_help()
            print("\nNo commands specified. Use --help for options or --interactive for menu.")
            sys.exit(1)
    
    except KeyboardInterrupt:
        print("\n\nInterrupted by user")
    finally:
        cfs.close()
        print("\nConnection closed.")

if __name__ == '__main__':
    main()

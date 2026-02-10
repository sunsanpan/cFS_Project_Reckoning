# Python Command Testing Scripts - Usage Guide

## Overview

Two Python scripts are provided for sending commands to your BMP280 cFS application:

1. **`bmp280_cmd_test.py`** - Full-featured testing tool with multiple modes
2. **`simple_cmd.py`** - Simple, quick command sender

Both scripts send UDP packets to cFS using the standard command interface.

## Script 1: bmp280_cmd_test.py (Full Featured)

### Features
- Individual command sending
- Automated test sequences
- Interactive menu mode
- Detailed logging and feedback
- Configurable host/port
- Blink patterns and rapid toggle tests

### Installation

No special dependencies required - uses only Python standard library:
```bash
# Make executable
chmod +x bmp280_cmd_test.py

# Or run with python3
python3 bmp280_cmd_test.py --help
```

### Basic Usage

#### Single Commands

```bash
# Turn LED on
python3 bmp280_cmd_test.py --led-on

# Turn LED off
python3 bmp280_cmd_test.py --led-off

# Toggle LED
python3 bmp280_cmd_test.py --led-toggle

# Query LED status
python3 bmp280_cmd_test.py --led-status

# Send NOOP
python3 bmp280_cmd_test.py --noop

# Reset counters
python3 bmp280_cmd_test.py --reset

# Enable sensor
python3 bmp280_cmd_test.py --enable

# Disable sensor
python3 bmp280_cmd_test.py --disable
```

#### Test Sequences

```bash
# Blink LED 5 times
python3 bmp280_cmd_test.py --blink 5

# Blink LED 10 times
python3 bmp280_cmd_test.py --blink 10

# Rapidly toggle 20 times
python3 bmp280_cmd_test.py --rapid-toggle 20

# Test basic commands
python3 bmp280_cmd_test.py --test-basic

# Test LED commands
python3 bmp280_cmd_test.py --test-led

# Test everything
python3 bmp280_cmd_test.py --test-all
```

#### Interactive Mode

```bash
# Start interactive menu
python3 bmp280_cmd_test.py --interactive

# Or shorthand
python3 bmp280_cmd_test.py -i
```

Interactive menu provides:
```
BMP280 cFS Command Test - Interactive Mode
============================================================

Basic Commands:
  0. NOOP
  1. Reset Counters
  2. Enable Sensor
  3. Disable Sensor

LED Commands:
  4. LED ON
  5. LED OFF
  6. LED Toggle
  7. LED Status

Test Sequences:
  b. Blink LED (5 times)
  t. Test All LED Commands
  a. Test All Commands
  r. Rapid Toggle (10 times)

Other:
  q. Quit
```

#### Remote cFS

```bash
# Connect to remote cFS
python3 bmp280_cmd_test.py --host 192.168.1.100 --led-on

# Custom port
python3 bmp280_cmd_test.py --host 192.168.1.100 --port 5000 --led-on
```

### Example Session

```bash
# Complete LED test
$ python3 bmp280_cmd_test.py --test-led

============================================================
TEST SEQUENCE: LED Control
============================================================

Step: LED Status (Initial)

--- LED Status Command ---
Purpose: Query current LED state
Sending command 7 to 127.0.0.1:1234
  Message length: 14 bytes
  Hex: 18821c0000071000000000000000
  ✓ Command sent successfully

Step: LED ON

--- LED ON Command ---
Purpose: Turn LED on (GPIO 17 = HIGH)
Sending command 4 to 127.0.0.1:1234
  Message length: 14 bytes
  Hex: 18821c0000041000000000000000
  ✓ Command sent successfully

[... continues with all tests ...]

------------------------------------------------------------
LED Control Test Results:
  ✓ PASS: LED Status (Initial)
  ✓ PASS: LED ON
  ✓ PASS: LED Status (After ON)
  ✓ PASS: LED OFF
  ✓ PASS: LED Status (After OFF)
  ✓ PASS: LED Toggle
  ✓ PASS: LED Status (After Toggle)
------------------------------------------------------------
```

---

## Script 2: simple_cmd.py (Quick & Easy)

### Features
- Minimal code, easy to understand
- Quick single commands
- Built-in blink and test sequences
- No command-line arguments needed

### Usage

```bash
# Make executable
chmod +x simple_cmd.py

# Run commands
python3 simple_cmd.py led-on
python3 simple_cmd.py led-off
python3 simple_cmd.py toggle
python3 simple_cmd.py status
```

### Available Commands

```bash
python3 simple_cmd.py noop      # NOOP command
python3 simple_cmd.py reset     # Reset counters
python3 simple_cmd.py enable    # Enable sensor
python3 simple_cmd.py disable   # Disable sensor
python3 simple_cmd.py led-on    # LED on
python3 simple_cmd.py led-off   # LED off
python3 simple_cmd.py toggle    # Toggle LED
python3 simple_cmd.py status    # Query LED status

# Special sequences
python3 simple_cmd.py blink     # Blink 5 times
python3 simple_cmd.py test      # Test all LED commands
```

### Example Session

```bash
$ python3 simple_cmd.py led-on
Sending command code 4 to 127.0.0.1:1234
✓ Sent

$ python3 simple_cmd.py blink
Blinking LED 5 times...
Blink 1/5
Sending command code 4 to 127.0.0.1:1234
✓ Sent
Sending command code 5 to 127.0.0.1:1234
✓ Sent
Blink 2/5
...
Done!
```

---

## Configuration

### Changing Target IP/Port

**bmp280_cmd_test.py:**
```bash
# Command line
python3 bmp280_cmd_test.py --host 192.168.1.50 --port 1234 --led-on

# Or edit the script
CFS_HOST = '192.168.1.50'
CFS_CMD_PORT = 1234
```

**simple_cmd.py:**
```python
# Edit at top of file
CFS_IP = '192.168.1.50'
CFS_CMD_PORT = 1234
```

### Changing Message IDs

If your cFS uses different message IDs, edit the scripts:

```python
# In bmp280_cmd_test.py or simple_cmd.py
BMP280_APP_CMD_MID = 0x1882  # Change to your MID
```

Check your actual MID in `bmp280_app_msgids.h`:
```c
#define BMP280_APP_CMD_MID  0x1882
```

---

## Monitoring Results

### Monitor cFS Events

In a separate terminal:
```bash
# Monitor system log
tail -f /var/log/syslog | grep BMP280

# Or check cFS event log
tail -f $CFS_PROJECT_DIR/build/exe/cpu1/cf/cfe_evs.log
```

### Expected Events

When you send LED ON:
```
BMP280: LED turned ON
```

When you send LED OFF:
```
BMP280: LED turned OFF
```

When you send Toggle:
```
BMP280: LED toggled to ON
```
or
```
BMP280: LED toggled to OFF
```

When you send Status:
```
BMP280: LED status is ON
```
or
```
BMP280: LED status is OFF
```

### Monitor Telemetry

If you have TO_LAB running, you can see housekeeping telemetry that includes LED state.

---

## Troubleshooting

### Commands Not Reaching cFS

**Problem:** Script says "sent" but nothing happens in cFS

**Solutions:**
1. Check cFS is running:
   ```bash
   ps aux | grep core-cpu1
   ```

2. Check firewall:
   ```bash
   sudo ufw allow 1234/udp
   ```

3. Verify port is correct:
   ```bash
   netstat -anu | grep 1234
   ```

4. Check message ID matches:
   ```bash
   grep CMD_MID bmp280_app/fsw/inc/bmp280_app_msgids.h
   ```

### LED Not Responding

**Problem:** Commands are received but LED doesn't change

**Check:**
1. GPIO initialization succeeded (check events)
2. LED is wired correctly
3. Run manual GPIO test:
   ```bash
   echo 1 > /sys/class/gpio/gpio17/value
   ```

### Network Issues

**Problem:** Connection refused or timeout

**Solutions:**
1. Check cFS IP address:
   ```bash
   ifconfig  # or ip addr
   ```

2. Test network connectivity:
   ```bash
   ping <cFS_IP>
   ```

3. Verify UDP port is open:
   ```bash
   sudo netstat -lunp | grep 1234
   ```

---

## Advanced Usage

### Automated Testing Script

Create a bash wrapper for automated testing:

```bash
#!/bin/bash
# auto_test.sh

echo "Starting automated LED test..."

# Basic test
python3 bmp280_cmd_test.py --test-basic
sleep 2

# LED test
python3 bmp280_cmd_test.py --test-led
sleep 2

# Blink test
python3 bmp280_cmd_test.py --blink 10

echo "Test complete!"
```

### Continuous Monitoring

Monitor commands and responses:

```bash
#!/bin/bash
# monitor.sh

# Terminal 1: Send commands
watch -n 2 'python3 simple_cmd.py toggle'

# Terminal 2: Monitor events
tail -f /var/log/syslog | grep BMP280
```

### Stress Testing

Test rapid command processing:

```bash
# Send 100 rapid toggles
python3 bmp280_cmd_test.py --rapid-toggle 100

# Or with simple script
for i in {1..100}; do
  python3 simple_cmd.py toggle
  sleep 0.1
done
```

---

## Script Architecture

### bmp280_cmd_test.py Structure

```
CFSCommand class
├── build_ccsds_header()  - Builds CCSDS packet header
├── send_command()         - Sends UDP packet to cFS
└── close()                - Cleanup

Command Functions
├── cmd_noop()
├── cmd_reset_counters()
├── cmd_enable_sensor()
├── cmd_disable_sensor()
├── cmd_led_on()
├── cmd_led_off()
├── cmd_led_toggle()
└── cmd_led_status()

Test Sequences
├── test_basic_commands()
├── test_led_commands()
├── test_led_blink()
├── test_rapid_toggle()
└── test_all()

Interactive Mode
└── interactive_menu()
```

### Packet Format

Both scripts build CCSDS command packets:

```
Byte 0-1:   Stream ID (Message ID with flags)
Byte 2-3:   Sequence count
Byte 4-5:   Packet length
Byte 6:     Command code (function code)
Byte 7:     Checksum
Byte 8-13:  Padding/Reserved
```

---

## Quick Reference

### Common Commands

| Action | bmp280_cmd_test.py | simple_cmd.py |
|--------|-------------------|---------------|
| LED On | `--led-on` | `led-on` |
| LED Off | `--led-off` | `led-off` |
| Toggle | `--led-toggle` | `toggle` |
| Blink 5x | `--blink 5` | `blink` |
| Test All | `--test-all` | `test` |
| Interactive | `--interactive` | N/A |

### Exit Codes

- `0` - Success
- `1` - Error or no commands specified

### Keyboard Shortcuts

Interactive mode:
- `q` - Quit
- `Ctrl+C` - Interrupt/Exit

---

## Tips & Best Practices

1. **Start Simple**: Use `simple_cmd.py` first to verify basic connectivity

2. **Monitor Events**: Always have a terminal monitoring cFS events

3. **Visual Feedback**: The LED provides immediate visual confirmation

4. **Test Incrementally**: 
   - First: Single command (`led-on`)
   - Then: Toggle test
   - Finally: Automated sequences

5. **Use Interactive Mode**: Great for manual testing and exploration

6. **Check Housekeeping**: Verify command counters increment in HK telemetry

7. **Log Everything**: Redirect output for debugging:
   ```bash
   python3 bmp280_cmd_test.py --test-all 2>&1 | tee test_log.txt
   ```

---

## Extending the Scripts

### Adding New Commands

To add new commands, update both scripts:

```python
# Add to CMD_CODES dictionary
CMD_CODES = {
    # ... existing codes ...
    'my-new-cmd': 8,  # Next available command code
}

# Add command function (bmp280_cmd_test.py)
def cmd_my_new_command(cfs):
    print("\n--- My New Command ---")
    return cfs.send_command(8)  # Command code
```

### Custom Test Sequences

Add custom sequences in `bmp280_cmd_test.py`:

```python
def test_custom_pattern(cfs):
    """Custom LED pattern"""
    pattern = [
        (CMD_LED_ON, 0.1),
        (CMD_LED_OFF, 0.2),
        (CMD_LED_ON, 0.3),
        (CMD_LED_OFF, 0.1),
    ]
    
    for cmd, delay in pattern:
        cfs.send_command(cmd)
        time.sleep(delay)
```

---

## Summary

- **Quick Testing**: Use `simple_cmd.py`
- **Comprehensive Testing**: Use `bmp280_cmd_test.py`
- **Interactive Exploration**: Use `--interactive` mode
- **Automated Testing**: Use `--test-all` or create scripts
- **Always Monitor**: Check cFS events for confirmation

Both scripts are ready to use with no external dependencies!

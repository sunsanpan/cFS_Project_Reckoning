# UART Telemetry Output Feature

## Overview

This enhancement adds a second UART interface to the FSWV1 cFS application for transmitting telemetry data. The telemetry data that is normally sent via UDP to port 5555 is now also transmitted via UART.

## Key Features

- **Dual Telemetry Transmission**: Telemetry is sent to both UDP (existing) and UART (new)
- **Separate UART Ports**: Uses a different UART than the IMU input to avoid conflicts
  - IMU Input UART: `/dev/ttyAMA0` (receives IMU data)
  - Telemetry Output UART: `/dev/ttyS0` (transmits telemetry data)
- **Format Options**: Supports both ASCII and binary CCSDS formats
- **Non-blocking**: UART initialization failure doesn't prevent app startup

## Files Modified

### 1. **fsw/inc/fswv1_app.h**
   - Added function prototypes:
     - `FSWV1_InitTelemetryUART()`
     - `FSWV1_SendTelemetryUART()`
     - `FSWV1_CloseTelemetryUART()`
   - Added new event IDs:
     - `FSWV1_APP_UART_TELEMETRY_INIT_INF_EID` (20)
     - `FSWV1_APP_UART_TELEMETRY_ERR_EID` (21)

### 2. **fsw/src/fswv1_app.c**
   - Added `FSWV1_InitTelemetryUART()` call in initialization
   - Added `FSWV1_SendTelemetryUART()` call in main loop (alongside UDP send)
   - Added `FSWV1_CloseTelemetryUART()` call in cleanup

### 3. **fsw/src/fswv1_uart_telemetry.c** (NEW FILE)
   - Complete implementation of telemetry UART functionality
   - Configurable UART device and baud rate
   - Support for both ASCII and binary formats

### 4. **CMakeLists.txt**
   - Added `fswv1_uart_telemetry.c` to the build sources

## Configuration

### UART Device Selection

The telemetry UART device can be configured in `fswv1_uart_telemetry.c`:

```c
#define TELEMETRY_UART_DEVICE "/dev/ttyS0"  /* Change this to match your hardware */
```

**Common UART Options:**
- `/dev/ttyAMA0` - Primary UART (GPIO 14/15) on Raspberry Pi (used for IMU)
- `/dev/ttyAMA1` - Secondary UART on Raspberry Pi (if available)
- `/dev/ttyS0` - Mini UART or alternative UART
- `/dev/ttyUSB0` - USB-to-Serial adapter
- `/dev/ttyUSB1` - Another USB-to-Serial adapter

### Baud Rate

The baud rate is set to 115200 by default:

```c
#define TELEMETRY_UART_BAUDRATE B115200
```

### Output Format

You can choose between ASCII and binary formats:

```c
#define TELEMETRY_ASCII_FORMAT 1  /* Set to 1 for ASCII, 0 for binary */
```

#### ASCII Format (Default)
```
BMP:T=25.30,P=101325.00 IMU:Ax=0.05,Ay=-0.12,Az=9.81,Gx=0.01,Gy=-0.02,Gz=0.00,T=25.50 TS=1234567890\n
```

**Fields:**
- `BMP:T` - BMP280 temperature (°C)
- `BMP:P` - BMP280 pressure (Pa)
- `IMU:Ax,Ay,Az` - Accelerometer X, Y, Z axes
- `IMU:Gx,Gy,Gz` - Gyroscope X, Y, Z axes
- `IMU:T` - IMU temperature (°C)
- `TS` - Timestamp (seconds)

#### Binary Format
Sends the complete CCSDS telemetry packet (same as UDP transmission). This includes:
- CCSDS Primary Header (6 bytes)
- CCSDS Secondary Header (timestamp)
- Telemetry Payload (all sensor data)

Total packet size: `sizeof(FSWV1_APP_CombinedTlm_t)`

## Hardware Setup

### Raspberry Pi Example

If using Raspberry Pi with two UARTs:

1. **IMU UART (existing)**: `/dev/ttyAMA0` (GPIO 14/15)
   - Receives IMU data from external device
   
2. **Telemetry UART (new)**: `/dev/ttyS0` or `/dev/ttyAMA1`
   - Transmits telemetry to external device/ground station

### Enable Additional UARTs on Raspberry Pi

Add to `/boot/config.txt`:
```
# Enable second UART
dtoverlay=uart1
dtoverlay=uart2
```

Or use USB-to-Serial adapters (recommended for testing):
- Connect USB-to-Serial adapter
- Device appears as `/dev/ttyUSB0` or `/dev/ttyUSB1`

## Testing

### 1. Using screen (ASCII format)

```bash
# Connect to the telemetry UART to view output
screen /dev/ttyS0 115200
```

You should see output like:
```
BMP:T=25.30,P=101325.00 IMU:Ax=0.05,Ay=-0.12,Az=9.81,Gx=0.01,Gy=-0.02,Gz=0.00,T=25.50 TS=1234567890
BMP:T=25.31,P=101326.00 IMU:Ax=0.06,Ay=-0.11,Az=9.80,Gx=0.02,Gy=-0.01,Gz=0.01,T=25.51 TS=1234567891
```

### 2. Using Python script (ASCII format)

```python
#!/usr/bin/env python3
import serial

# Open telemetry UART
ser = serial.Serial('/dev/ttyS0', 115200, timeout=1)

print("Listening for telemetry data...")
try:
    while True:
        line = ser.readline().decode('utf-8').strip()
        if line:
            print(f"Received: {line}")
except KeyboardInterrupt:
    print("\nStopped")
finally:
    ser.close()
```

### 3. Using Python script (Binary format)

```python
#!/usr/bin/env python3
import serial
import struct

# Open telemetry UART
ser = serial.Serial('/dev/ttyS0', 115200, timeout=1)

# Size of CCSDS packet (adjust based on actual size)
PACKET_SIZE = 64  # Update this based on sizeof(FSWV1_APP_CombinedTlm_t)

print("Listening for telemetry packets...")
try:
    while True:
        data = ser.read(PACKET_SIZE)
        if len(data) == PACKET_SIZE:
            # Parse CCSDS header and payload
            # (Implementation depends on packet structure)
            print(f"Received {len(data)} bytes")
except KeyboardInterrupt:
    print("\nStopped")
finally:
    ser.close()
```

## Verification

After compiling and running the modified cFS application:

1. **Check initialization messages**:
   ```
   FSWV1_TELEMETRY_UART: Initializing telemetry UART on /dev/ttyS0 at 115200 baud...
   FSWV1_TELEMETRY_UART: Telemetry UART initialized on /dev/ttyS0 at 115200 baud
   FSWV1_TELEMETRY_UART: Ready to transmit telemetry data
   ```

2. **Verify telemetry transmission**:
   - Connect to the UART using screen or Python script
   - You should see telemetry data at approximately 1 Hz (default rate)

3. **Check for errors**:
   - If UART fails to open, app will log an error but continue running
   - UDP telemetry will still work even if UART fails

## Troubleshooting

### Permission Denied Error

```bash
# Add your user to the dialout group
sudo usermod -a -G dialout $USER

# Reboot or logout/login for changes to take effect
```

### UART Device Not Found

1. Check available UART devices:
   ```bash
   ls -l /dev/tty*
   ```

2. Verify UART is enabled in device tree (Raspberry Pi):
   ```bash
   dtoverlay -a | grep uart
   ```

3. Try a USB-to-Serial adapter as alternative

### No Data Received

1. Check baud rate matches on both ends
2. Verify correct UART device is configured
3. Check wiring (TX → RX, RX → TX, GND → GND)
4. Monitor system logs: `dmesg | grep tty`

## Integration with Ground Systems

The binary CCSDS format option allows direct integration with ground systems that expect CCSDS telemetry packets:

- Use the same packet processing as UDP telemetry
- Same message ID (`FSWV1_APP_COMBINED_TLM_MID`)
- Same packet structure as defined in `fswv1_app_msg.h`

## Performance Considerations

- **Non-blocking writes**: UART writes use non-blocking mode with timeout
- **Error suppression**: Write errors are logged every 100 occurrences to avoid log spam
- **Minimal overhead**: UART transmission happens after UDP, so it doesn't delay UDP packets
- **Independent operation**: UART failure doesn't affect UDP telemetry or app operation

## Summary of Changes

1. ✅ Added new UART for telemetry output (separate from IMU UART)
2. ✅ Telemetry sent to both UDP and UART simultaneously
3. ✅ Configurable UART device and format
4. ✅ Non-fatal initialization (app continues if UART unavailable)
5. ✅ Proper cleanup on app exit
6. ✅ Event logging for initialization and errors

## Future Enhancements

Possible improvements:
- Command to enable/disable UART telemetry at runtime
- Configurable baud rate via command
- Statistics on UART transmission success/failure
- Support for multiple telemetry destinations

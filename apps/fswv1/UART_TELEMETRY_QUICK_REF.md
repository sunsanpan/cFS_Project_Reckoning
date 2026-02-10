# UART Telemetry Quick Reference Card

## Quick Start

### 1. Configure UART Device
Edit `fsw/src/fswv1_uart_telemetry.c` line 29:
```c
#define TELEMETRY_UART_DEVICE "/dev/ttyS0"  // Change this line
```

**Common Options:**
- `/dev/ttyS0` - Mini UART (Raspberry Pi)
- `/dev/ttyAMA1` - Secondary UART (Raspberry Pi)  
- `/dev/ttyUSB0` - USB-to-Serial adapter

### 2. Choose Output Format
Edit `fsw/src/fswv1_uart_telemetry.c` line 36:
```c
#define TELEMETRY_ASCII_FORMAT 1  // 1=ASCII, 0=Binary
```

### 3. Build and Run
```bash
cd /path/to/cfs
make
make install
./core-cpu1
```

### 4. View Telemetry
```bash
# Simple viewing
screen /dev/ttyS0 115200

# OR with Python script
python3 uart_telemetry_receiver.py
```

## Hardware Connections

### Raspberry Pi UART Pins
| UART | TX Pin | RX Pin | Function |
|------|--------|--------|----------|
| ttyAMA0 | GPIO 14 | GPIO 15 | IMU Input (used) |
| ttyS0 | GPIO 14 | GPIO 15 | Mini UART (available) |
| ttyAMA1 | GPIO 0 | GPIO 1 | Available (if enabled) |

### Enable Additional UARTs
Add to `/boot/config.txt`:
```
dtoverlay=uart1
dtoverlay=uart2
```

### USB-to-Serial (Easiest for Testing)
- Just plug in USB-to-Serial adapter
- Device appears as `/dev/ttyUSB0`
- No GPIO configuration needed
- Works on any Linux system

## Telemetry Format Examples

### ASCII Format Output
```
BMP:T=25.30,P=101325.00 IMU:Ax=0.05,Ay=-0.12,Az=9.81,Gx=0.01,Gy=-0.02,Gz=0.00,T=25.50 TS=1234567890
```

### Field Meanings
- `BMP:T` = BMP280 Temperature (°C)
- `BMP:P` = BMP280 Pressure (Pa)
- `IMU:Ax,Ay,Az` = Accelerometer X,Y,Z
- `IMU:Gx,Gy,Gz` = Gyroscope X,Y,Z  
- `IMU:T` = IMU Temperature (°C)
- `TS` = Timestamp (seconds)

## Troubleshooting

### Permission Error
```bash
sudo usermod -a -G dialout $USER
logout  # or reboot
```

### Check Available UARTs
```bash
ls -l /dev/tty*
```

### Check if UART is Open
```bash
lsof | grep tty
```

### Test UART Loopback
```bash
# Connect TX to RX on UART
echo "test" > /dev/ttyS0
cat /dev/ttyS0
```

## Key Files Modified

| File | Purpose |
|------|---------|
| `fsw/src/fswv1_uart_telemetry.c` | UART implementation (NEW) |
| `fsw/inc/fswv1_app.h` | Function prototypes |
| `fsw/src/fswv1_app.c` | Initialization & send calls |
| `CMakeLists.txt` | Build configuration |

## Default Configuration

```c
Device:    /dev/ttyS0
Baud Rate: 115200
Format:    ASCII
Mode:      8N1 (8 data bits, no parity, 1 stop bit)
```

## Integration Points

### In `FSWV1_APP_Init()`:
```c
FSWV1_InitTelemetryUART();  // Line ~253
```

### In Main Loop:
```c
FSWV1_SendTelemetryUART(&SensorData, &IMUData);  // Line ~122
```

### In Cleanup:
```c
FSWV1_CloseTelemetryUART();  // Line ~134
```

## Verification Checklist

- [ ] UART device exists: `ls -l /dev/ttyS0`
- [ ] User has permissions: `groups | grep dialout`
- [ ] cFS shows init message: "Telemetry UART initialized"
- [ ] Data visible on UART: `screen /dev/ttyS0 115200`
- [ ] Data rate is ~1 Hz
- [ ] UDP telemetry still works

## Configuration Matrix

| Use Case | UART Device | Format | Notes |
|----------|-------------|--------|-------|
| Pi GPIO | `/dev/ttyS0` | ASCII | Easiest, uses mini UART |
| Pi Secondary | `/dev/ttyAMA1` | ASCII | Requires dtoverlay |
| USB Adapter | `/dev/ttyUSB0` | ASCII | Best for testing |
| Ground Station | `/dev/ttyS0` | Binary | CCSDS format |
| Debugging | `/dev/ttyUSB0` | ASCII | Human readable |

## Performance

- **Rate**: 1 Hz (matches sensor read rate)
- **Packet Size**: 
  - ASCII: ~150 bytes/packet
  - Binary: ~64 bytes/packet
- **CPU Impact**: < 0.1%
- **Latency**: Non-blocking, < 1ms

## Support

- Full documentation: `UART_TELEMETRY_GUIDE.md`
- Change summary: `CHANGES_UART_TELEMETRY.md`
- Test script: `uart_telemetry_receiver.py`

---
**Important**: This feature is independent of UDP telemetry. If UART initialization fails, the app continues normally with UDP-only operation.

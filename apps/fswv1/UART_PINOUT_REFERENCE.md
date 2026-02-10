# UART Pinout Reference for FSWV1 Application

## Overview

The FSWV1 application uses **TWO separate UARTs**:
1. **IMU Input UART** - Receives IMU sensor data
2. **Telemetry Output UART** - Transmits telemetry data

## Raspberry Pi UART Pinout

### Primary UART (ttyAMA0) - IMU Input
**Device**: `/dev/ttyAMA0`  
**Function**: Receives IMU data from external sensor  
**Configuration**: 115200 baud, 8N1

| Signal | GPIO Pin | Physical Pin | Direction | Description |
|--------|----------|--------------|-----------|-------------|
| TX     | GPIO 14  | Pin 8        | Output    | Transmit (not used for IMU) |
| RX     | GPIO 15  | Pin 10       | Input     | **Receive IMU data** ✓ |
| GND    | GND      | Pin 6/14/20  | -         | Ground reference |

**Wiring for IMU Input:**
```
IMU Device          Raspberry Pi
─────────────────   ──────────────────────
TX (IMU)     ──────> GPIO 15 (Pin 10) RX
GND          ──────> GND (Pin 6)
VCC (3.3V)   ──────> 3.3V (Pin 1)
```

### Secondary UART (ttyS0 - Mini UART) - Telemetry Output
**Device**: `/dev/ttyS0`  
**Function**: Transmits telemetry data  
**Configuration**: 115200 baud, 8N1

| Signal | GPIO Pin | Physical Pin | Direction | Description |
|--------|----------|--------------|-----------|-------------|
| TX     | GPIO 14  | Pin 8        | Output    | **Transmit telemetry** ✓ |
| RX     | GPIO 15  | Pin 10       | Input     | Receive (not used) |
| GND    | GND      | Pin 6/14/20  | -         | Ground reference |

**Note**: By default, mini UART (ttyS0) shares the same physical pins as ttyAMA0!

**Wiring for Telemetry Output:**
```
Raspberry Pi        Ground Station/Device
──────────────────  ─────────────────────
GPIO 14 (Pin 8) TX ──────> RX
GND (Pin 6)        ──────> GND
```

## ⚠️ IMPORTANT: Pin Conflict Resolution

### The Problem
Both `ttyAMA0` and `ttyS0` use the **same physical GPIO pins** (14 & 15) by default on Raspberry Pi!

### Solutions

#### Option 1: Use Different Hardware UARTs (RECOMMENDED)
Enable additional UART interfaces that use different GPIO pins.

Add to `/boot/config.txt`:
```
# Enable UART1 on GPIO 0/1
dtoverlay=uart1

# Enable UART2 on GPIO 0/1  
dtoverlay=uart2

# Enable UART3 on GPIO 4/5
dtoverlay=uart3

# Enable UART4 on GPIO 8/9
dtoverlay=uart4

# Enable UART5 on GPIO 12/13
dtoverlay=uart5
```

**Recommended Configuration:**
- **IMU Input**: `/dev/ttyAMA0` (GPIO 14/15, Pin 8/10)
- **Telemetry Output**: `/dev/ttyAMA1` (GPIO 0/1, Pin 27/28)

#### Option 2: Use USB-to-Serial Adapter (EASIEST FOR TESTING)
This is the **simplest solution** and recommended for initial testing.

**Advantages:**
- No GPIO pin conflicts
- Works on any Linux system
- Hot-pluggable
- Easy to test

**Wiring:**
```
USB-to-Serial       Raspberry Pi
─────────────────   ──────────────
Appears as /dev/ttyUSB0
No GPIO connections needed - just plug into USB port
```

**Configuration:**
```c
// In fswv1_uart_telemetry.c
#define TELEMETRY_UART_DEVICE "/dev/ttyUSB0"  // USB adapter
```

#### Option 3: Software Serial (Not Recommended)
Use bit-banging on different GPIO pins. Not recommended for reliable telemetry.

## Complete Pin Reference Tables

### Raspberry Pi 40-Pin Header

```
      3.3V  [ 1] [ 2]  5V
 GPIO  2  [ 3] [ 4]  5V
 GPIO  3  [ 5] [ 6]  GND
 GPIO  4  [ 7] [ 8]  GPIO 14 (UART0 TX / Mini UART TX) ← SHARED!
       GND  [ 9] [10]  GPIO 15 (UART0 RX / Mini UART RX) ← SHARED!
GPIO 17  [11] [12]  GPIO 18
GPIO 27  [13] [14]  GND
GPIO 22  [15] [16]  GPIO 23
     3.3V  [17] [18]  GPIO 24
GPIO 10  [19] [20]  GND
 GPIO  9  [21] [22]  GPIO 25
GPIO 11  [23] [24]  GPIO 8
       GND  [25] [26]  GPIO 7
 GPIO  0  [27] [28]  GPIO 1  ← UART1 available here!
 GPIO  5  [29] [30]  GND
 GPIO  6  [31] [32]  GPIO 12
GPIO 13  [33] [34]  GND
GPIO 19  [35] [36]  GPIO 16
GPIO 26  [37] [38]  GPIO 20
       GND  [39] [40]  GPIO 21
```

### All Available UART Pins on Raspberry Pi

| UART Device | TX Pin | RX Pin | Enable Method |
|-------------|--------|--------|---------------|
| ttyAMA0 (Primary) | GPIO 14 (Pin 8) | GPIO 15 (Pin 10) | Enabled by default |
| ttyS0 (Mini UART) | GPIO 14 (Pin 8) | GPIO 15 (Pin 10) | Shares with ttyAMA0! |
| ttyAMA1 (UART1) | GPIO 0 (Pin 27) | GPIO 1 (Pin 28) | dtoverlay=uart1 |
| ttyAMA2 (UART2) | GPIO 0 (Pin 27) | GPIO 1 (Pin 28) | dtoverlay=uart2 |
| ttyAMA3 (UART3) | GPIO 4 (Pin 7) | GPIO 5 (Pin 29) | dtoverlay=uart3 |
| ttyAMA4 (UART4) | GPIO 8 (Pin 24) | GPIO 9 (Pin 21) | dtoverlay=uart4 |
| ttyAMA5 (UART5) | GPIO 12 (Pin 32) | GPIO 13 (Pin 33) | dtoverlay=uart5 |

## Recommended Hardware Configurations

### Configuration A: Separate Hardware UARTs (Best for Production)

```
Component           UART Device    GPIO Pins       Physical Pins
─────────────────   ────────────   ─────────────   ─────────────
IMU Input           /dev/ttyAMA0   GPIO 14/15      Pin 8/10
Telemetry Output    /dev/ttyAMA1   GPIO 0/1        Pin 27/28
```

**Enable UART1** in `/boot/config.txt`:
```
dtoverlay=uart1
```

**Code Configuration:**
```c
// fswv1_uart.c (IMU input - already configured)
#define UART_DEVICE "/dev/ttyAMA0"  

// fswv1_uart_telemetry.c (Telemetry output - change this)
#define TELEMETRY_UART_DEVICE "/dev/ttyAMA1"
```

### Configuration B: USB Adapter (Best for Testing)

```
Component           UART Device      Connection
─────────────────   ──────────────   ─────────────────────
IMU Input           /dev/ttyAMA0     GPIO 14/15 (Pin 8/10)
Telemetry Output    /dev/ttyUSB0     USB port (plug & play)
```

**No config.txt changes needed!**

**Code Configuration:**
```c
// fswv1_uart.c (IMU input - already configured)
#define UART_DEVICE "/dev/ttyAMA0"  

// fswv1_uart_telemetry.c (Telemetry output - change this)
#define TELEMETRY_UART_DEVICE "/dev/ttyUSB0"
```

### Configuration C: Two USB Adapters (Easiest Development)

```
Component           UART Device      Connection
─────────────────   ──────────────   ─────────────────────
IMU Input           /dev/ttyUSB0     USB port 1
Telemetry Output    /dev/ttyUSB1     USB port 2
```

**Code Configuration:**
```c
// fswv1_uart.c (IMU input - change this)
#define UART_DEVICE "/dev/ttyUSB0"  

// fswv1_uart_telemetry.c (Telemetry output - change this)
#define TELEMETRY_UART_DEVICE "/dev/ttyUSB1"
```

## Wiring Diagrams

### Wiring Diagram 1: IMU Input (Existing)

```
┌─────────────────────────────┐
│      IMU Sensor Device      │
│  (Arduino/ESP32/STM32)      │
│                             │
│  TX ─────────────┐          │
│  GND ───────┐    │          │
│  VCC ────┐  │    │          │
└──────────┼──┼────┼──────────┘
           │  │    │
           │  │    │
┌──────────┼──┼────┼──────────┐
│          │  │    │          │
│  3.3V ◄──┘  │    │          │
│  GND  ◄─────┘    │          │
│  GPIO 15 (RX) ◄──┘          │
│  GPIO 14 (TX) (not used)    │
│                             │
│     Raspberry Pi            │
│     /dev/ttyAMA0            │
└─────────────────────────────┘
```

### Wiring Diagram 2: Telemetry Output (USB Adapter - RECOMMENDED)

```
┌─────────────────────────────┐
│     Raspberry Pi            │
│                             │
│     USB Port                │
│        │                    │
└────────┼────────────────────┘
         │
         │ USB Cable
         │
┌────────┼────────────────────┐
│        ▼                    │
│   USB-to-Serial Adapter     │
│   Appears as /dev/ttyUSB0   │
│                             │
│   TX ─────────────┐         │
│   RX (not used)   │         │
│   GND ────────┐   │         │
└───────────────┼───┼─────────┘
                │   │
                │   │
┌───────────────┼───┼─────────┐
│               │   │         │
│   GND ◄───────┘   │         │
│   RX ◄────────────┘         │
│                             │
│   Ground Station / PC       │
│   Serial Terminal           │
└─────────────────────────────┘
```

### Wiring Diagram 3: Telemetry Output (Hardware UART1)

```
┌─────────────────────────────┐
│     Raspberry Pi            │
│                             │
│  GPIO 0 (TX) ────────┐      │
│  Pin 27              │      │
│                      │      │
│  GND ────────┐       │      │
│  Pin 6/14/20 │       │      │
└──────────────┼───────┼──────┘
               │       │
               │       │
┌──────────────┼───────┼──────┐
│              │       │      │
│  GND ◄───────┘       │      │
│  RX ◄────────────────┘      │
│                             │
│  Ground Station / PC        │
│  Serial Terminal            │
└─────────────────────────────┘
```

## Voltage Levels - CRITICAL!

### Raspberry Pi GPIO Voltage
- **Logic Level**: 3.3V
- **NOT 5V tolerant** - will damage GPIO pins!

### Safe Connections

✅ **Safe Configurations:**
- Pi (3.3V) ↔ Pi (3.3V)
- Pi (3.3V) ↔ 3.3V UART device
- Pi (3.3V) ↔ 3.3V USB-to-Serial adapter

⚠️ **Requires Level Shifter:**
- Pi (3.3V) ↔ Arduino 5V
- Pi (3.3V) ↔ 5V UART device

❌ **WILL DAMAGE PI:**
- Connecting 5V signal directly to Pi GPIO

### Level Shifter Example
If connecting to 5V device:
```
5V Device          Level Shifter      Raspberry Pi (3.3V)
─────────         ──────────────      ──────────────────
TX (5V)  ───────> HV → LV    ───────> GPIO 15 (RX)
         ←─────── LV ← HV    ←─────── GPIO 14 (TX)
GND      ───────> GND ← GND  ───────> GND
```

## Testing Pin Configuration

### Test 1: Verify UART Devices Exist
```bash
ls -l /dev/ttyAMA* /dev/ttyS* /dev/ttyUSB*
```

**Expected Output:**
```
crw-rw---- 1 root dialout 204, 64 Jan 23 10:00 /dev/ttyAMA0
crw-rw---- 1 root dialout 204, 65 Jan 23 10:00 /dev/ttyAMA1
crw-rw---- 1 root dialout   4, 64 Jan 23 10:00 /dev/ttyS0
crw-rw---- 1 root dialout 188,  0 Jan 23 10:00 /dev/ttyUSB0
```

### Test 2: Check GPIO Pin States
```bash
# Install raspi-gpio if not available
sudo apt-get install raspi-gpio

# Check GPIO 14/15 (UART0)
raspi-gpio get 14,15

# Check GPIO 0/1 (UART1)
raspi-gpio get 0,1
```

### Test 3: UART Loopback Test
```bash
# Connect TX to RX on the UART you want to test
# Then run:
echo "test" > /dev/ttyAMA0 &
cat /dev/ttyAMA0
# Should display "test"
```

## Quick Reference Summary

### Current Configuration (Default)
```
IMU Input:       /dev/ttyAMA0  (GPIO 14/15, Pin 8/10)
Telemetry Out:   /dev/ttyS0    (GPIO 14/15, Pin 8/10) ← CONFLICT!
```

### Recommended Change
```
IMU Input:       /dev/ttyAMA0  (GPIO 14/15, Pin 8/10)
Telemetry Out:   /dev/ttyUSB0  (USB adapter)          ← NO CONFLICT!
```

### Alternative (Production)
```
IMU Input:       /dev/ttyAMA0  (GPIO 14/15, Pin 8/10)
Telemetry Out:   /dev/ttyAMA1  (GPIO 0/1, Pin 27/28)  ← NO CONFLICT!
                 Requires: dtoverlay=uart1 in /boot/config.txt
```

## Files to Modify

If you choose a different UART device, update these files:

### For Telemetry Output UART:
**File**: `fsw/src/fswv1_uart_telemetry.c`  
**Line**: ~29  
**Change**:
```c
#define TELEMETRY_UART_DEVICE "/dev/ttyUSB0"  // or /dev/ttyAMA1
```

### For IMU Input UART (if needed):
**File**: `fsw/src/fswv1_uart.c`  
**Line**: ~34  
**Change**:
```c
#define UART_DEVICE "/dev/ttyAMA0"  // Usually don't need to change this
```

## Troubleshooting

### Both UARTs Not Working
**Likely Cause**: Pin conflict (both using GPIO 14/15)  
**Solution**: Use USB adapter or enable UART1

### IMU Not Receiving Data
**Check**: 
- IMU TX connected to Pi GPIO 15 (Pin 10)
- Baud rate matches (115200)
- Voltage levels (3.3V)
- IMU is powered and sending data

### Telemetry Not Transmitting
**Check**:
- Correct UART device in code
- Device appears in `/dev/` listing
- No other process using the UART
- Receiving device configured correctly

---

**Recommended Quick Start**: Use a USB-to-Serial adapter for telemetry output to avoid any pin conflicts!

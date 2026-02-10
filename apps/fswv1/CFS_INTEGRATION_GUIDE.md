# cFS Build System Integration Guide for FSWV1

## Overview

This guide explains how to integrate the FSWV1 application into the cFS build system, including the target configuration and startup script setup.

## Directory Structure

Typical cFS directory structure:
```
cfs/
├── apps/
│   ├── fswv1/                    # Your FSWV1 application (copy here)
│   │   ├── fsw/
│   │   │   ├── src/
│   │   │   └── inc/
│   │   └── CMakeLists.txt
│   ├── sample_app/
│   ├── sch_lab/
│   └── ...
├── sample_defs/
│   ├── targets/
│   │   └── cpu1/                 # Your target configuration
│   │       ├── inc/
│   │       │   └── target_config.cmake
│   │       └── ...
│   ├── cpu1_cfe_es_startup.scr  # Startup script
│   └── ...
├── build/
│   └── exe/
│       └── cpu1/
│           ├── cf/
│           │   └── cfe_es_startup.scr  # Final startup script location
│           └── core-cpu1            # Executable
└── CMakeLists.txt                # Top-level CMake
```

## Step-by-Step Integration

### Step 1: Copy FSWV1 Application

```bash
# Navigate to your cFS apps directory
cd /path/to/cfs/apps

# Extract or copy the FSWV1 application
tar -xzf fswv1_with_uart_telemetry.tar.gz

# OR if you have the directory:
cp -r /path/to/fswv1 .

# Verify structure
ls fswv1/
# Should show: fsw/ CMakeLists.txt README.md etc.
```

### Step 2: Add FSWV1 to Target Configuration

**File**: `sample_defs/targets/cpu1/inc/target_config.cmake`

Or if using simplified structure:
**File**: `sample_defs/cpu1_target.cmake`

Add fswv1 to the application list:

```cmake
##########################################################################
# Target: cpu1
##########################################################################

# List of applications to build for this target
set(TGT1_APPLIST
    # Core flight services applications
    sch_lab
    ci_lab
    to_lab
    sample_app
    
    # Custom applications
    fswv1          # ← ADD THIS LINE
)

# OR if using this format:
list(APPEND TGT_APP_LIST
    sch_lab
    ci_lab
    to_lab
    sample_app
    fswv1          # ← ADD THIS LINE
)
```

**Full Example** (`target_config.cmake`):

```cmake
##########################################################################
# CPU1 Target Configuration
##########################################################################

# Set mission name
set(MISSION_NAME "cFS_FSWV1")

# Target name
set(TGT1_NAME cpu1)

# Application list
set(TGT1_APPLIST
    sch_lab
    ci_lab  
    to_lab
    sample_app
    fswv1          # FSWV1 sensor application
)

# System applications (usually don't modify)
set(TGT1_SYSTEM_APPLIST
    cfe_assert
)

# Static applications (loaded as libraries)
set(TGT1_STATIC_APPLIST
    sample_lib
)

# File system mapping
set(TGT1_FILELIST
    "cfe_es_startup.scr"
)

##########################################################################
# FSWV1 specific settings (optional)
##########################################################################

# If you need special compiler flags
# set(fswv1_CFLAGS "-Wall -Wextra")

# If you need to override UART devices at build time
# add_compile_definitions(
#     TELEMETRY_UART_DEVICE="/dev/ttyUSB0"
# )
```

### Step 3: Configure Startup Script

**File**: `sample_defs/cpu1_cfe_es_startup.scr`

Or during build it may be:
**File**: `build/exe/cpu1/cf/cfe_es_startup.scr`

Add FSWV1 entry:

```
!
! Startup script for cFS
!

CFE_LIB, sample_lib,  SAMPLE_LIB_Init,      SAMPLE_LIB,        0,     0,    0x0, 0;
CFE_APP, sample_app,  SAMPLE_APP_Main,      SAMPLE_APP,       50,  8192, 0x0, 0;
CFE_APP, sch_lab,     SCH_LAB_AppMain,      SCH_LAB_APP,      80, 16384, 0x0, 0;
CFE_APP, ci_lab,      CI_Lab_AppMain,       CI_LAB_APP,       70,  8192, 0x0, 0;
CFE_APP, to_lab,      TO_Lab_AppMain,       TO_LAB_APP,       74, 16384, 0x0, 0;

! ← ADD THIS LINE
CFE_APP, fswv1,       FSWV1_APP_Main,       FSWV1_APP,        60, 16384, 0x0, 0;

!
! Entry Format:
! Entry Type, Name, Entry Point, Filename, Priority, Stack Size, Load Addr, Exception Action
!
```

**Startup Script Field Explanation:**

| Field | Value | Meaning |
|-------|-------|---------|
| Entry Type | `CFE_APP` | This is an application (not library) |
| Name | `fswv1` | Application name (must match) |
| Entry Point | `FSWV1_APP_Main` | Main function name |
| Filename | `FSWV1_APP` | Shared object name (usually uppercase) |
| Priority | `60` | Task priority (lower = higher priority) |
| Stack Size | `16384` | 16KB stack (adequate for sensors) |
| Load Address | `0x0` | Let OS choose address |
| Exception Action | `0` | Restart app on exception |

### Step 4: Build Configuration

**Option A: Clean Build (Recommended)**

```bash
cd /path/to/cfs

# Clean previous builds
make distclean
# OR
rm -rf build

# Prepare build system
make prep

# Build everything
make

# Install to build/exe/cpu1
make install
```

**Option B: Incremental Build**

```bash
cd /path/to/cfs

# Just rebuild if you modified fswv1 code
make fswv1

# Install
make install
```

### Step 5: Verify Build

```bash
# Check if fswv1 library was built
ls build/exe/cpu1/lib/libfswv1.so

# Check if startup script was copied
ls build/exe/cpu1/cf/cfe_es_startup.scr

# Verify fswv1 is in startup script
grep fswv1 build/exe/cpu1/cf/cfe_es_startup.scr
```

Expected output:
```
CFE_APP, fswv1,       FSWV1_APP_Main,       FSWV1_APP,        60, 16384, 0x0, 0;
```

## Alternative: targets.cmake Method

Some cFS versions use a different structure:

**File**: `sample_defs/targets.cmake`

```cmake
# This file defines the targets to build

# CPU1 Target
list(APPEND MISSION_GLOBAL_APPLIST
    sch_lab
    ci_lab
    to_lab
    sample_app
    fswv1          # ← ADD HERE
)

# Target-specific file table
set(cpu1_FILELIST
    "cfe_es_startup.scr"
)
```

## System Prerequisites

Before running cFS with FSWV1:

### 1. Install Required Libraries

```bash
# libgpiod (for GPIO/LED control)
sudo apt-get install libgpiod-dev libgpiod2

# Python pyserial (for test scripts)
pip3 install pyserial
```

### 2. Configure UART Permissions

```bash
# Add user to dialout group
sudo usermod -a -G dialout $USER

# Logout and login, or reboot
# Verify:
groups | grep dialout
```

### 3. Enable UARTs (Raspberry Pi)

```bash
# Enable serial port
sudo raspi-config
# Interface Options → Serial Port → Enable

# OR edit /boot/config.txt:
sudo nano /boot/config.txt

# Add these lines:
enable_uart=1
dtoverlay=uart1    # For second UART (optional)

# Reboot
sudo reboot
```

### 4. Verify UART Devices

```bash
# Check UART devices exist
ls -l /dev/ttyAMA* /dev/ttyS* /dev/ttyUSB*

# Should show:
# /dev/ttyAMA0  (IMU input)
# /dev/ttyS0 or /dev/ttyUSB0 (telemetry output)
```

### 5. Configure I2C (for BMP280)

```bash
# Enable I2C
sudo raspi-config
# Interface Options → I2C → Enable

# Verify I2C device exists
ls -l /dev/i2c-1

# Check I2C address (if BMP280 connected)
i2cdetect -y 1
# Should show device at 0x76 or 0x77
```

## Running cFS with FSWV1

### Method 1: Standard Run

```bash
cd /path/to/cfs/build/exe/cpu1

# Run cFS
./core-cpu1
```

### Method 2: With Output Logging

```bash
cd /path/to/cfs/build/exe/cpu1

# Run with output to file
./core-cpu1 2>&1 | tee cfs_output.log
```

### Method 3: Background Run

```bash
cd /path/to/cfs/build/exe/cpu1

# Run in background
nohup ./core-cpu1 > cfs.log 2>&1 &

# Check if running
ps aux | grep core-cpu1

# View log
tail -f cfs.log
```

## Verification After Startup

### 1. Check Initialization Messages

Look for these messages in console output:

```
CFE_ES: Starting: fswv1
FSWV1: Application initialized
FSWV1: I2C sensor initialized on /dev/i2c-1
FSWV1_UART: UART initialized on /dev/ttyAMA0 at 115200 baud
FSWV1_TELEMETRY_UART: Telemetry UART initialized on /dev/ttyS0 at 115200 baud
FSWV1: UDP socket initialized (dest: 127.0.0.1:5555)
FSWV1_GPIO: GPIO initialized on chip gpiochip0, pin 17
```

### 2. Monitor Telemetry

**UDP Telemetry:**
```bash
# In another terminal
cd /path/to/cfs/apps/fswv1
python3 udp_telemetry_receiver.py
```

**UART Telemetry:**
```bash
# View with screen
screen /dev/ttyS0 115200

# OR use Python script
python3 uart_telemetry_receiver.py --device /dev/ttyS0
```

### 3. Send Commands

```bash
cd /path/to/cfs/apps/fswv1

# Test basic commands
python3 fswv1_cmd_test.py

# LED commands
python3 fswv1_cmd_test.py --led-on
python3 fswv1_cmd_test.py --led-off
python3 fswv1_cmd_test.py --led-toggle
```

## Troubleshooting Build Issues

### Issue 1: "Could not find fswv1"

**Symptom**: CMake can't find fswv1 directory

**Solution**:
```bash
# Check app directory exists
ls apps/fswv1/CMakeLists.txt

# If not, copy it:
cp -r /path/to/fswv1 apps/

# Clean and rebuild
make distclean
make prep
make
```

### Issue 2: "undefined reference to libgpiod"

**Symptom**: Linker error about libgpiod

**Solution**:
```bash
# Install libgpiod development files
sudo apt-get install libgpiod-dev

# Verify CMakeLists.txt has:
grep "target_link_libraries(fswv1 gpiod)" apps/fswv1/CMakeLists.txt

# Rebuild
make fswv1
make install
```

### Issue 3: Startup script not found

**Symptom**: "Error loading startup script"

**Solution**:
```bash
# Check startup script exists
ls sample_defs/cpu1_cfe_es_startup.scr

# Or check final location
ls build/exe/cpu1/cf/cfe_es_startup.scr

# If missing, copy template
cp apps/fswv1/cfe_es_startup.scr sample_defs/cpu1_cfe_es_startup.scr

# Rebuild
make install
```

### Issue 4: FSWV1 not starting

**Symptom**: cFS starts but FSWV1 doesn't appear

**Check**:
1. Is fswv1 in startup script?
   ```bash
   grep fswv1 build/exe/cpu1/cf/cfe_es_startup.scr
   ```

2. Is shared library present?
   ```bash
   ls build/exe/cpu1/lib/libfswv1.so
   ```

3. Check for errors:
   ```bash
   grep -i error cfs.log
   grep -i fswv1 cfs.log
   ```

## Runtime Configuration

### Override UART Devices Without Rebuilding

You can use environment variables or configuration files:

**Method 1: Symbolic Links**
```bash
# If telemetry UART device is /dev/ttyUSB0 but code expects /dev/ttyS0
sudo ln -s /dev/ttyUSB0 /dev/ttyS0
```

**Method 2: Modify Source and Rebuild**
```bash
# Edit UART device
nano apps/fswv1/fsw/src/fswv1_uart_telemetry.c
# Change line 29: #define TELEMETRY_UART_DEVICE "/dev/ttyUSB0"

# Rebuild just fswv1
make fswv1
make install
```

## Priority Tuning

If FSWV1 interferes with other apps, adjust priority:

**In startup script** (`cfe_es_startup.scr`):
```
! Lower number = higher priority
! Typical ranges:
!   10-30: Critical system tasks
!   40-60: Normal applications
!   70-90: Low priority background tasks

CFE_APP, fswv1, FSWV1_APP_Main, FSWV1_APP, 60, 16384, 0x0, 0;
                                              ^^
                                              Adjust this
```

**Recommendations:**
- `50` - Higher priority (if sensor timing is critical)
- `60` - Normal priority (default, recommended)
- `70` - Lower priority (if sensors can tolerate delays)

## Stack Size Tuning

If FSWV1 crashes with stack overflow, increase stack size:

```
CFE_APP, fswv1, FSWV1_APP_Main, FSWV1_APP, 60, 16384, 0x0, 0;
                                                ^^^^^
                                                Increase this
```

**Recommendations:**
- `8192` (8KB) - Minimum for simple apps
- `16384` (16KB) - Default for FSWV1 (recommended)
- `32768` (32KB) - If you add more features
- `65536` (64KB) - Maximum for most systems

## Summary Checklist

- [ ] FSWV1 directory in `apps/fswv1/`
- [ ] CMakeLists.txt has `target_link_libraries(fswv1 gpiod)`
- [ ] fswv1 added to target configuration
- [ ] fswv1 added to startup script
- [ ] libgpiod-dev installed
- [ ] User in dialout group
- [ ] UARTs enabled and accessible
- [ ] I2C enabled (if using BMP280)
- [ ] Clean build successful
- [ ] Shared library created: `libfswv1.so`
- [ ] Startup script in `build/exe/cpu1/cf/`
- [ ] cFS runs and FSWV1 initializes
- [ ] Telemetry visible on UDP and/or UART

## Quick Start Command Sequence

```bash
# 1. Copy application
cd /path/to/cfs
cp -r /path/to/fswv1 apps/

# 2. Add to startup script
echo 'CFE_APP, fswv1, FSWV1_APP_Main, FSWV1_APP, 60, 16384, 0x0, 0;' >> sample_defs/cpu1_cfe_es_startup.scr

# 3. Add to target config (edit manually)
nano sample_defs/targets/cpu1/inc/target_config.cmake
# Add: fswv1

# 4. Build
make distclean
make prep
make
make install

# 5. Run
cd build/exe/cpu1
./core-cpu1
```

---

**For detailed hardware setup and UART configuration, see: UART_PINOUT_REFERENCE.md**

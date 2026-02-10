# Complete Guide: BMP280 Sensor cFS Application

## Overview
This guide will walk you through creating a complete cFS application to read BMP280 sensor data, transmit it via UDP, and print it to the terminal.

## Prerequisites
- Latest cFS framework installed (with EDS support)
- Basic understanding of C programming
- BMP280 sensor connected via I2C

## Directory Structure

Your cFS application should be placed in the `apps` directory of your cFS installation:

```
cFS/
├── apps/
│   └── bmp280_app/              # Your new application
│       ├── fsw/
│       │   ├── inc/             # Header files
│       │   │   └── bmp280_app.h
│       │   ├── src/             # Source files
│       │   │   ├── bmp280_app.c
│       │   │   └── bmp280_sensor.c
│       │   └── tables/          # Table definitions (if needed)
│       ├── eds/                 # EDS XML files (NEW in latest cFS)
│       │   └── bmp280_app.xml
│       └── CMakeLists.txt
├── cfe/
├── osal/
├── psp/
└── tools/
```

## Step-by-Step Implementation

### Step 1: Navigate to the apps directory

```bash
cd <your_cfs_path>/apps
mkdir bmp280_app
cd bmp280_app
```

### Step 2: Create directory structure

```bash
mkdir -p fsw/inc
mkdir -p fsw/src
mkdir -p eds
```

## File Contents

I'll provide all the necessary files below. Create each file as shown.

---

## Key Concepts

### 1. **EDS (Electronic Data Sheets)**
The latest cFS uses XML files to define messages, commands, and telemetry. This replaces manual message ID definitions.

### 2. **Per-App Message IDs**
Each app has its own message ID space, making integration cleaner.

### 3. **Software Bus (SB)**
Used for inter-app communication. Your app will:
- Publish telemetry messages with sensor data
- Subscribe to command messages

### 4. **OSAL (Operating System Abstraction Layer)**
Provides OS-independent APIs for:
- Task management
- I2C communication (OS_open, OS_read, OS_write)
- UDP sockets (OS_SocketOpen, OS_SocketSendTo)
- Printing (OS_printf)

---

## What This App Does

1. **Initializes** the BMP280 sensor via I2C
2. **Reads** temperature and pressure data periodically
3. **Publishes** data on Software Bus
4. **Transmits** data via UDP to a ground station
5. **Prints** data to terminal using OS_printf
6. **Responds** to commands (enable/disable, set rate)

---

## Integration Steps (After Creating Files)

### 1. Add to cFS Build System

Edit `<cfs_root>/sample_defs/targets.cmake` and add:
```cmake
list(APPEND MISSION_GLOBAL_APPLIST bmp280_app)
```

### 2. Configure Startup Script

Edit `<cfs_root>/sample_defs/cpu1_cfe_es_startup.scr`:
```
CFE_APP, bmp280_app,  BMP280_APP_Main,  BMP280_APP,  50,  16384,  0x0, 0;
```

### 3. Build cFS
```bash
cd <cfs_root>
make prep
make
make install
```

### 4. Run cFS
```bash
cd build/exe/cpu1
./core-cpu1
```

---

## Next Steps
- Adjust sensor reading frequency in the code
- Configure UDP destination IP/port
- Add error handling for sensor failures
- Implement additional commands as needed

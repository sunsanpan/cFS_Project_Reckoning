# BMP280 Sensor cFS Application

A complete cFS application for reading BMP280 sensor data via I2C, publishing to Software Bus, transmitting via UDP, and printing to terminal.

## Features

✅ **Sensor Reading**: Reads temperature and pressure from BMP280 via I2C  
✅ **Software Bus**: Publishes sensor data using cFS messaging  
✅ **UDP Transmission**: Sends data to ground station via UDP  
✅ **Terminal Output**: Prints readings using OS_printf  
✅ **Commands**: Enable/disable sensor, reset counters  
✅ **EDS Support**: Uses latest cFS with per-app message IDs and XML definitions  
✅ **Simulation Mode**: Falls back to simulated data if hardware unavailable  

## Directory Structure

```
bmp280_app/
├── CMakeLists.txt              # Build configuration
├── README.md                   # This file
├── fsw/                        # Flight software
│   ├── inc/                    # Header files
│   │   ├── bmp280_app.h           # Main app header
│   │   ├── bmp280_app_msg.h       # Message definitions (stub)
│   │   ├── bmp280_app_msgids.h    # Message IDs (stub)
│   │   └── bmp280_app_version.h   # Version info
│   └── src/                    # Source files
│       ├── bmp280_app.c           # Main app logic
│       └── bmp280_sensor.c        # Sensor interface & UDP
└── eds/                        # EDS definitions
    └── bmp280_app.xml             # Message definitions (XML)
```

## Prerequisites

1. **cFS Framework**: Latest version with EDS support
2. **Hardware** (optional): BMP280 sensor on I2C bus
3. **Tools**: CMake, GCC, make

## Installation Steps

### Step 1: Copy Application to cFS

```bash
# Navigate to your cFS apps directory
cd <your_cfs_root>/apps

# Copy the bmp280_app directory here
cp -r /path/to/bmp280_app .

# Verify structure
ls bmp280_app
# Should show: CMakeLists.txt  README.md  eds/  fsw/
```

### Step 2: Add to Mission Build

Edit `<cfs_root>/sample_defs/targets.cmake`:

```cmake
# Add to the app list
list(APPEND MISSION_GLOBAL_APPLIST 
    bmp280_app
)
```

### Step 3: Configure Startup

Edit `<cfs_root>/sample_defs/cpu1_cfe_es_startup.scr`:

```
CFE_APP, bmp280_app, BMP280_APP_Main, BMP280_APP, 50, 16384, 0x0, 0;
```

**Parameters explained:**
- `bmp280_app`: App name (matches library name)
- `BMP280_APP_Main`: Entry point function
- `BMP280_APP`: App name in uppercase
- `50`: Priority (50 is typical)
- `16384`: Stack size (16KB)
- `0x0`: Exception action
- `0`: No additional flags

### Step 4: Build cFS

```bash
cd <cfs_root>

# Prepare build
make prep

# Build
make

# Install
make install
```

### Step 5: Run cFS

```bash
cd build/exe/cpu1
./core-cpu1
```

## Configuration

### I2C Settings (in bmp280_app.h)

```c
#define BMP280_I2C_DEVICE "/dev/i2c-1"    // I2C device path
#define BMP280_I2C_ADDRESS 0x76            // Or 0x77
```

### UDP Settings (in bmp280_app.h)

```c
#define BMP280_UDP_PORT 5555               // Destination port
#define BMP280_UDP_DEST_IP "127.0.0.1"     // Ground station IP
```

### Read Rate (in bmp280_app.h)

```c
#define BMP280_DEFAULT_READ_RATE 1  // Hz (once per second)
```

## Usage

### Viewing Output

The app prints sensor data to the terminal every read cycle:

```
BMP280: Temp=25.34°C, Press=101325.00 Pa
```

### Sending Commands

Using `cmdUtil` or ground system:

```bash
# No-op (verify app is running)
cmdUtil --es-app-name BMP280_APP --mid 0x1884 --cc 0

# Reset counters
cmdUtil --es-app-name BMP280_APP --mid 0x1884 --cc 1

# Disable sensor
cmdUtil --es-app-name BMP280_APP --mid 0x1884 --cc 3

# Enable sensor
cmdUtil --es-app-name BMP280_APP --mid 0x1884 --cc 2
```

### Receiving UDP Data

On your ground station, listen on the configured port:

```bash
# Using netcat
nc -ul 5555

# Or Python
python3 << 'EOF'
import socket
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind(('0.0.0.0', 5555))
while True:
    data, addr = sock.recvfrom(1024)
    print(f"Received: {data.decode()}")
EOF
```

Data format: `BMP280,<temp>,<pressure>,<timestamp>`

### Viewing Telemetry

Subscribe to telemetry packets:

```bash
# Housekeeping telemetry (MID: 0x0884)
# Sensor data telemetry (MID: 0x0885)
```

Use COSMOS, CFS Ground System, or cmdUtil to view.

## How It Works

### Main Loop

1. **Initialize**: Opens I2C device, creates UDP socket, subscribes to commands
2. **Wait for Messages**: Pends on command pipe with 500ms timeout
3. **On Timeout**: Reads sensor if enabled, publishes data, sends UDP, prints to terminal
4. **On Command**: Processes command (enable/disable/reset/noop)

### Sensor Reading

- **Real Hardware**: Reads from I2C device `/dev/i2c-1` at address `0x76`
- **Simulation**: If I2C fails, generates realistic simulated data
- **Compensation**: Applies BMP280 calibration formulas from datasheet

### Data Flow

```
BMP280 Sensor (I2C)
    ↓
Read & Compensate
    ↓
┌─────────────────┐
│   Update Data   │
└─────────────────┘
    ↓
    ├──→ Print to Terminal (OS_printf)
    ├──→ Publish to Software Bus (CFE_SB)
    └──→ Send via UDP (OS_Socket)
```

## Troubleshooting

### Build Errors

**Error**: `bmp280_app_msgids.h not found`
- **Solution**: The EDS system should generate this. Check that `eds/bmp280_app.xml` exists.

**Error**: `undefined reference to 'CFE_MSG_Init'`
- **Solution**: Ensure you're using latest cFS version. Update cFE submodule.

### Runtime Errors

**Error**: `Failed to open I2C device`
- **Solution**: Normal if no hardware. App continues in simulation mode.
- Check I2C device path: `ls /dev/i2c-*`
- Check permissions: `sudo chmod 666 /dev/i2c-1`

**Error**: `UDP send error`
- **Solution**: Check network connectivity and firewall rules.
- Verify destination IP is reachable: `ping <IP>`

**No output in terminal**
- Check app is running: `cat build/exe/cpu1/cf/cfe_es_app_info.log`
- Check sensor is enabled (enabled by default at startup)
- View system log: `tail -f build/exe/cpu1/cf/cfe_es_syslog.dat`

### Command Issues

**Commands not responding**
- Verify message IDs are correct after EDS generation
- Check `build/inc/bmp280_app_msgids.h` for actual IDs
- Use correct message ID in command tool

## Understanding EDS (Latest cFS)

### What is EDS?

EDS (Electronic Data Sheets) is an XML-based system for defining:
- Message structures
- Message IDs  
- Commands
- Telemetry

### How EDS Works

1. **Define**: Create XML file (`eds/bmp280_app.xml`) with message definitions
2. **Build**: cFS build system processes XML and generates C headers
3. **Use**: Generated headers provide message types and IDs

### Generated Files

After building, check these generated files:

```bash
# Message structures
build/inc/bmp280_app_msg.h

# Message IDs  
build/inc/bmp280_app_msgids.h

# Type definitions
build/inc/bmp280_app_types.h
```

### Per-App Message IDs

In latest cFS, each app has its own message ID namespace:
- **Format**: `${MISSION_NAME}/BMP280_APP/<MSG_TYPE>`
- **Assignment**: cFS assigns numeric values automatically
- **Benefit**: No manual ID conflicts between apps

## Key cFS APIs Used

### OSAL APIs

```c
OS_printf()              // Print to terminal
OS_OpenCreate()          // Open I2C device
OS_read() / OS_write()   // I2C communication
OS_SocketOpen()          // Create UDP socket
OS_SocketSendTo()        // Send UDP data
OS_close()               // Close file/socket
```

### cFE APIs

```c
CFE_ES_RegisterApp()     // Register with Executive Services
CFE_EVS_SendEvent()      // Send event message
CFE_SB_CreatePipe()      // Create message pipe
CFE_SB_Subscribe()       // Subscribe to messages
CFE_SB_ReceiveBuffer()   // Receive message
CFE_SB_TransmitMsg()     // Send telemetry
CFE_TIME_GetTime()       // Get timestamp
CFE_MSG_Init()           // Initialize message
```

## Customization

### Adding New Commands

1. **Add to EDS XML** (`eds/bmp280_app.xml`):
   ```xml
   <Define name="NEW_CMD_CC" value="4"/>
   <ContainerDataType name="NewCmd">...</ContainerDataType>
   ```

2. **Add handler** in `bmp280_app.c`:
   ```c
   case BMP280_APP_NEW_CMD_CC:
       BMP280_APP_NewCommand(...);
       break;
   ```

### Changing Read Rate

Modify the main loop timeout (currently 500ms):

```c
// In BMP280_APP_Main()
status = CFE_SB_ReceiveBuffer(&SBBufPtr, 
                              BMP280_APP_Data.CommandPipe, 
                              1000); // 1 second timeout = 1 Hz
```

### Different Sensor

Replace `bmp280_sensor.c` with your sensor's I2C protocol:
1. Update register definitions
2. Modify read/write functions
3. Update compensation formulas

## Testing Without Hardware

The app automatically runs in simulation mode if I2C is unavailable:

- Generates realistic temperature (~25°C with variation)
- Generates realistic pressure (~101325 Pa with variation)  
- All other functionality works identically

Perfect for:
- Development without hardware
- CI/CD testing
- Demonstration

## Further Development

### Adding More Sensors

Create separate sensor files:
```
fsw/src/bme680_sensor.c
fsw/src/mpu6050_sensor.c
```

Update CMakeLists.txt to include them.

### Adding Configuration Tables

1. Define table in `fsw/tables/bmp280_config.c`
2. Register with Table Services
3. Use `CFE_TBL_Load()` to load configuration

### Adding Unit Tests

1. Create `unit-test/` directory
2. Write tests using CFE's test framework
3. Add to CMakeLists.txt

## Additional Resources

- **cFS Documentation**: https://github.com/nasa/cFS
- **BMP280 Datasheet**: Bosch BMP280 Product Page
- **OSAL Guide**: See `osal-apiguide.pdf`
- **cFE Guide**: See `cfe-usersguide.pdf`

## License

This application is provided as an example for educational purposes.
Adapt and use according to your mission requirements.

## Support

For issues with:
- **cFS framework**: Check NASA cFS GitHub issues
- **This app**: Review this README and source code comments
- **Hardware**: Consult BMP280 datasheet and I2C documentation

---

**Author**: Generated for cFS Beginner Tutorial  
**Version**: 1.0.0  
**Date**: 2024  
**Compatible with**: cFS 7.0+ with EDS support

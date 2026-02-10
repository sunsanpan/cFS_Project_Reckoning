# BMP280 cFS App with LED Control - Enhanced Version

## Overview

This is an enhanced version of the BMP280 sensor application for cFS (Core Flight System) running on Raspberry Pi 5. In addition to reading temperature and pressure data from the BMP280 sensor via I2C, this version includes **GPIO-based LED control** through telecommands.

## New Features

### LED Control via GPIO

The application now controls an LED connected to GPIO 17 (Physical Pin 11) on the Raspberry Pi 5. You can change the GPIO pin by modifying the `LED_GPIO_PIN` constant in `bmp280_gpio.c`.

### New Telecommands

Four new telecommands have been added for LED control:

1. **LED_ON (Command Code 4)**: Turn the LED on
2. **LED_OFF (Command Code 5)**: Turn the LED off
3. **LED_TOGGLE (Command Code 6)**: Toggle the LED state (ON→OFF or OFF→ON)
4. **LED_STATUS (Command Code 7)**: Query and report the current LED state

### Standard cFS Commands

The application also includes standard cFS commands:

1. **NOOP (Command Code 0)**: No operation, used for testing
2. **RESET_COUNTERS (Command Code 1)**: Reset command and error counters
3. **ENABLE (Command Code 2)**: Enable BMP280 sensor readings
4. **DISABLE (Command Code 3)**: Disable BMP280 sensor readings

## Hardware Setup

### LED Connection

Connect an LED to your Raspberry Pi 5 as follows:

```
GPIO 17 (Pin 11) ----[LED]----[330Ω Resistor]---- GND
```

**Important Notes:**
- Use a current-limiting resistor (220Ω - 330Ω recommended) to protect the LED
- Connect the LED's anode (longer leg) to GPIO through the resistor
- Connect the LED's cathode (shorter leg) to GND
- GPIO 17 is Physical Pin 11 on the 40-pin header

### BMP280 Connection

The BMP280 sensor remains connected via I2C as in the original app:

```
BMP280 VCC  → 3.3V (Pin 1)
BMP280 GND  → GND (Pin 6)
BMP280 SCL  → GPIO 3 / I2C1 SCL (Pin 5)
BMP280 SDA  → GPIO 2 / I2C1 SDA (Pin 3)
```

## GPIO Implementation Details

### sysfs GPIO Interface

The application uses the Linux sysfs GPIO interface for LED control, which provides:

- **Portability**: Works across different Linux systems
- **No special permissions**: Can work with appropriate udev rules
- **cFS compatibility**: Doesn't require kernel modules or special libraries

### GPIO Control Flow

1. **Initialization** (`BMP280_InitGPIO`):
   - Exports GPIO 17 to userspace via `/sys/class/gpio/export`
   - Sets the GPIO direction to "output"
   - Initializes the LED to OFF state

2. **LED Control** (`BMP280_SetLED`):
   - Writes "1" or "0" to `/sys/class/gpio/gpio17/value`
   - Updates internal LED state tracking

3. **LED Status** (`BMP280_GetLED`):
   - Reads current value from `/sys/class/gpio/gpio17/value`
   - Returns the actual hardware state

4. **Cleanup** (`BMP280_CloseGPIO`):
   - Turns off LED
   - Unexports GPIO via `/sys/class/gpio/unexport`

## Building the Application

### Prerequisites

```bash
# Ensure you have cFS built and configured
cd $CFS_PROJECT_DIR

# Copy this app to your cFS apps directory
cp -r bmp280_app $CFS_PROJECT_DIR/apps/
```

### Build Steps

1. Add to your `targets.cmake`:
```cmake
list(APPEND MISSION_GLOBAL_APPLIST bmp280_app)
```

2. Build cFS:
```bash
cd $CFS_PROJECT_DIR
make clean
make prep
make
make install
```

3. The app will be built with three source files:
   - `bmp280_app.c` - Main application logic
   - `bmp280_sensor.c` - BMP280 I2C sensor interface
   - `bmp280_gpio.c` - GPIO LED control (NEW)

## Sending Telecommands

### Using cmdUtil (cFS command-line tool)

```bash
# Turn LED ON
cmdUtil --pktid=0x1882 --cmdcode=4 --endian=LE

# Turn LED OFF
cmdUtil --pktid=0x1882 --cmdcode=5 --endian=LE

# Toggle LED
cmdUtil --pktid=0x1882 --cmdcode=6 --endian=LE

# Query LED status
cmdUtil --pktid=0x1882 --cmdcode=7 --endian=LE

# Enable sensor reading
cmdUtil --pktid=0x1882 --cmdcode=2 --endian=LE

# Disable sensor reading
cmdUtil --pktid=0x1882 --cmdcode=3 --endian=LE

# Reset counters
cmdUtil --pktid=0x1882 --cmdcode=1 --endian=LE

# NOOP
cmdUtil --pktid=0x1882 --cmdcode=0 --endian=LE
```

**Note**: Packet ID (0x1882) may vary based on your cFS configuration. Check your `bmp280_app_msgids.h` file.

### Using COSMOS or Ground System

Configure your ground system with these command definitions:

| Command | Code | Description |
|---------|------|-------------|
| NOOP | 0 | No operation |
| RESET_COUNTERS | 1 | Reset counters |
| ENABLE | 2 | Enable sensor |
| DISABLE | 3 | Disable sensor |
| LED_ON | 4 | Turn LED on |
| LED_OFF | 5 | Turn LED off |
| LED_TOGGLE | 6 | Toggle LED |
| LED_STATUS | 7 | Query LED status |

## Telemetry

### Housekeeping Telemetry

The housekeeping telemetry now includes LED state:

```c
typedef struct {
    uint8  CommandCounter;        // Number of successful commands
    uint8  CommandErrorCounter;   // Number of failed commands
    uint8  SensorEnabled;         // 1=enabled, 0=disabled
    uint32 ReadRate;              // Sensor read rate in Hz
    uint8  LedState;              // 1=ON, 0=OFF (NEW)
} BMP280_APP_HkTlm_Payload_t;
```

### Sensor Telemetry

Remains unchanged from the original app:

```c
typedef struct {
    float  Temperature;  // Degrees Celsius
    float  Pressure;     // Pascals
    uint32 Timestamp;    // Timestamp
} BMP280_APP_SensorTlm_Payload_t;
```

## Event Messages

New event messages for LED control:

| Event ID | Type | Description |
|----------|------|-------------|
| 11 | INFO | GPIO initialized successfully |
| 12 | ERROR | GPIO operation error |
| 13 | INFO | LED turned ON |
| 14 | INFO | LED turned OFF |
| 15 | INFO | LED toggled |
| 16 | INFO | LED status reported |

## Troubleshooting

### GPIO Permission Issues

If you get permission errors accessing GPIO:

```bash
# Add your user to the gpio group (if it exists)
sudo usermod -a -G gpio $USER

# Or create udev rules for GPIO access
sudo nano /etc/udev/rules.d/99-gpio.rules
```

Add:
```
SUBSYSTEM=="gpio", KERNEL=="gpiochip*", GROUP="gpio", MODE="0660"
SUBSYSTEM=="gpio", KERNEL=="gpio*", GROUP="gpio", MODE="0660"
```

Then reload:
```bash
sudo udevadm control --reload-rules
sudo udevadm trigger
```

### GPIO Already Exported

If GPIO 17 is already exported:

```bash
# Unexport it first
echo 17 | sudo tee /sys/class/gpio/unexport

# Or the app will handle it automatically
```

### LED Not Working

1. **Check wiring**: Ensure LED polarity is correct
2. **Check resistor**: Use 220Ω - 330Ω resistor
3. **Test manually**:
```bash
# Export GPIO
echo 17 | sudo tee /sys/class/gpio/export
echo out | sudo tee /sys/class/gpio/gpio17/direction
echo 1 | sudo tee /sys/class/gpio/gpio17/value  # LED ON
echo 0 | sudo tee /sys/class/gpio/gpio17/value  # LED OFF
```

### Changing GPIO Pin

To use a different GPIO pin, edit `fsw/src/bmp280_gpio.c`:

```c
// Change this line to your desired GPIO number
#define LED_GPIO_PIN 17  // Change to 18, 23, 24, etc.
```

Also update the path strings if needed:
```c
#define GPIO_DIRECTION_PATH "/sys/class/gpio/gpioXX/direction"
#define GPIO_VALUE_PATH     "/sys/class/gpio/gpioXX/value"
```

Replace `XX` with your GPIO number.

## File Structure

```
bmp280_app/
├── CMakeLists.txt                 # Build configuration (updated)
├── README.md                      # This file
├── LED_CONTROL_GUIDE.md          # LED control documentation
├── eds/
│   └── bmp280_app.xml            # EDS definitions (updated)
└── fsw/
    ├── inc/
    │   ├── bmp280_app.h          # Main header (updated)
    │   ├── bmp280_app_msg.h      # Message definitions (updated)
    │   ├── bmp280_app_msgids.h   # Message IDs
    │   └── bmp280_app_version.h  # Version info
    └── src/
        ├── bmp280_app.c          # Main app logic (updated)
        ├── bmp280_sensor.c       # BMP280 sensor interface
        └── bmp280_gpio.c         # GPIO LED control (NEW)
```

## cFS API Usage

This application demonstrates proper use of cFS APIs:

- **CFE_EVS**: Event Services for logging
- **CFE_SB**: Software Bus for messaging
- **CFE_MSG**: Message API for packet handling
- **CFE_ES**: Executive Services for app lifecycle
- **OS_printf**: OSAL print for debugging
- **OS_TaskDelay**: OSAL delay for timing

## License

This application follows the cFS licensing model. Refer to the main cFS repository for license details.

## Author

Enhanced with LED control telecommands for educational and demonstration purposes.

## Version History

- **v1.0.0**: Original BMP280 sensor app
- **v1.1.0**: Added GPIO LED control with telecommands (current version)

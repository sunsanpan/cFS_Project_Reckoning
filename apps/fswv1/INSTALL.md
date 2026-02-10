# Quick Installation Guide

## Installation Steps

### 1. Extract the App

```bash
cd $CFS_PROJECT_DIR/apps/
tar -xzf bmp280_app_with_led_control.tar.gz
```

### 2. Add to Your cFS Build

Edit `$CFS_PROJECT_DIR/targets.cmake`:

```cmake
# Add to your mission app list
list(APPEND MISSION_GLOBAL_APPLIST bmp280_app)
```

### 3. Configure Message IDs (if needed)

Edit `$CFS_PROJECT_DIR/sample_defs/cpu1_msgids.h` or your targets message ID file:

```c
#define BMP280_APP_CMD_MID        0x1882
#define BMP280_APP_SEND_HK_MID    0x1883  
#define BMP280_APP_HK_TLM_MID     0x0882
#define BMP280_APP_SENSOR_TLM_MID 0x0883
```

Or if using a dedicated msgids.h for bmp280_app, the file already contains:

```c
#define BMP280_APP_CMD_MID        0x1882
#define BMP280_APP_SEND_HK_MID    0x1883
#define BMP280_APP_HK_TLM_MID     0x0882
#define BMP280_APP_SENSOR_TLM_MID 0x0883
```

### 4. Build cFS

```bash
cd $CFS_PROJECT_DIR
make clean
make prep
make
make install
```

### 5. Hardware Setup

#### Connect BMP280 Sensor
```
BMP280 VCC  → 3.3V (Pin 1)
BMP280 GND  → GND (Pin 6)
BMP280 SCL  → GPIO 3 / I2C1 SCL (Pin 5)
BMP280 SDA  → GPIO 2 / I2C1 SDA (Pin 3)
```

#### Connect LED
```
GPIO 17 (Pin 11) → [LED Anode] → [330Ω Resistor] → [LED Cathode] → GND
```

### 6. Run cFS

```bash
cd $CFS_PROJECT_DIR/build/exe/cpu1
sudo ./core-cpu1
```

### 7. Test LED Control

```bash
# Turn LED ON
cmdUtil --pktid=0x1882 --cmdcode=4 --endian=LE

# Turn LED OFF
cmdUtil --pktid=0x1882 --cmdcode=5 --endian=LE

# Toggle LED
cmdUtil --pktid=0x1882 --cmdcode=6 --endian=LE
```

## Files Included

- **fsw/src/bmp280_app.c** - Main application (enhanced)
- **fsw/src/bmp280_sensor.c** - BMP280 I2C interface
- **fsw/src/bmp280_gpio.c** - GPIO LED control (NEW)
- **fsw/inc/*.h** - Headers (updated)
- **eds/bmp280_app.xml** - EDS definitions (updated)
- **CMakeLists.txt** - Build configuration (updated)
- **LED_CONTROL_GUIDE.md** - Complete documentation
- **TESTING_GUIDE.md** - Testing procedures
- **CHANGES_SUMMARY.md** - Summary of all changes

## Command Reference

| Command | Code | Function |
|---------|------|----------|
| NOOP | 0 | No operation |
| RESET_COUNTERS | 1 | Reset counters |
| ENABLE | 2 | Enable sensor |
| DISABLE | 3 | Disable sensor |
| LED_ON | 4 | Turn LED on |
| LED_OFF | 5 | Turn LED off |
| LED_TOGGLE | 6 | Toggle LED |
| LED_STATUS | 7 | Query LED status |

## Verification

The app is working correctly when you see these events:

```
BMP280 App Initialized. Version 1.0.0.0
BMP280_GPIO: GPIO 17 initialized successfully
BMP280: Temp=XX.XX°C, Press=XXXXX.XX Pa
```

And LED responds to commands.

## Troubleshooting

See `TESTING_GUIDE.md` for detailed troubleshooting steps.

Common issues:
- GPIO permissions → See LED_CONTROL_GUIDE.md
- I2C not detected → Check wiring, run `sudo i2cdetect -y 1`
- LED not lighting → Check LED polarity and resistor

## Support

For issues or questions, refer to:
1. LED_CONTROL_GUIDE.md - Complete documentation
2. TESTING_GUIDE.md - Testing and verification
3. CHANGES_SUMMARY.md - Technical details

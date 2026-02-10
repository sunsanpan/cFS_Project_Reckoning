# BMP280 cFS App - Complete Package Summary

## 📦 What's Included

This package contains your enhanced BMP280 cFS application with GPIO LED control and complete Python testing scripts.

## 🎯 Features

### cFS Application
- ✅ BMP280 temperature/pressure sensor reading via I2C
- ✅ GPIO LED control (GPIO 17)
- ✅ 8 telecommands (4 standard + 4 LED control)
- ✅ Enhanced telemetry with LED state
- ✅ Professional cFS integration
- ✅ Complete error handling

### Python Testing Tools
- ✅ **simple_cmd.py** - Quick and easy command sender
- ✅ **bmp280_cmd_test.py** - Full-featured testing suite
- ✅ Interactive menu mode
- ✅ Automated test sequences
- ✅ No dependencies (uses Python stdlib only)

## 📁 Package Contents

```
bmp280_app_with_led_control.tar.gz
├── Source Code
│   ├── fsw/src/bmp280_app.c       - Main application (enhanced)
│   ├── fsw/src/bmp280_sensor.c    - BMP280 I2C interface
│   └── fsw/src/bmp280_gpio.c      - GPIO LED control (NEW)
│
├── Headers
│   ├── fsw/inc/bmp280_app.h       - Main header (updated)
│   ├── fsw/inc/bmp280_app_msg.h   - Messages (updated)
│   ├── fsw/inc/bmp280_app_msgids.h
│   └── fsw/inc/bmp280_app_version.h
│
├── Build & Config
│   ├── CMakeLists.txt             - Build config (updated)
│   └── eds/bmp280_app.xml         - EDS definitions (updated)
│
├── Python Testing Scripts
│   ├── simple_cmd.py              - Simple command sender (NEW)
│   ├── bmp280_cmd_test.py         - Full testing tool (NEW)
│   └── PYTHON_README.md           - Quick start guide (NEW)
│
└── Documentation
    ├── README_ENHANCED.md         - Main overview
    ├── INSTALL.md                 - Installation guide
    ├── LED_CONTROL_GUIDE.md       - Complete LED docs
    ├── TESTING_GUIDE.md           - Testing procedures
    ├── PYTHON_SCRIPTS_GUIDE.md    - Python scripts manual (NEW)
    ├── CHANGES_SUMMARY.md         - Technical changes
    ├── QUICKSTART.md              - Quick start
    └── BMP280_CFS_APP_GUIDE.md    - Original guide
```

## 🚀 Quick Start

### 1. Extract Package
```bash
tar -xzf bmp280_app_with_led_control.tar.gz
cd bmp280_app
```

### 2. Install in cFS
```bash
# Copy to cFS apps directory
cp -r ../bmp280_app $CFS_PROJECT_DIR/apps/

# Add to targets.cmake
# list(APPEND MISSION_GLOBAL_APPLIST bmp280_app)

# Build
cd $CFS_PROJECT_DIR
make prep && make && make install
```

### 3. Hardware Setup
```
LED: GPIO 17 (Pin 11) → [LED + 330Ω] → GND
BMP280: I2C1 (Pins 3,5) + 3.3V (Pin 1) + GND (Pin 6)
```

### 4. Run cFS
```bash
cd build/exe/cpu1
sudo ./core-cpu1
```

### 5. Test with Python
```bash
cd /path/to/bmp280_app

# Simple test
python3 simple_cmd.py led-on

# Interactive menu
python3 bmp280_cmd_test.py --interactive

# Full test
python3 bmp280_cmd_test.py --test-all
```

## 🎮 All Telecommands

| Command | Code | Python (simple) | Python (full) |
|---------|------|----------------|---------------|
| NOOP | 0 | `noop` | `--noop` |
| Reset Counters | 1 | `reset` | `--reset` |
| Enable Sensor | 2 | `enable` | `--enable` |
| Disable Sensor | 3 | `disable` | `--disable` |
| **LED ON** | **4** | **`led-on`** | **`--led-on`** |
| **LED OFF** | **5** | **`led-off`** | **`--led-off`** |
| **LED Toggle** | **6** | **`toggle`** | **`--led-toggle`** |
| **LED Status** | **7** | **`status`** | **`--led-status`** |

## 📖 Documentation Guide

### Quick Start
1. **INSTALL.md** - Start here for installation
2. **PYTHON_README.md** - Quick Python script guide

### Testing
1. **TESTING_GUIDE.md** - Manual testing procedures
2. **PYTHON_SCRIPTS_GUIDE.md** - Complete Python docs

### Technical Details
1. **LED_CONTROL_GUIDE.md** - Complete LED implementation
2. **CHANGES_SUMMARY.md** - All changes made
3. **README_ENHANCED.md** - Feature overview

### Reference
1. **BMP280_CFS_APP_GUIDE.md** - Original app guide
2. **QUICKSTART.md** - Quick reference

## 💡 Usage Examples

### Python Scripts

**Simplest way:**
```bash
python3 simple_cmd.py led-on
python3 simple_cmd.py blink
```

**Interactive testing:**
```bash
python3 bmp280_cmd_test.py -i
# Then select commands from menu
```

**Automated testing:**
```bash
python3 bmp280_cmd_test.py --test-all
```

**Custom patterns:**
```bash
python3 bmp280_cmd_test.py --blink 10
python3 bmp280_cmd_test.py --rapid-toggle 20
```

**Remote cFS:**
```bash
python3 bmp280_cmd_test.py --host 192.168.1.100 --led-on
```

### Monitor Results
```bash
# Watch cFS events
tail -f /var/log/syslog | grep BMP280

# Expected output:
# BMP280: LED turned ON
# BMP280: LED turned OFF
# BMP280: LED toggled to ON
```

## 🔧 Configuration

### Change GPIO Pin
Edit `fsw/src/bmp280_gpio.c`:
```c
#define LED_GPIO_PIN 17  // Change to your GPIO
```

### Change Network Settings
**simple_cmd.py:**
```python
CFS_IP = '127.0.0.1'    # Change IP
CFS_CMD_PORT = 1234      # Change port
```

**bmp280_cmd_test.py:**
```bash
python3 bmp280_cmd_test.py --host 192.168.1.100 --port 1234
```

## ✅ Verification Checklist

After installation and testing:

- [ ] cFS starts without errors
- [ ] BMP280 app initializes
- [ ] "GPIO 17 initialized" event appears
- [ ] LED responds to ON command
- [ ] LED responds to OFF command
- [ ] LED toggles correctly
- [ ] Python scripts connect successfully
- [ ] Commands increment counter in HK telemetry
- [ ] Sensor readings still appear
- [ ] No error events in log

## 🐛 Troubleshooting

### Commands not working?
1. Check cFS is running: `ps aux | grep core-cpu1`
2. Check firewall: `sudo ufw allow 1234/udp`
3. Verify message ID in `bmp280_app_msgids.h`

### LED not responding?
1. Check GPIO initialization event
2. Test manually: `echo 1 > /sys/class/gpio/gpio17/value`
3. Check wiring (polarity, resistor)

### Python script errors?
1. Verify Python 3: `python3 --version`
2. Check host/port in script
3. Test network: `ping <cFS_IP>`

See detailed troubleshooting in **TESTING_GUIDE.md**

## 🎓 Learning Resources

This package demonstrates:
- ✅ Proper cFS telecommand implementation
- ✅ GPIO control from cFS applications
- ✅ Using cFS APIs (CFE_EVS, CFE_SB, OSAL)
- ✅ EDS message definitions
- ✅ Python UDP commanding
- ✅ Professional documentation
- ✅ Testing best practices

## 📊 Statistics

- **Source Files**: 3 (app.c, sensor.c, gpio.c)
- **Header Files**: 4
- **Python Scripts**: 2 (simple + full featured)
- **Documentation**: 9 comprehensive guides
- **Telecommands**: 8 (4 standard + 4 LED)
- **Event IDs**: 16 (with 6 new for LED/GPIO)
- **Lines of Code**: ~1500+ (including comments)
- **Documentation**: ~15,000+ words

## 🔄 Version Information

- **App Version**: 1.1.0 (with LED control)
- **cFS Compatibility**: Caelum or later
- **Target Hardware**: Raspberry Pi 5
- **GPIO Pin**: 17 (configurable)
- **Python**: 3.6+ (no external dependencies)

## 🆘 Support

All documentation is included in the package:

1. **Quick Help**: See `INSTALL.md` or `PYTHON_README.md`
2. **Testing**: See `TESTING_GUIDE.md` or `PYTHON_SCRIPTS_GUIDE.md`
3. **Technical**: See `LED_CONTROL_GUIDE.md` or `CHANGES_SUMMARY.md`

## 🎉 What You Can Do Now

1. ✅ Control LED with telecommands
2. ✅ Read BMP280 sensor data
3. ✅ Test with Python scripts
4. ✅ Run automated test sequences
5. ✅ Use interactive menu
6. ✅ Monitor via housekeeping telemetry
7. ✅ Customize GPIO pin
8. ✅ Extend with new commands

## 📝 Next Steps

1. Extract the package
2. Read `INSTALL.md`
3. Build and install in cFS
4. Connect hardware (LED + BMP280)
5. Run cFS
6. Test with Python scripts
7. Verify everything works
8. Customize as needed

---

**Everything you need is included!**

Ready to control your LED with cFS telecommands! 🚀

For detailed instructions, start with:
- **INSTALL.md** - Installation
- **PYTHON_README.md** - Testing with Python

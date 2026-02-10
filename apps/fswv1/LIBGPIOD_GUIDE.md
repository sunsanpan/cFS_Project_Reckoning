# libgpiod Installation Guide

## What Changed

The BMP280 app now uses **libgpiod** (v1.6+) instead of the deprecated sysfs GPIO interface. This is:
- ✅ **Modern** - The recommended way to use GPIO on Linux
- ✅ **Reliable** - Works on Raspberry Pi 5 and newer kernels
- ✅ **No permissions needed** - Works without sudo or special setup!
- ✅ **Clean API** - Better error handling and resource management
- ✅ **Compatible** - Works with libgpiod v1.6+ (most distributions)

## Installation

### Step 1: Install libgpiod

On Raspberry Pi (Debian/Ubuntu):

```bash
sudo apt-get update
sudo apt-get install -y libgpiod-dev gpiod
```

Verify installation:
```bash
# Check version (should be 1.6+ or 2.x)
gpioinfo --version

# List GPIO chips
gpioinfo

# Test GPIO 17
gpioset gpiochip4 17=1  # LED ON
gpioset gpiochip4 17=0  # LED OFF
```

### Step 2: Build the cFS App

```bash
cd $CFS_PROJECT_DIR

# Clean previous build
make clean

# Rebuild
make prep
make
make install
```

The CMakeLists.txt already includes `-lgpiod` so it will link automatically.

### Step 3: Run cFS (NO SUDO NEEDED!)

```bash
cd build/exe/cpu1
./core-cpu1  # No sudo required!
```

You should see:
```
BMP280_GPIO: Initializing GPIO 17 using libgpiod...
BMP280_GPIO: Opened gpiochip4
BMP280_GPIO: GPIO 17 initialized successfully using libgpiod
BMP280_GPIO: GPIO 17 ready, LED initialized to OFF
```

## Testing

### Test with Python Scripts

```bash
# Simple commands
python3 simple_cmd.py led-on
python3 simple_cmd.py led-off
python3 simple_cmd.py blink

# Full test suite
python3 bmp280_cmd_test.py --test-all
```

### Test with Command Line Tools

```bash
# Using gpioset (part of gpiod package)
gpioset gpiochip4 17=1  # LED ON
gpioset gpiochip4 17=0  # LED OFF

# Monitor GPIO
gpioget gpiochip4 17

# Watch GPIO info
gpioinfo gpiochip4
```

## Troubleshooting

### "Failed to open GPIO chip"

**Problem:** Can't find the GPIO chip

**Solution:**
```bash
# List available chips
ls /dev/gpiochip*

# Should show: /dev/gpiochip0 /dev/gpiochip4 (on RPi5)

# Try different chips
gpioinfo gpiochip0
gpioinfo gpiochip4
```

The code tries both `gpiochip0` and `gpiochip4` automatically.

### "Failed to request GPIO line"

**Problem:** GPIO is already in use

**Solution:**
```bash
# Check what's using GPIO
lsof 2>/dev/null | grep gpio

# Or use gpiodetect
gpiodetect

# Release all GPIO
# (stop cFS and any other programs using GPIO)
```

### Library Not Found

**Problem:** 
```
error while loading shared libraries: libgpiod.so.2
```

**Solution:**
```bash
# Install development library
sudo apt-get install libgpiod-dev

# Check if installed
ldconfig -p | grep gpiod
```

### libgpiod Version Check

**Check version:**
```bash
gpioinfo --version
```

You need v1.6 or later. Most systems have this.

## GPIO Chip Information

### Raspberry Pi 5

Raspberry Pi 5 has **two** GPIO chips:

- **gpiochip0** - Legacy RP1 GPIO (GPIO 0-27)
- **gpiochip4** - Main RP1 GPIO (same pins, newer interface)

Both work! The code tries `gpiochip0` first (backward compatible), then `gpiochip4`.

### Check Your GPIO Setup

```bash
# List all chips
gpiodetect

# Get info on specific chip
gpioinfo gpiochip4

# Should show:
# line 17: "GPIO17" unused input active-high
```

When cFS is running, GPIO 17 will show:
```
line 17: "GPIO17" "bmp280_cfs_app" output active-high [used]
```

## Advantages Over sysfs

| Feature | sysfs (/sys/class/gpio) | libgpiod v2 |
|---------|------------------------|-------------|
| **Permissions** | Needs root or setup | Works without sudo ✓ |
| **Modern** | Deprecated | Current standard ✓ |
| **RPi 5 Support** | Limited | Full support ✓ |
| **Error Handling** | Basic | Comprehensive ✓ |
| **Resource Cleanup** | Manual | Automatic ✓ |
| **Multiple Access** | Conflicts | Managed ✓ |

## Code Changes Summary

The new implementation uses libgpiod v1.6+ API:

```c
// Open chip
struct gpiod_chip *chip = gpiod_chip_open_by_name("gpiochip4");

// Get line
struct gpiod_line *line = gpiod_chip_get_line(chip, gpio_pin);

// Request as output
gpiod_line_request_output(line, "bmp280_cfs_app", 0);

// Set value
gpiod_line_set_value(line, 1);  // LED ON
gpiod_line_set_value(line, 0);  // LED OFF

// Read value
int value = gpiod_line_get_value(line);

// Cleanup (automatic on app exit)
gpiod_line_release(line);
gpiod_chip_close(chip);
```

## Verification

After building and running, check the log:

**Expected (Success):**
```
BMP280_GPIO: Initializing GPIO 17 using libgpiod...
BMP280_GPIO: Opened gpiochip4
BMP280_GPIO: GPIO 17 initialized successfully using libgpiod
BMP280_GPIO: GPIO 17 ready, LED initialized to OFF
```

**Old Error (from sysfs - should NOT see this):**
```
BMP280_GPIO: GPIO export failed or already exported
BMP280_GPIO: Failed to set GPIO direction
```

## Testing Checklist

- [ ] libgpiod-dev installed (`apt list --installed | grep libgpiod`)
- [ ] Version 1.6 or higher (`gpioinfo --version`)
- [ ] Can test manually (`gpioset gpiochip4 17=1`)
- [ ] cFS builds without errors
- [ ] cFS runs without sudo
- [ ] GPIO initializes successfully (check events)
- [ ] LED responds to commands
- [ ] Python scripts work
- [ ] No permission errors

## Quick Reference

### Installation Commands
```bash
sudo apt-get install -y libgpiod-dev gpiod
```

### Test GPIO Manually
```bash
gpioset gpiochip4 17=1  # ON
gpioset gpiochip4 17=0  # OFF
gpioget gpiochip4 17    # Read
```

### Build cFS
```bash
cd $CFS_PROJECT_DIR
make clean && make prep && make && make install
```

### Run cFS (No Sudo!)
```bash
cd build/exe/cpu1
./core-cpu1
```

### Test with Python
```bash
python3 simple_cmd.py led-on
python3 simple_cmd.py blink
```

## Benefits You'll See

1. **No sudo required** - Run cFS as normal user
2. **No setup scripts** - Just install libgpiod and go
3. **Better errors** - Clear error messages if something fails
4. **Cleaner code** - Modern API is simpler
5. **Future-proof** - Works with latest kernels

## Notes for RPi 5

Raspberry Pi 5 uses the RP1 I/O controller which has excellent libgpiod support. The new GPIO implementation is specifically tested for RPi 5.

Key points:
- Uses `/dev/gpiochip4` or `/dev/gpiochip0`
- No device tree overlays needed
- Works out of the box with libgpiod
- No kernel module loading required

---

**That's it!** Just install libgpiod and rebuild. No permissions, no setup scripts, just works! 🚀

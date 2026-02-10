# QUICK FIX - GPIO with libgpiod

## The app now uses libgpiod (Modern GPIO)

**Good news:** No more permission errors! No sudo needed!

## Installation (One Time)

```bash
# Install libgpiod
sudo apt-get update
sudo apt-get install -y libgpiod-dev gpiod

# Verify it works
gpioset gpiochip4 17=1  # LED should turn ON
gpioset gpiochip4 17=0  # LED should turn OFF
```

If that works, you're ready!

## Build cFS

```bash
cd $CFS_PROJECT_DIR
make clean
make prep
make
make install
```

## Run cFS (NO SUDO!)

```bash
cd build/exe/cpu1
./core-cpu1  # No sudo required!
```

You should see:
```
BMP280_GPIO: Initializing GPIO 17 using libgpiod...
BMP280_GPIO: Opened gpiochip4
BMP280_GPIO: GPIO 17 initialized successfully using libgpiod
```

## Test with Python

```bash
python3 simple_cmd.py led-on   # LED turns on!
python3 simple_cmd.py led-off  # LED turns off!
python3 simple_cmd.py blink    # LED blinks!
```

---

## Troubleshooting

### "Failed to open GPIO chip"

```bash
# Check available chips
ls /dev/gpiochip*

# Try manual test
gpioinfo gpiochip4
```

### "Library not found"

```bash
# Reinstall
sudo apt-get install --reinstall libgpiod-dev
```

### Still not working?

See **LIBGPIOD_GUIDE.md** for detailed help.

---

**That's it!** Just install libgpiod, rebuild, and run. No permissions, no sudo! 🎉

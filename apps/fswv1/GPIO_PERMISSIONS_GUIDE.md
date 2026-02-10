# GPIO Permissions Setup Guide

## Problem

The error you're seeing:
```
BMP280_GPIO: GPIO export failed or already exported
BMP280_GPIO: Failed to set GPIO direction
BMP280: GPIO initialization failed
```

This happens because the cFS application doesn't have permission to access `/sys/class/gpio/` files.

## Quick Fix (Easiest!)

Run the provided setup script **once** before starting cFS:

```bash
cd /path/to/bmp280_app
sudo ./setup_gpio.sh
```

This script will:
- Export GPIO 17
- Set it as output
- Test the LED
- Set proper permissions
- Add your user to the gpio group

Then you can run cFS **without sudo**:
```bash
cd $CFS_PROJECT_DIR/build/exe/cpu1
./core-cpu1  # No sudo needed!
```

## Alternative: Run cFS as Root (Quick Test)

For quick testing, run cFS as root:

```bash
cd $CFS_PROJECT_DIR/build/exe/cpu1
sudo ./core-cpu1
```

**Note:** Running as root is not recommended for production but works for testing.

## Permanent Solution: udev Rules

For a permanent solution that works across reboots:

### 1. Create udev Rules

```bash
sudo nano /etc/udev/rules.d/99-gpio.rules
```

Add these lines:
```
# Allow gpio group to access GPIO
SUBSYSTEM=="gpio", KERNEL=="gpiochip*", GROUP="gpio", MODE="0660"
SUBSYSTEM=="gpio", KERNEL=="gpio*", GROUP="gpio", MODE="0660"

# Auto-export GPIO 17 for BMP280 LED
SUBSYSTEM=="gpio", ACTION=="add", KERNEL=="gpio17", RUN+="/bin/sh -c 'echo out > /sys/class/gpio/gpio17/direction'"
```

### 2. Create gpio Group (if it doesn't exist)

```bash
sudo groupadd gpio
```

### 3. Add Your User to gpio Group

```bash
sudo usermod -a -G gpio $USER
```

### 4. Reload udev Rules

```bash
sudo udevadm control --reload-rules
sudo udevadm trigger
```

### 5. Log Out and Back In

For group membership to take effect:
```bash
logout
# Then log back in
```

### 6. Verify Group Membership

```bash
groups
# Should include 'gpio'
```

## Manual Setup (If Script Doesn't Work)

### Step 1: Export GPIO

```bash
# As root
sudo su
echo 17 > /sys/class/gpio/export
```

### Step 2: Set Direction

```bash
echo out > /sys/class/gpio/gpio17/direction
```

### Step 3: Test

```bash
# LED should turn on
echo 1 > /sys/class/gpio/gpio17/value

# LED should turn off
echo 0 > /sys/class/gpio/gpio17/value
```

### Step 4: Set Permissions

```bash
# Create gpio group
groupadd gpio

# Add your user
usermod -a -G gpio $USER

# Set ownership
chown -R root:gpio /sys/class/gpio/gpio17
chmod -R 770 /sys/class/gpio/gpio17

# Set export/unexport permissions
chown root:gpio /sys/class/gpio/export /sys/class/gpio/unexport
chmod 220 /sys/class/gpio/export /sys/class/gpio/unexport

exit  # Exit root
```

### Step 5: Log Out and Back In

```bash
logout
# Log back in for group membership to take effect
```

## Systemd Service (Auto-Setup on Boot)

Create `/etc/systemd/system/bmp280-gpio.service`:

```ini
[Unit]
Description=Setup GPIO for BMP280 LED
After=multi-user.target

[Service]
Type=oneshot
ExecStart=/usr/local/bin/setup_gpio.sh
RemainAfterExit=yes

[Install]
WantedBy=multi-user.target
```

Then:
```bash
# Copy setup script
sudo cp setup_gpio.sh /usr/local/bin/

# Enable service
sudo systemctl daemon-reload
sudo systemctl enable bmp280-gpio.service
sudo systemctl start bmp280-gpio.service

# Check status
sudo systemctl status bmp280-gpio.service
```

## Verification

After setup, verify GPIO is accessible without sudo:

```bash
# Should work without sudo
cat /sys/class/gpio/gpio17/value
echo 1 > /sys/class/gpio/gpio17/value
echo 0 > /sys/class/gpio/gpio17/value
```

If these commands work **without sudo**, your permissions are correct!

## Raspberry Pi 5 Specific Notes

Raspberry Pi 5 uses a different GPIO controller (RP1). The sysfs interface still works but:

1. Make sure kernel modules are loaded:
```bash
lsmod | grep gpio
```

2. Check if GPIO 17 exists:
```bash
ls -l /sys/class/gpio/
```

3. If sysfs GPIO is deprecated, consider using libgpiod (see next section)

## Alternative: Using libgpiod (Modern Method)

If sysfs GPIO is giving you trouble, you can use the modern libgpiod approach:

### Install libgpiod

```bash
sudo apt-get update
sudo apt-get install -y gpiod libgpiod-dev
```

### Test with gpioset/gpioget

```bash
# Turn LED on
gpioset gpiochip0 17=1

# Turn LED off
gpioset gpiochip0 17=0

# Read value
gpioget gpiochip0 17
```

### Modify the App

If you want to use libgpiod in the cFS app instead of sysfs, I can provide an updated `bmp280_gpio.c` that uses libgpiod. Let me know!

## Troubleshooting

### "Permission denied" on export

**Symptom:**
```bash
bash: /sys/class/gpio/export: Permission denied
```

**Solution:**
```bash
sudo ./setup_gpio.sh
```

### GPIO already exported

**Symptom:**
```
write: Device or resource busy
```

**Solution:**
```bash
sudo su
echo 17 > /sys/class/gpio/unexport
sleep 1
echo 17 > /sys/class/gpio/export
exit
```

### Changes don't persist across reboot

**Solution:** Use the systemd service method above

### User not in gpio group

**Check:**
```bash
groups | grep gpio
```

**Fix:**
```bash
sudo usermod -a -G gpio $USER
# Log out and back in
```

### GPIO direction keeps resetting

**Check if something else is using GPIO 17:**
```bash
lsof 2>/dev/null | grep gpio17
```

### Can't access GPIO files

**Check file permissions:**
```bash
ls -l /sys/class/gpio/gpio17/
```

Should show:
```
drwxrwx--- ... root gpio ... /sys/class/gpio/gpio17/
```

## Quick Diagnostic

Run this to check your setup:

```bash
echo "=== GPIO Diagnostic ==="
echo "1. Checking GPIO existence:"
ls -ld /sys/class/gpio/gpio17 2>/dev/null && echo "  ✓ GPIO 17 exists" || echo "  ✗ GPIO 17 not exported"

echo ""
echo "2. Checking permissions:"
ls -l /sys/class/gpio/gpio17/direction 2>/dev/null | head -1

echo ""
echo "3. Checking group membership:"
groups | grep gpio && echo "  ✓ User in gpio group" || echo "  ✗ User NOT in gpio group"

echo ""
echo "4. Testing write access:"
if echo "out" > /sys/class/gpio/gpio17/direction 2>/dev/null; then
    echo "  ✓ Can write to GPIO (good!)"
else
    echo "  ✗ Cannot write to GPIO (needs setup)"
fi

echo ""
echo "5. Testing value access:"
cat /sys/class/gpio/gpio17/value 2>/dev/null && echo "  ✓ Can read GPIO value" || echo "  ✗ Cannot read GPIO value"
```

## Summary of Solutions

| Problem | Quick Fix | Permanent Fix |
|---------|-----------|---------------|
| Permission denied | `sudo ./setup_gpio.sh` | udev rules + gpio group |
| Can't export GPIO | Run cFS as root | systemd service |
| Works but not after reboot | Run setup script | systemd service + udev |
| GPIO busy | Unexport first | Check what's using it |

## Recommended Approach

**For Testing:**
1. Run `sudo ./setup_gpio.sh`
2. Run cFS without sudo

**For Production:**
1. Set up udev rules
2. Create systemd service
3. Add user to gpio group
4. Reboot to verify it persists

## Support

If you're still having issues:

1. Run the diagnostic script above
2. Check `dmesg | grep gpio` for kernel messages
3. Verify hardware connections
4. Try manual GPIO control first
5. Consider using libgpiod instead of sysfs

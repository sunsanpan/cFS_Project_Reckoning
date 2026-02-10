#!/bin/bash
#
# GPIO Setup Script for BMP280 cFS App
# Sets up GPIO 17 with proper permissions for LED control
#

set -e

GPIO_PIN=17
GPIO_GROUP="gpio"

echo "=========================================="
echo "BMP280 GPIO Setup Script"
echo "=========================================="
echo "GPIO Pin: $GPIO_PIN"
echo ""

# Check if running as root
if [ "$EUID" -ne 0 ]; then 
    echo "ERROR: This script must be run as root (sudo)"
    echo "Usage: sudo ./setup_gpio.sh"
    exit 1
fi

echo "[1/5] Checking if GPIO $GPIO_PIN is already exported..."
if [ -d "/sys/class/gpio/gpio$GPIO_PIN" ]; then
    echo "  GPIO $GPIO_PIN is already exported, unexporting first..."
    echo $GPIO_PIN > /sys/class/gpio/unexport
    sleep 0.5
fi
echo "  ✓ GPIO ready to export"

echo ""
echo "[2/5] Exporting GPIO $GPIO_PIN..."
echo $GPIO_PIN > /sys/class/gpio/export
sleep 0.5
echo "  ✓ GPIO exported"

echo ""
echo "[3/5] Setting GPIO direction to output..."
echo "out" > /sys/class/gpio/gpio$GPIO_PIN/direction
echo "  ✓ Direction set to output"

echo ""
echo "[4/5] Testing GPIO (LED should blink)..."
echo "1" > /sys/class/gpio/gpio$GPIO_PIN/value
echo "  LED should be ON now..."
sleep 1
echo "0" > /sys/class/gpio/gpio$GPIO_PIN/value
echo "  LED should be OFF now..."
sleep 0.5
echo "  ✓ GPIO test complete"

echo ""
echo "[5/5] Setting permissions for non-root access..."

# Create gpio group if it doesn't exist
if ! getent group $GPIO_GROUP > /dev/null 2>&1; then
    echo "  Creating $GPIO_GROUP group..."
    groupadd $GPIO_GROUP
fi

# Add current user to gpio group (if sudo was used)
if [ -n "$SUDO_USER" ]; then
    echo "  Adding user $SUDO_USER to $GPIO_GROUP group..."
    usermod -a -G $GPIO_GROUP $SUDO_USER
fi

# Set ownership and permissions
echo "  Setting GPIO file permissions..."
chown -R root:$GPIO_GROUP /sys/class/gpio/gpio$GPIO_PIN
chmod -R 770 /sys/class/gpio/gpio$GPIO_PIN
chown root:$GPIO_GROUP /sys/class/gpio/export /sys/class/gpio/unexport
chmod 220 /sys/class/gpio/export /sys/class/gpio/unexport

echo "  ✓ Permissions set"

echo ""
echo "=========================================="
echo "✓ GPIO Setup Complete!"
echo "=========================================="
echo ""
echo "GPIO $GPIO_PIN is now ready for use."
echo "The cFS app can now control the LED."
echo ""
echo "IMPORTANT: If you added a new user to the gpio group,"
echo "           you need to log out and log back in for the"
echo "           group membership to take effect."
echo ""
echo "To test manually:"
echo "  echo 1 > /sys/class/gpio/gpio$GPIO_PIN/value  # LED ON"
echo "  echo 0 > /sys/class/gpio/gpio$GPIO_PIN/value  # LED OFF"
echo ""
echo "To make this persistent across reboots:"
echo "  1. Copy this script to /usr/local/bin/"
echo "  2. Create a systemd service to run it at boot"
echo "  3. Or add udev rules (see GPIO_PERMISSIONS_GUIDE.md)"
echo ""

#!/bin/bash
# UART Debug Script - Check everything

echo "=========================================="
echo "FSWV1 UART Debug Checker"
echo "=========================================="
echo

# 1. Check if UART device exists
echo "1. Checking UART device..."
if [ -e /dev/ttyAMA0 ]; then
    echo "   ✓ /dev/ttyAMA0 exists"
    ls -l /dev/ttyAMA0
else
    echo "   ✗ /dev/ttyAMA0 NOT FOUND"
    echo "   Try: sudo raspi-config → Interface Options → Serial Port"
fi
echo

# 2. Check permissions
echo "2. Checking permissions..."
if [ -r /dev/ttyAMA0 ] && [ -w /dev/ttyAMA0 ]; then
    echo "   ✓ Can read and write /dev/ttyAMA0"
else
    echo "   ✗ Permission denied"
    echo "   Try: sudo usermod -a -G dialout $USER"
    echo "   Then log out and back in"
fi
echo

# 3. Check if user is in dialout group
echo "3. Checking dialout group..."
if groups | grep -q dialout; then
    echo "   ✓ User is in dialout group"
else
    echo "   ✗ User NOT in dialout group"
    echo "   Run: sudo usermod -a -G dialout $USER"
    echo "   Then log out and back in"
fi
echo

# 4. Check UART configuration
echo "4. Checking UART config..."
if [ -f /boot/config.txt ]; then
    if grep -q "enable_uart=1" /boot/config.txt; then
        echo "   ✓ UART enabled in /boot/config.txt"
    else
        echo "   ⚠ UART may not be enabled"
        echo "   Add: enable_uart=1 to /boot/config.txt"
    fi
elif [ -f /boot/firmware/config.txt ]; then
    if grep -q "enable_uart=1" /boot/firmware/config.txt; then
        echo "   ✓ UART enabled in /boot/firmware/config.txt"
    else
        echo "   ⚠ UART may not be enabled"
        echo "   Add: enable_uart=1 to /boot/firmware/config.txt"
    fi
else
    echo "   ⚠ Could not find config.txt"
fi
echo

# 5. Check if something is using UART
echo "5. Checking if UART is in use..."
if lsof /dev/ttyAMA0 2>/dev/null; then
    echo "   ⚠ UART is being used by another process"
else
    echo "   ✓ UART is free"
fi
echo

# 6. Try to read UART settings
echo "6. Checking UART settings..."
if command -v stty >/dev/null 2>&1; then
    if stty -F /dev/ttyAMA0 2>/dev/null; then
        echo "   ✓ UART settings readable"
        stty -F /dev/ttyAMA0 speed
    else
        echo "   ✗ Cannot read UART settings (permission issue?)"
    fi
else
    echo "   ⚠ stty not available"
fi
echo

# 7. Test UART loopback
echo "7. Testing UART write..."
if echo "TEST" > /dev/ttyAMA0 2>/dev/null; then
    echo "   ✓ Can write to UART"
else
    echo "   ✗ Cannot write to UART"
fi
echo

echo "=========================================="
echo "FIXES:"
echo "=========================================="
echo "If UART not found:"
echo "  sudo raspi-config"
echo "  → Interface Options → Serial Port"
echo "  → Login shell: NO"
echo "  → Hardware enabled: YES"
echo "  sudo reboot"
echo
echo "If permission denied:"
echo "  sudo usermod -a -G dialout \$USER"
echo "  Then log out and log back in"
echo
echo "If UART in use:"
echo "  Check what's using it: sudo lsof /dev/ttyAMA0"
echo "  Kill process or disable getty/console on UART"
echo
echo "=========================================="

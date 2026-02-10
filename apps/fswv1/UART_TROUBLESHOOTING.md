# UART Troubleshooting Guide

## Step 1: Run Debug Script

```bash
chmod +x debug_uart.sh
./debug_uart.sh
```

This checks:
- UART device exists
- Permissions
- Configuration
- If something else is using UART

---

## Step 2: Test UART Hardware

### Test A: Simple Loopback (if you have a wire)
```bash
# Connect GPIO 14 (TX) to GPIO 15 (RX) with a wire

# Terminal 1
cat /dev/ttyAMA0

# Terminal 2
echo "TEST123" > /dev/ttyAMA0

# You should see "TEST123" in Terminal 1
```

### Test B: Standalone Test Program
```bash
# Compile
gcc -o uart_test uart_test.c

# Run in Terminal 1
./uart_test

# Send data in Terminal 2
echo '$,0.05,-0.12,9.81,0.01,-0.02,0.00,25.5,#' > /dev/ttyAMA0
```

**Expected output in Terminal 1:**
```
✓ Packet 1 received: $,0.05,-0.12,9.81,0.01,-0.02,0.00,25.5,#
  Parsed successfully!
  Accel: X=0.05 Y=-0.12 Z=9.81
  Gyro:  X=0.01 Y=-0.02 Z=0.00
  Temp:  25.50 °C
```

---

## Step 3: Check cFS is Reading UART

### Add Debug Prints

Edit `/home/ares/Desktop/cubesat/fswv1/cFS/apps/fswv1/fsw/src/fswv1_uart.c`

Find the `FSWV1_ReadUART` function and add this at the start:

```c
int32 FSWV1_ReadUART(FSWV1_IMUData_t *data)
{
    char byte;
    ssize_t bytes_read;
    
    OS_printf("DEBUG: FSWV1_ReadUART called\n");  // <-- ADD THIS
    
    if (!UART_Initialized || uart_fd < 0)
    {
        OS_printf("DEBUG: UART not initialized!\n");  // <-- ADD THIS
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }
    // ... rest of function
```

Rebuild and run. You should see:
```
DEBUG: FSWV1_ReadUART called
DEBUG: FSWV1_ReadUART called
DEBUG: FSWV1_ReadUART called
```

If you DON'T see this, the function is not being called!

---

## Step 4: Check IMUEnabled Flag

In cFS console, when the app starts, you should see:
```
FSWV1: UART initialized on /dev/ttyAMA0 at 115200 baud
```

If you see:
```
FSWV1: UART initialization failed
```

Then UART failed to open. Check permissions!

---

## Step 5: Verify Data Flow

Add more debug in the parse function. Edit `fswv1_uart.c`:

```c
static int32 ParseIMUData(const char *str, FSWV1_IMUData_t *data)
{
    int parsed;
    
    OS_printf("DEBUG: Parsing: %s\n", str);  // <-- ADD THIS
    
    parsed = sscanf(str, "$,%f,%f,%f,%f,%f,%f,%f,#",
                    &data->Accel_X,
                    &data->Accel_Y,
                    &data->Accel_Z,
                    &data->Gyro_X,
                    &data->Gyro_Y,
                    &data->Gyro_Z,
                    &data->Temperature);
    
    OS_printf("DEBUG: Parsed %d values\n", parsed);  // <-- ADD THIS
    
    if (parsed != 7)
    {
        OS_printf("DEBUG: Parse failed!\n");  // <-- ADD THIS
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }
    
    OS_printf("DEBUG: Successfully parsed IMU data\n");  // <-- ADD THIS
    // ... rest
```

---

## Common Issues & Fixes

### Issue 1: Permission Denied
```bash
sudo usermod -a -G dialout $USER
# Log out and back in!
```

### Issue 2: UART Not Enabled
```bash
sudo raspi-config
# Interface Options → Serial Port
# Login shell over serial: NO
# Serial port hardware: YES
sudo reboot
```

### Issue 3: Getty Using UART
```bash
# Check
sudo systemctl status serial-getty@ttyAMA0.service

# Disable
sudo systemctl stop serial-getty@ttyAMA0.service
sudo systemctl disable serial-getty@ttyAMA0.service
```

### Issue 4: Bluetooth Using UART (Older Pi)
```bash
# Add to /boot/config.txt or /boot/firmware/config.txt
dtoverlay=disable-bt

# Reboot
sudo reboot
```

### Issue 5: Wrong UART Device
Try these alternatives:
- `/dev/ttyAMA0` (Primary UART, Pi 5/4)
- `/dev/ttyS0` (Mini UART, older Pi)
- `/dev/ttyUSB0` (USB-to-Serial adapter)

```bash
ls -l /dev/tty*
```

---

## Quick Test Sequence

```bash
# 1. Check hardware
./debug_uart.sh

# 2. Test standalone
gcc -o uart_test uart_test.c
./uart_test
# In another terminal:
echo '$,0.05,-0.12,9.81,0.01,-0.02,0.00,25.5,#' > /dev/ttyAMA0

# 3. If above works, rebuild cFS with debug prints
cd $CFS_PROJECT_DIR
make clean
make
make install

# 4. Run cFS
cd build/exe/cpu1
./core-cpu1

# 5. Send UART data
echo '$,0.05,-0.12,9.81,0.01,-0.02,0.00,25.5,#' > /dev/ttyAMA0

# 6. Check cFS console for:
#    - "DEBUG: FSWV1_ReadUART called"
#    - "DEBUG: Parsing: ..."
#    - "FSWV1: IMU Ax=0.05 ..."
```

---

## Still Not Working?

### Check cFS is actually calling UART read

Add this to the main loop in `fswv1_app.c` (after line ~80):

```c
/* Read IMU data from UART (always, independent of SensorEnabled) */
OS_printf("DEBUG: About to read UART, IMUEnabled=%d\n", FSWV1_APP_Data.IMUEnabled);  // ADD
if (FSWV1_APP_Data.IMUEnabled)
{
    status = FSWV1_ReadUART(&FSWV1_APP_Data.IMUData);
    OS_printf("DEBUG: UART read returned status=0x%08X\n", status);  // ADD
    // ...
```

This will show if the read is being attempted.

---

## What Should Work

If UART is working properly:

**Terminal 1 (cFS):**
```
FSWV1: BMP Temp=26.18°C, Press=943.05 Pa
FSWV1: IMU Ax=0.05 Ay=-0.12 Az=9.81 Gx=0.01 Gy=-0.02 Gz=0.00 T=25.50
```

**Terminal 2 (UDP receiver):**
```
BMP_Temperature:  26.18 °C
BMP_Pressure:     943.05 Pa
Accel_X:          0.05
Accel_Y:         -0.12
Accel_Z:          9.81
Gyro_X:           0.01
Gyro_Y:          -0.02
Gyro_Z:           0.00
IMU_Temperature:  25.50 °C
```

---

**Start with Step 1 and work through each step!**

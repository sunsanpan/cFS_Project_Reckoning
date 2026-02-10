# Quick Testing Guide - BMP280 App with LED Control

## Hardware Quick Check

### Test LED Manually (Before running cFS)

```bash
# Export GPIO 17
echo 17 | sudo tee /sys/class/gpio/export

# Set as output
echo out | sudo tee /sys/class/gpio/gpio17/direction

# Turn ON (LED should light up)
echo 1 | sudo tee /sys/class/gpio/gpio17/value

# Turn OFF (LED should turn off)
echo 0 | sudo tee /sys/class/gpio/gpio17/value

# Cleanup
echo 17 | sudo tee /sys/class/gpio/unexport
```

If this works, your LED is wired correctly!

## Running the cFS App

### Start cFS

```bash
cd $CFS_PROJECT_DIR/build/exe/cpu1
sudo ./core-cpu1
```

### Monitor Event Messages

In another terminal:
```bash
tail -f /var/log/syslog | grep BMP280
```

Or check cFS event log directly.

## Command Testing Sequence

### Test 1: Basic Commands

```bash
# NOOP - Should increment command counter
cmdUtil --pktid=0x1882 --cmdcode=0 --endian=LE

# Reset Counters
cmdUtil --pktid=0x1882 --cmdcode=1 --endian=LE
```

**Expected**: Event messages confirming commands received

### Test 2: LED Control

```bash
# Turn LED ON
cmdUtil --pktid=0x1882 --cmdcode=4 --endian=LE
# Check: LED should light up
# Event: "BMP280: LED turned ON"

# Turn LED OFF  
cmdUtil --pktid=0x1882 --cmdcode=5 --endian=LE
# Check: LED should turn off
# Event: "BMP280: LED turned OFF"

# Toggle LED (should turn ON if currently OFF)
cmdUtil --pktid=0x1882 --cmdcode=6 --endian=LE
# Check: LED state should change
# Event: "BMP280: LED toggled to ON/OFF"

# Query LED Status
cmdUtil --pktid=0x1882 --cmdcode=7 --endian=LE
# Event: "BMP280: LED status is ON/OFF"
```

### Test 3: Sensor Control

```bash
# Disable sensor readings
cmdUtil --pktid=0x1882 --cmdcode=3 --endian=LE
# Check: Temperature/Pressure readings should stop

# Enable sensor readings
cmdUtil --pktid=0x1882 --cmdcode=2 --endian=LE
# Check: Temperature/Pressure readings should resume
```

### Test 4: Combined Operations

```bash
# Rapid LED blink test
for i in {1..10}; do
  cmdUtil --pktid=0x1882 --cmdcode=6 --endian=LE
  sleep 0.5
done
# Check: LED should blink 10 times
```

## Monitoring Telemetry

### Using cFS TO (Telemetry Output)

The housekeeping telemetry includes:
- CommandCounter
- CommandErrorCounter  
- SensorEnabled
- ReadRate
- **LedState** (0=OFF, 1=ON)

### Using to_lab or Ground System

Configure your ground system to display the HK telemetry packet and watch the LedState field change as you send commands.

## Expected Event Messages

### On Startup
```
BMP280 App Initialized. Version 1.0.0.0
BMP280_GPIO: GPIO 17 initialized successfully
```

### On LED Commands
```
BMP280: LED turned ON
BMP280: LED turned OFF
BMP280: LED toggled to ON
BMP280: LED status is OFF
```

### On Sensor Commands
```
BMP280: Sensor ENABLED
BMP280: Sensor DISABLED
```

### On Sensor Readings (when enabled)
```
BMP280: Temp=25.34°C, Press=101325.00 Pa
```

## Troubleshooting Quick Checks

### LED Not Responding

1. Check GPIO permissions:
```bash
ls -l /sys/class/gpio/
```

2. Check if GPIO is exported:
```bash
ls /sys/class/gpio/ | grep gpio17
```

3. Manually test GPIO:
```bash
echo 1 | sudo tee /sys/class/gpio/gpio17/value
```

### Commands Not Working

1. Verify packet ID in your msgids file:
```bash
grep CMD_MID bmp280_app_msgids.h
```

2. Check if app is running:
```bash
ps aux | grep core-cpu1
```

3. Check command counter in HK telemetry - should increment

### Sensor Not Reading

1. Check I2C connection:
```bash
sudo i2cdetect -y 1
```
Should show device at 0x76 or 0x77

2. Check event log for sensor errors

## Verification Checklist

- [ ] cFS starts without errors
- [ ] BMP280 app initializes successfully
- [ ] GPIO initialized (event message)
- [ ] LED responds to ON command
- [ ] LED responds to OFF command
- [ ] LED toggles correctly
- [ ] LED status reports correctly
- [ ] Sensor readings appear in telemetry
- [ ] Enable/Disable commands work
- [ ] Command counters increment
- [ ] No error events in log
- [ ] Housekeeping telemetry shows correct LED state

## Performance Tests

### LED Response Time

```bash
# Measure time from command to LED change
time cmdUtil --pktid=0x1882 --cmdcode=4 --endian=LE
```

Should be < 100ms for responsive control.

### Concurrent Operations

```bash
# Send commands while sensor is reading
# LED control should not affect sensor readings
cmdUtil --pktid=0x1882 --cmdcode=6 --endian=LE  # Toggle LED
```

Check: Sensor telemetry should continue normally.

## Notes

- Default GPIO is 17 (Physical Pin 11)
- Default sensor read rate is 1 Hz (adjustable)
- LED control is non-blocking
- All commands use cFS standard message format
- Event messages use cFS EVS (Event Services)

## Success Criteria

The app is working correctly when:
1. All commands execute without errors
2. LED visibly responds to commands
3. LED state in telemetry matches physical LED
4. Sensor readings continue independently
5. No permission or resource errors in logs

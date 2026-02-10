# Message ID Quick Reference

## Your BMP280 App Message IDs

Based on `bmp280_app_msgids.h`:

```
BMP280_APP_CMD_MID       = 0x1884  ← Use this for commands!
BMP280_APP_SEND_HK_MID   = 0x1885
BMP280_APP_HK_TLM_MID    = 0x0884
BMP280_APP_SENSOR_TLM_MID = 0x0885
```

## If Commands Still Don't Work

### Option 1: Use the Finder Script

```bash
python3 find_msgid.py
```

This will try different message IDs. Watch your cFS events to see which one gets "BMP280: NOOP command".

### Option 2: Check the Build Directory

After building, the actual message IDs are in:

```bash
cat $CFS_PROJECT_DIR/build/inc/bmp280_app_msgids.h
```

Look for the `BMP280_APP_CMD_MID` value.

### Option 3: Check Your Mission Config

Your mission might assign different message IDs. Check:

```bash
cat $CFS_PROJECT_DIR/sample_defs/cpu1_msgids.h
# or
cat $CFS_PROJECT_DIR/<your_mission>/cpu1/cpu1_msgids.h
```

## Current Status

**SAMPLE_APP is receiving the commands** - this means:
- Your Python script is working
- The command is reaching cFS
- But the message ID (0x1882) was wrong
- SAMPLE_APP has MID 0x1882

**Now fixed to 0x1884** - BMP280_APP should receive commands now!

## Test It

```bash
python3 simple_cmd.py led-on
```

You should see:
```
BMP280: LED turned ON
```

Instead of:
```
SAMPLE_APP: Invalid ground command code: CC = 4
```

## If Still Wrong

Run the finder:
```bash
python3 find_msgid.py
```

Watch for which MID shows:
```
BMP280: NOOP command
```

Then update both Python scripts with that MID.

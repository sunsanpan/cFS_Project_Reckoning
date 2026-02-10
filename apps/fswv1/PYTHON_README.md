# Quick Start - Python Testing Scripts

## Two Scripts Provided

### 1. simple_cmd.py (Easiest to use!)

**Quick LED control:**
```bash
python3 simple_cmd.py led-on      # Turn LED on
python3 simple_cmd.py led-off     # Turn LED off
python3 simple_cmd.py toggle      # Toggle LED
python3 simple_cmd.py blink       # Blink 5 times
```

### 2. bmp280_cmd_test.py (Full featured)

**Interactive menu:**
```bash
python3 bmp280_cmd_test.py --interactive
```

**Single commands:**
```bash
python3 bmp280_cmd_test.py --led-on
python3 bmp280_cmd_test.py --led-off
python3 bmp280_cmd_test.py --blink 10
```

**Run all tests:**
```bash
python3 bmp280_cmd_test.py --test-all
```

## Requirements

- Python 3 (already installed on Raspberry Pi)
- cFS running on localhost or network
- No pip packages needed!

## First Test

1. Start cFS:
```bash
cd $CFS_PROJECT_DIR/build/exe/cpu1
sudo ./core-cpu1
```

2. In another terminal:
```bash
cd /path/to/bmp280_app
python3 simple_cmd.py led-on
```

3. Check:
- LED should light up
- cFS events show: "BMP280: LED turned ON"

## Remote cFS

If cFS is on another machine:

**simple_cmd.py** - Edit the file:
```python
CFS_IP = '192.168.1.100'  # Change this
```

**bmp280_cmd_test.py** - Command line:
```bash
python3 bmp280_cmd_test.py --host 192.168.1.100 --led-on
```

## All Available Commands

| Command | Code | Action |
|---------|------|--------|
| noop | 0 | Test command |
| reset | 1 | Reset counters |
| enable | 2 | Enable sensor |
| disable | 3 | Disable sensor |
| led-on | 4 | Turn LED on |
| led-off | 5 | Turn LED off |
| toggle | 6 | Toggle LED |
| status | 7 | Query LED state |

## Examples

```bash
# Simple script - short commands
python3 simple_cmd.py led-on
python3 simple_cmd.py led-off
python3 simple_cmd.py toggle
python3 simple_cmd.py blink

# Full script - descriptive commands
python3 bmp280_cmd_test.py --led-on
python3 bmp280_cmd_test.py --led-off
python3 bmp280_cmd_test.py --led-toggle
python3 bmp280_cmd_test.py --blink 5

# Full script - test sequences
python3 bmp280_cmd_test.py --test-led
python3 bmp280_cmd_test.py --test-all

# Interactive mode (best for exploring)
python3 bmp280_cmd_test.py --interactive
```

## Monitoring

Open a second terminal to watch events:
```bash
tail -f /var/log/syslog | grep BMP280
```

You should see:
```
BMP280: LED turned ON
BMP280: LED turned OFF
BMP280: LED toggled to ON
```

## Troubleshooting

**Commands sent but nothing happens?**
1. Check cFS is running: `ps aux | grep core-cpu1`
2. Check firewall: `sudo ufw allow 1234/udp`
3. Verify IP address in script matches cFS host

**LED not responding?**
1. Check GPIO initialization in cFS events
2. Test LED manually: `echo 1 > /sys/class/gpio/gpio17/value`
3. Check LED wiring (resistor, polarity)

**Network error?**
1. Verify cFS IP: `ifconfig` or `ip addr`
2. Test connectivity: `ping <cFS_IP>`
3. Check port: `netstat -anu | grep 1234`

## Full Documentation

See `PYTHON_SCRIPTS_GUIDE.md` for complete documentation including:
- Detailed usage examples
- All command-line options
- Advanced testing sequences
- Script architecture
- Customization guide

## Tips

1. **Start with simple_cmd.py** - easier to use
2. **Use interactive mode** for manual testing
3. **Monitor events** while testing
4. **Watch the LED** for visual confirmation
5. **Check HK telemetry** for LED state

Happy testing! 🚀

# UDP Telemetry Receiver Guide

## Quick Start (Simple Version)

```bash
python3 udp_telemetry_receiver.py
```

**Output:**
```
======================================================================
FSWV1 Telemetry - 14:23:45.123
======================================================================

📡 BMP280 Sensor (I2C):
  Temperature:   26.18 °C
  Pressure:     943.05 Pa

🚀 IMU Data (UART):
  Accelerometer:
    X:    0.05
    Y:   -0.12
    Z:    9.81
  Gyroscope:
    X:    0.01
    Y:   -0.02
    Z:    0.00
  Temperature:   25.50 °C

⏱️  Timestamp: 12345 seconds
======================================================================
```

---

## Advanced Version (Multiple Modes)

### 1. Detailed Mode (Default)
```bash
python3 udp_receiver_advanced.py
```

### 2. Compact Mode (One Line Per Packet)
```bash
python3 udp_receiver_advanced.py -m compact
```

**Output:**
```
[14:23:45.123] BMP:26.2°C 943Pa | A: 0.05,-0.12, 9.81 | G: 0.01,-0.02, 0.00 | T:25.5°C
[14:23:46.125] BMP:26.2°C 943Pa | A: 0.04,-0.11, 9.80 | G: 0.00,-0.01, 0.01 | T:25.5°C
[14:23:47.127] BMP:26.1°C 943Pa | A: 0.05,-0.12, 9.82 | G: 0.01,-0.02,-0.01 | T:25.6°C
```

### 3. CSV Mode (Easy to Import)
```bash
python3 udp_receiver_advanced.py -m csv
```

**Output:**
```
Time,BMP_Temp,BMP_Press,Accel_X,Accel_Y,Accel_Z,Gyro_X,Gyro_Y,Gyro_Z,IMU_Temp,Timestamp
2026-01-21 14:23:45.123,26.18,943.05,0.0500,-0.1200,9.8100,0.0100,-0.0200,0.0000,25.50,12345
2026-01-21 14:23:46.125,26.20,943.10,0.0400,-0.1100,9.8000,0.0000,-0.0100,0.0100,25.50,12346
```

### 4. Log to File
```bash
python3 udp_receiver_advanced.py -l telemetry_log.csv
```
Saves all data to `telemetry_log.csv` while displaying on screen.

### 5. Compact Mode with Logging
```bash
python3 udp_receiver_advanced.py -m compact -l data.csv
```

### 6. Show Statistics
```bash
python3 udp_receiver_advanced.py -s
```
Press Ctrl+C to see stats:
```
======================================================================
Statistics
======================================================================
Packets received: 127
Duration: 127.3 seconds
Rate: 1.00 packets/sec

BMP280 Temperature: 25.98 - 26.21 °C
BMP280 Pressure:    942.77 - 943.17 Pa
======================================================================
```

### 7. Different Port
```bash
python3 udp_receiver_advanced.py -p 6666
```

---

## Command Line Options (Advanced)

```
usage: udp_receiver_advanced.py [-h] [-m {detailed,compact,csv}] 
                                 [-l LOG] [-p PORT] [-s]

options:
  -h, --help            Show help message
  -m, --mode            Display mode: detailed, compact, or csv
  -l, --log LOG         Log to CSV file
  -p, --port PORT       UDP port (default: 5555)
  -s, --stats           Show statistics on exit
```

---

## Example Usage Scenarios

### Monitor in Real-Time
```bash
python3 udp_telemetry_receiver.py
```

### Log Flight Data
```bash
python3 udp_receiver_advanced.py -m compact -l flight_$(date +%Y%m%d_%H%M%S).csv -s
```

### Quick Check
```bash
python3 udp_receiver_advanced.py -m compact
```

### Data Collection for Analysis
```bash
python3 udp_receiver_advanced.py -m csv > data.csv
```

### Stream to Another Program
```bash
python3 udp_receiver_advanced.py -m csv | ./your_analyzer.py
```

---

## Data Format

### Packet Structure (52 bytes)
```
Bytes 0-11:   cFS Header (skip)
Bytes 12-15:  BMP Temperature (float, °C)
Bytes 16-19:  BMP Pressure (float, Pa)
Bytes 20-23:  Accel X (float)
Bytes 24-27:  Accel Y (float)
Bytes 28-31:  Accel Z (float)
Bytes 32-35:  Gyro X (float)
Bytes 36-39:  Gyro Y (float)
Bytes 40-43:  Gyro Z (float)
Bytes 44-47:  IMU Temperature (float, °C)
Bytes 48-51:  Timestamp (uint32, seconds)
```

All floats are **big-endian** (network byte order).

---

## Troubleshooting

### No Data Received
```bash
# Check if cFS is running and transmitting
nc -ul 5555 | hexdump -C

# Check firewall
sudo ufw allow 5555/udp

# Check if port is in use
netstat -an | grep 5555
```

### Wrong Data
```bash
# Verify packet size
nc -ul 5555 | wc -c
# Should receive ~52-64 bytes per packet
```

### Permission Issues
```bash
# If port < 1024, need sudo
sudo python3 udp_receiver_advanced.py -p 514
```

---

## Tips

1. **Use compact mode** for long-running monitoring
2. **Use CSV mode** for data analysis in Excel/Python
3. **Always use -l** to log data for later analysis
4. **Use -s** to see packet rate and data ranges
5. **Pipe to file** to save for later: `python3 ... > data.txt`

---

## Import CSV in Python

```python
import pandas as pd

# Read logged CSV
df = pd.read_csv('telemetry_log.csv')

# Plot BMP280 temperature
df['BMP_Temp'].plot()

# Calculate statistics
print(df.describe())
```

---

Ready to receive telemetry! 🚀

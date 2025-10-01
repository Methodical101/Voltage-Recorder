# ESP32 High-Fidelity Voltage Recorder

This project turns a ESP32 into a high-precision voltage recorder that captures analog voltages from an external source (0-3.3V range), stores them in memory, and can replay them using the built-in DAC. Perfect for capturing and reproducing sensor signals, waveforms, or any time-varying voltage patterns.

## Features

- **High-Precision ADC**: 12-bit resolution with 64x oversampling for improved accuracy
- **Calibrated Readings**: Automatic ADC calibration using ESP32 eFuse values
- **Memory-Only Storage**: Stores up to 5000 samples (~20KB) in RAM
- **Configurable Sample Rate**: 1-10000 Hz (recommended ≤200 Hz for stable timing)
- **Voltage Replay**: Outputs recorded voltages via 8-bit DAC
- **Serial Control**: Full control via simple text commands over USB serial
- **Real-time Monitoring**: Live voltage readings and recording progress
- **Visual Feedback**: Built-in LED indicates system status

## Hardware Requirements

### Components
- **ESP32 Development Board** (e.g., ESP32 DevKit v1, ESP32-WROOM-32)
- **USB Cable** (for programming and serial communication)
- **Voltage Source** (0-3.3V max - **DO NOT EXCEED 3.3V!**)
- Optional: Resistor divider if measuring higher voltages

### Pin Connections

| Function | GPIO Pin | Description |
|----------|----------|-------------|
| **Voltage Input** | GPIO36 (ADC1_CH0) | Connect your voltage source here (0-3.3V max!) |
| **Voltage Output** | GPIO25 (DAC1) | Outputs recorded voltages during replay |
| **Status LED** | GPIO2 | Built-in LED shows system status |

⚠️ **IMPORTANT**: The ESP32 ADC input pins can only handle **0-3.3V**. Exceeding this voltage will permanently damage your board!

## Technical Specifications

- **ADC Resolution**: 12-bit (0-4095 counts)
- **ADC Range**: 0-3.3V with 11dB attenuation
- **Oversampling**: 64 samples averaged per reading
- **DAC Resolution**: 8-bit (0-255 counts)
- **DAC Range**: 0-3.3V
- **Maximum Samples**: 5000 samples
- **Memory Usage**: ~20KB for full buffer
- **Sample Rate Range**: 1-10000 Hz
- **Default Sample Rate**: 100 Hz
- **Serial Baud Rate**: 115200

## Setup Instructions

### 1. Install PlatformIO
Install [PlatformIO](https://platformio.org/) for your preferred IDE (VS Code recommended).

### 2. Clone and Open Project
```bash
git clone https://github.com/Methodical101/Voltage-Recorder.git
cd Voltage-Recorder
```

### 3. Connect ESP32
Connect your ESP32 board to your computer via USB.

### 4. Upload Code
```bash
pio run --target upload
```

### 5. Open Serial Monitor
```bash
pio device monitor
```
Or use the PlatformIO serial monitor at 115200 baud.

## Usage Guide

### Starting the System

When you first connect, you'll see:
```
=== ESP32 Simple Voltage Recorder ===
Memory-only version - no SD card needed!
Initializing...
ADC: Using eFuse Vref
DAC initialized for voltage replication
Setup complete!
```

### Available Commands

All commands are case-insensitive. Type them in the serial monitor and press Enter.

| Command | Description | Example |
|---------|-------------|---------|
| `start` or `begin` | Start recording voltages | `start` |
| `stop` | Stop recording | `stop` |
| `show` or `print` | Display recorded data | `show` |
| `replay` or `replicate` | Replay voltages on DAC output | `replay` |
| `status` | Show system status and statistics | `status` |
| `read` | Read current voltage once | `read` |
| `clear` | Clear sample buffer | `clear` |
| `rate <Hz>` | Set sample rate (1-10000 Hz) | `rate 200` |
| `help` | Show command list | `help` |

### Basic Workflow

1. **Check Current Voltage**:
   ```
   > read
   Current voltage: 1.2345 V
   ```

2. **Set Sample Rate** (optional):
   ```
   > rate 100
   Sample rate set to 100 Hz
   ```

3. **Start Recording**:
   ```
   > start
   Started recording at 100 Hz...
   Type 'stop' to end recording.
   Recorded 100 samples...
   Recorded 200 samples...
   ```

4. **Stop Recording**:
   ```
   > stop
   Recording stopped. Captured 250 samples.
   Actual recording duration: 2.50 seconds
   Type 'show' to view data or 'replay' to replicate voltages.
   ```

5. **View System Status**:
   ```
   > status
   === System Status ===
   Recording: NO
   Samples in buffer: 250/5000
   Sample rate: 100 Hz
   Memory usage: 1.0 KB
   Current voltage: 1.2345 V
   Recorded range: 0.9876 - 1.5432 V (avg: 1.2345 V)
   Actual recording duration: 2.50 seconds
   ```

6. **View Recorded Data**:
   ```
   > show
   Printing 250 recorded samples:
   Sample#,Voltage(V),Time(ms)
   ------------------------
   0,1.2345,0.0
   1,1.2346,10.0
   2,1.2347,20.0
   ...
   ```

7. **Replay Voltages**:
   ```
   > replay
   Replaying 250 voltage samples...
   Note: ESP32 DAC has limited precision (8-bit, 0-3.3V range)
   Type any key to stop replay.
   
   Sample 0: 1.2345V -> DAC 95
   Sample 100: 1.3456V -> DAC 104
   Sample 200: 1.4567V -> DAC 113
   Replay completed.
   Expected duration: 2.500 s, Replay duration: 2.501 s
   ```

## LED Status Indicators

- **Solid ON**: System ready / Recording stopped
- **Blinking**: Recording in progress
- **OFF**: System initializing

## Performance Notes

### Sample Rate Recommendations

- **≤200 Hz**: Recommended for reliable timing and accurate replay
- **200-1000 Hz**: May work but timing accuracy decreases
- **>1000 Hz**: Not recommended - significant timing errors likely

The system will warn you if you set a sample rate above 200 Hz:
```
WARNING: Sample rate is above safe value (200 Hz). Recording and replay timing may be inaccurate!
```

### Memory Limitations

- Maximum 5000 samples (~20KB)
- At 100 Hz: 50 seconds of recording
- At 200 Hz: 25 seconds of recording
- At 1000 Hz: 5 seconds of recording

When the buffer is full, recording stops automatically:
```
Buffer full! Stopping recording.
```

### ADC/DAC Precision

- **ADC**: 12-bit input (4096 levels) with 64x oversampling provides ~0.8mV resolution
- **DAC**: 8-bit output (256 levels) provides ~13mV resolution
- Replay precision is limited by 8-bit DAC resolution

## Safety Warnings

⚠️ **CRITICAL SAFETY INFORMATION**

1. **Never exceed 3.3V on GPIO36** - This will damage your ESP32!
2. **Use a resistor divider** if measuring voltages above 3.3V
3. **No reverse voltage protection** - Negative voltages will damage the board
4. **No overcurrent protection** - Keep source impedance reasonably high

### Measuring Higher Voltages

To measure voltages above 3.3V, use a voltage divider:

For 5V signals:
```
Vin ---[10kΩ]---+---[20kΩ]--- GND
                |
              GPIO36
```
This divides by 3, so 5V → 1.67V (safe for ESP32)

For 12V signals:
```
Vin ---[33kΩ]---+---[10kΩ]--- GND
                |
              GPIO36
```
This divides by 4.3, so 12V → 2.79V (safe for ESP32)

Remember to scale your readings in software if using a divider!

## Troubleshooting

### Problem: "Buffer full!" message
**Solution**: Increase sample rate or clear buffer more frequently

### Problem: "Timing may be inaccurate" warning
**Solution**: Lower sample rate to ≤200 Hz

### Problem: Replay duration doesn't match recording
**Solution**: Lower sample rate - the ESP32 can't maintain timing at high rates

### Problem: No voltage readings / always 0V
**Solution**: 
- Check connections to GPIO36
- Verify voltage source is within 0-3.3V
- Ensure common ground between ESP32 and voltage source

### Problem: Replay voltages don't match recorded values
**Solution**: This is expected - DAC has lower resolution (8-bit) than ADC (12-bit)

## Example Applications

- Sensor data logging (temperature, light, pressure transducers)
- Waveform capture and analysis
- Signal characterization
- Testing DAC linearity
- Audio envelope recording (low frequency)
- Battery discharge profiling
- Solar panel output monitoring

## Code Structure

```
Voltage-Recorder/
├── src/
│   └── main.cpp          # Main application code
├── platformio.ini        # PlatformIO configuration
└── README.md            # This file
```

### Key Functions

- `setupADC()` - Configures 12-bit ADC with calibration
- `setupDAC()` - Initializes 8-bit DAC output
- `readVoltageHighPrecision()` - Takes 64 averaged samples
- `startRecording()` - Begins voltage capture
- `stopRecording()` - Ends voltage capture
- `replayVoltages()` - Outputs recorded data via DAC
- `printData()` - Displays recorded samples
- `printStatus()` - Shows system information

## Contributing

Contributions are welcome! Please feel free to submit issues or pull requests.

## License

This project is open source. Please check the repository for license details.

// =============================
// ESP32 High-Fidelity Voltage Recorder
// =============================
// This sketch lets your ESP32 record voltages from an external source (0-3.3V)
// with high precision, store them in memory, and replay them using the DAC output.
// You control everything via serial commands.
//
// Author: Christian Foster
// Date: September 2025
// =============================

#include <Arduino.h>
#include <driver/adc.h>      // ESP32 ADC driver for analog input
#include <esp_adc_cal.h>     // ESP32 ADC calibration for accurate readings
#include <driver/dac.h>      // ESP32 DAC driver for analog output

// =============================
// Pin Definitions
// =============================
#define ADC_PIN 36          // GPIO36 (ADC1_CH0) - Connect your voltage source here (0-3.3V max!)
#define DAC_PIN 25          // GPIO25 (DAC1) - Outputs recorded voltages for replay
#define LED_PIN 2           // Built-in LED for status indication

// =============================
// ADC (Analog to Digital Converter) Configuration
// =============================
#define ADC_VREF 1000       // Reference voltage in mV (used for calibration)

// =============================
// Recording Settings
// =============================
#define MAX_SAMPLES 5000    // Maximum number of samples to store in memory (about 20KB)
#define DEFAULT_SAMPLE_RATE 100 // Default sample rate in Hz (samples per second)

// =============================
// Global Variables
// =============================
bool recording = false;                 // True if currently recording
float voltageBuffer[MAX_SAMPLES];       // Buffer to store recorded voltages
int sampleCount = 0;                    // Number of samples recorded
int sampleRate = DEFAULT_SAMPLE_RATE;   // Current sample rate in Hz
unsigned long lastSampleTime = 0;       // Timestamp of last sample
esp_adc_cal_characteristics_t adc_chars;// ADC calibration characteristics
unsigned long recordingStartTime = 0; // Time when recording started (ms)
unsigned long recordingEndTime = 0;   // Time when recording ended (ms)
float adcOffset = 0.0; // ADC offset (in volts) measured during calibration
int ADC_SAMPLES = 64;      // Oversampling: number of samples per reading for better precision (now variable)

// =============================
// Function Declarations
// =============================
void setupADC();
void setupDAC();
float readVoltageHighPrecision();
void processSerialCommands();
void startRecording();
void stopRecording();
void replayVoltages();
void printStatus();
void printHelp();
void printData();
void calibrateADCOffset();

// =============================
// Arduino Setup Function
// =============================
void setup() {
    Serial.begin(115200);   // Start serial communication at 115200 baud
    delay(1000);            // Wait for serial to initialize
    pinMode(LED_PIN, OUTPUT);           // Set LED pin as output
    digitalWrite(LED_PIN, LOW);         // Turn off LED initially
    Serial.println("=== ESP32 Simple Voltage Recorder ===");
    Serial.printf("Version: %s\n", VERSION);
    Serial.println("Initializing...");
    setupADC();    // Set up ADC for voltage readings
    setupDAC();    // Set up DAC for voltage replay
    Serial.println("Auto-calibrating ADC offset.");
    delay(1000); // Wait 1 second for user to connect pin to GND
    calibrateADCOffset();
    Serial.println("Setup complete!");
    printHelp();   // Show available commands
    digitalWrite(LED_PIN, HIGH); // Turn on LED to indicate ready
}

// =============================
// Arduino Main Loop
// =============================
void loop() {
    processSerialCommands(); // Check for serial commands from user
    // If recording is active, take samples at the specified rate
    if (recording && sampleCount < MAX_SAMPLES) {
        unsigned long currentTime = millis();
        if (sampleCount == 0) recordingStartTime = currentTime; // Mark start time
        // Check if it's time for the next sample
        if (currentTime - lastSampleTime >= (1000 / sampleRate)) {
            voltageBuffer[sampleCount] = readVoltageHighPrecision(); // Store voltage in buffer
            sampleCount++;
            lastSampleTime = currentTime;
            // Blink LED during recording (visual feedback)
            digitalWrite(LED_PIN, (sampleCount % 100 < 50) ? HIGH : LOW);
            // Print progress every 100 samples
            if (sampleCount % 100 == 0) {
                Serial.printf("Recorded %d samples...\n", sampleCount);
            }
            // If buffer is full, stop recording automatically
            if (sampleCount >= MAX_SAMPLES) {
                Serial.println("Buffer full! Stopping recording.");
                stopRecording();
            }
        }
    }
    delay(1); // Small delay to avoid busy loop
}

// =============================
// ADC Setup
// =============================
void setupADC() {
    // Configure ADC1 for 12-bit resolution (0-4095)
    adc1_config_width(ADC_WIDTH_BIT_12);
    // Set attenuation to 11dB for full 0-3.3V range
    adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11);
    
    // Calibrate ADC for accurate readings
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, ADC_VREF, &adc_chars);
    
    if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
        Serial.println("ADC: Using eFuse Vref");
    } else if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
        Serial.println("ADC: Using eFuse Two Point");
    } else {
        Serial.println("ADC: Using Default Vref");
    }
}

// =============================
// DAC Setup
// =============================
void setupDAC() {
    // Enable DAC output on GPIO25
    dac_output_enable(DAC_CHANNEL_1);
    // Set DAC output to 0V initially
    dac_output_voltage(DAC_CHANNEL_1, 0);
    Serial.println("DAC initialized for voltage replication");
}

// =============================
// High-Precision Voltage Reading
// =============================
float readVoltageHighPrecision() {
    uint32_t total = 0;
    // Take multiple samples and average them for better accuracy
    for (int i = 0; i < ADC_SAMPLES; i++) {
        total += adc1_get_raw(ADC1_CHANNEL_0); // Read raw ADC value
        delayMicroseconds(10); // Small delay between samples
    }
    uint32_t average = total / ADC_SAMPLES;
    // Convert raw ADC value to millivolts using calibration
    uint32_t voltage_mv = esp_adc_cal_raw_to_voltage(average, &adc_chars);
    float voltage = voltage_mv / 1000.0; // Convert to volts
    // Subtract offset
    voltage -= adcOffset;
    if (voltage < 0) voltage = 0.0; // Clamp to zero
    return voltage;
}

// Calibrate ADC offset (call with pin grounded)
void calibrateADCOffset() {
    Serial.println("Make sure the ADC pin is connected to GND during calibration.");
    uint32_t total = 0;
    for (int i = 0; i < ADC_SAMPLES; i++) {
        total += adc1_get_raw(ADC1_CHANNEL_0);
        delayMicroseconds(10);
    }
    uint32_t average = total / ADC_SAMPLES;
    uint32_t voltage_mv = esp_adc_cal_raw_to_voltage(average, &adc_chars);
    adcOffset = voltage_mv / 1000.0;
    Serial.printf("ADC offset calibrated: %.4f V\n", adcOffset);
}

// =============================
// Serial Command Processing
// =============================
void processSerialCommands() {
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n'); // Read command from serial
        command.trim();
        command.toLowerCase();
        // Check which command was entered
        if (command.startsWith("start") || command.startsWith("begin")) {
            startRecording();
        }
        else if (command.startsWith("stop")) {
            stopRecording();
        }
        else if (command.startsWith("show") || command.startsWith("print")) {
            printData();
        }
        else if (command.startsWith("replay") || command.startsWith("replicate")) {
            replayVoltages();
        }
        else if (command.startsWith("status")) {
            printStatus();
        }
        else if (command.startsWith("clear")) {
            sampleCount = 0;
            Serial.println("Buffer cleared.");
        }
        else if (command.startsWith("rate")) {
            int newRate = command.substring(5).toInt();
            if (newRate > 0 && newRate <= 10000) {
                sampleRate = newRate;
                Serial.printf("Sample rate set to %d Hz\n", sampleRate);
                if (sampleRate > 200) {
                    Serial.println("WARNING: Sample rate is above safe value (200 Hz). Recording and replay timing may be inaccurate!");
                }
            } else {
                Serial.println("Invalid sample rate (1-10000 Hz)");
            }
        }
        else if (command.startsWith("samples")) {
            int newSamples = command.substring(7).toInt();
            if (newSamples >= 1 && newSamples <= 1024) {
                ADC_SAMPLES = newSamples;
                Serial.printf("ADC samples per reading set to %d\n", ADC_SAMPLES);
            } else {
                Serial.println("Invalid ADC samples (1-1024)");
            }
        }
        else if (command.startsWith("read")) {
            float voltage = readVoltageHighPrecision();
            Serial.printf("Current voltage: %.4f V\n", voltage);
        }
        else if (command.startsWith("calibrate")) {
            calibrateADCOffset();
        }
        else if (command.startsWith("help")) {
            printHelp();
        }
        else {
            Serial.println("Unknown command. Type 'help' for available commands.");
        }
    }
}

// =============================
// Start Recording
// =============================
void startRecording() {
    if (recording) {
        Serial.println("Already recording!");
        return;
    }
    sampleCount = 0;           // Reset buffer
    recording = true;          // Set flag
    lastSampleTime = millis(); // Start timing
    recordingStartTime = lastSampleTime; // Store start time
    Serial.printf("Started recording at %d Hz...\n", sampleRate);
    Serial.println("Type 'stop' to end recording.");
}

// =============================
// Stop Recording
// =============================
void stopRecording() {
    if (!recording) {
        Serial.println("Not currently recording.");
        return;
    }
    recording = false;         // Clear flag
    digitalWrite(LED_PIN, HIGH); // Turn LED on
    recordingEndTime = millis(); // Store end time
    Serial.printf("Recording stopped. Captured %d samples.\n", sampleCount);
    Serial.printf("Actual recording duration: %.2f seconds\n", (recordingEndTime - recordingStartTime) / 1000.0);
    Serial.println("Type 'show' to view data or 'replay' to replicate voltages.");
}

// =============================
// Print Recorded Data
// =============================
void printData() {
    if (sampleCount == 0) {
        Serial.println("No data recorded!");
        return;
    }
    Serial.printf("Printing %d recorded samples:\n", sampleCount);
    Serial.println("Sample#,Voltage(V),Time(ms)");
    Serial.println("------------------------");
    for (int i = 0; i < sampleCount; i++) {
        float timeMs = (float)i * 1000.0 / sampleRate;
        Serial.printf("%d,%.4f,%.1f\n", i, voltageBuffer[i], timeMs);
        // Pause every 20 lines to prevent overwhelming the serial output
        if ((i + 1) % 20 == 0 && i < sampleCount - 1) {
            Serial.println("--- Press any key to continue ---");
            while (!Serial.available()) delay(10);
            while (Serial.available()) Serial.read(); // Clear buffer
        }
    }
    Serial.println("------------------------");
    Serial.printf("Total: %d samples\n", sampleCount);
}

// =============================
// Replay Recorded Voltages on DAC
// =============================
void replayVoltages() {
    if (sampleCount == 0) {
        Serial.println("No data to replay!");
        return;
    }
    Serial.printf("Replaying %d voltage samples...\n", sampleCount);
    Serial.println("Note: ESP32 DAC has limited precision (8-bit, 0-3.3V range)");
    Serial.println("Type any key to stop replay.\n");
    unsigned long replayStart = millis(); // Start timing
    unsigned long sampleInterval_us = 1000000UL / sampleRate;
    unsigned long nextSampleTime = micros();
    float lastPrintedVoltage = -1.0; // Track last printed voltage for change detection
    for (int i = 0; i < sampleCount && !Serial.available(); i++) {
        float voltage = voltageBuffer[i];
        // Convert voltage to DAC value (0-255 for 0-3.3V)
        // Clamp to 3.3V max
        if (voltage > 3.3) voltage = 3.3;
        if (voltage < 0) voltage = 0;
        uint8_t dacValue = (uint8_t)(voltage * 255.0 / 3.3);
        dac_output_voltage(DAC_CHANNEL_1, dacValue); // Output voltage on DAC
        // Print every 50th sample OR when voltage changes significantly (>0.1V)
        if (i % 50 == 0 || fabs(voltage - lastPrintedVoltage) > 0.1) {
            Serial.printf("Sample %d: %.4fV -> DAC %d\n", i, voltage, dacValue);
            lastPrintedVoltage = voltage;
        }
        // Precise timing using micros()
        nextSampleTime += sampleInterval_us;
        long wait = nextSampleTime - micros();
        if (wait > 0) {
            delayMicroseconds(wait);
        }
    }
    unsigned long replayEnd = millis();
    // Clear any pending serial input
    while (Serial.available()) Serial.read();
    // Reset DAC to 0V
    dac_output_voltage(DAC_CHANNEL_1, 0);
    Serial.println("Replay completed.");
    float replaySec = (replayEnd - replayStart) / 1000.0;
    float expectedSec = (recordingEndTime > recordingStartTime) ? ((recordingEndTime - recordingStartTime) / 1000.0) : ((float)sampleCount / sampleRate);
    Serial.printf("Expected duration: %.3f s, Replay duration: %.3f s\n", expectedSec, replaySec);
    if (sampleRate > 200) {
        Serial.println("WARNING: Replay rate is above safe value (200 Hz). Timing may be inaccurate!");
    }
    if (fabs(replaySec - expectedSec) > 0.2 * expectedSec) {
        Serial.println("ERROR: Exceeded stable sample rate! Replay duration does not match expected duration. Lower your sample rate for reliable timing.");
    }
}

// =============================
// Print System Status
// =============================
void printStatus() {
    Serial.println("=== System Status ===");
    Serial.printf("Recording: %s\n", recording ? "YES" : "NO");
    Serial.printf("Samples in buffer: %d/%d\n", sampleCount, MAX_SAMPLES);
    Serial.printf("Sample rate: %d Hz\n", sampleRate);
    if (sampleRate > 200) {
        Serial.println("WARNING: Sample rate is above safe value (200 Hz). Recording and replay timing may be inaccurate!");
    }
    Serial.printf("Memory usage: %.1f KB\n", (float)(sampleCount * sizeof(float)) / 1024.0);
    float currentVoltage = readVoltageHighPrecision();
    Serial.printf("Current voltage: %.4f V\n", currentVoltage);
    if (sampleCount > 0) {
        float minV = voltageBuffer[0], maxV = voltageBuffer[0];
        float avgV = 0;
        for (int i = 0; i < sampleCount; i++) {
            if (voltageBuffer[i] < minV) minV = voltageBuffer[i];
            if (voltageBuffer[i] > maxV) maxV = voltageBuffer[i];
            avgV += voltageBuffer[i];
        }
        avgV /= sampleCount;
        Serial.printf("Recorded range: %.4f - %.4f V (avg: %.4f V)\n", minV, maxV, avgV);
        Serial.printf("Actual recording duration: %.2f seconds\n", (recordingEndTime > recordingStartTime) ? ((recordingEndTime - recordingStartTime) / 1000.0) : 0.0);
    }
}

// =============================
// Print Help / Commands
// =============================
void printHelp() {
    Serial.println("\n=== Available Commands ===");
    Serial.println("start/begin   - Start voltage recording");
    Serial.println("stop          - Stop recording");
    Serial.println("show/print    - Display recorded data");
    Serial.println("replay        - Replicate recorded voltages on DAC pin");
    Serial.println("status        - Show system status");
    Serial.println("read          - Read current voltage");
    Serial.println("clear         - Clear sample buffer");
    Serial.println("calibrate     - Calibrate ADC offset (run with pin grounded)");
    Serial.println("rate <Hz>     - Set sample rate (1-10000 Hz)");
    Serial.println("samples <N>   - Set ADC samples per reading (1-1024)");
    Serial.println("help          - Show this help");
    Serial.println("\nConnections:");
    Serial.printf("Voltage input: GPIO%d (0-3.3V max!)\n", ADC_PIN);
    Serial.printf("Voltage output: GPIO%d (DAC)\n", DAC_PIN);
    Serial.printf("\nMax samples: %d (%.1f KB memory)\n", MAX_SAMPLES, (float)(MAX_SAMPLES * sizeof(float)) / 1024.0);
}
// =============================
// END OF FILE
// =============================
# Release Notes

## Version: v1.1.2

- Added baseline variables for sample rate (`BASELINE_SAMPLE_RATE`, default 100 Hz) and ADC samples (`BASELINEAdcSamples`, default 32).
- Renamed `ADC_SAMPLES` to `adcSamples` (camelCase) throughout the code for consistency.
- All error and warning checks now use baseline variables instead of hardcoded values.
- Dynamic warning messages: Now indicate whether high sample rate, high ADC samples, or both are likely causing timing issues in recording and replay.
- Improved maintainability and clarity of configuration and diagnostics.

---

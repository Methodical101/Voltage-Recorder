## What's New

### Added
- `calibrate` command: Calibrates and corrects ADC offset for accurate voltage readings. Run with ADC pin grounded.
- Automatic ADC offset calibration on startup, with user prompt and delay.
- `samples <N>` command: Set the number of ADC samples averaged per reading (1–1024) for precision control.

### Fixed
- All voltage readings now subtract the measured offset for improved accuracy.
- Improved validation and warnings for unsafe sample rates in the `rate <Hz>` command.

### Improved
- Help text updated to include new commands.
- Enhanced user guidance and error messages for serial commands.

---

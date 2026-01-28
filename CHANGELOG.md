# Changelog

All notable changes to SpoolStation will be documented in this file.

## [0.1.1b2] - unreleased

### Added
- Periodic auto-connect retry for printers - printers with auto-connect enabled will now automatically reconnect every 30 seconds if disconnected
- Unit tests for auto-connect functionality (5 new tests)
- ESLint configuration for CI
- Complete frontend/backend test suite (173 frontend, 200+ backend tests)
- API browser and key management
- Support page in frontend
- Filament color database

### Changed
- Auto-connect now runs as a periodic background task instead of one-shot at startup
- Improved Dashboard Current Spool card stability

### Fixed
- Dashboard Current Spool card now shows "Device Offline" when SpoolBuddy display is disconnected
- Fixed core_weight calculation mismatch between frontend and backend
- Fixed sync weight button not updating card values (race condition)
- Fixed weight bouncing between values when NFC tag detection flickers
- Fixed active AMS slot display
- Fixed manage link in top debug notification bar
- Fixed PA Profile badge count

## [0.1.1b1] - 2026-01-24

### Added
- Color-cycling spool animation for empty state
- Sync button for scale mismatches
- K profile indicator, tag ID, and scale comparison on Current Spool card
- Firmware simulator screens and functions

### Fixed
- Weight calculations using Default Core Weight consistently
- Test infrastructure fixes

## [0.1.0] - 2026-01-20

### Added
- Initial release
- NFC-based spool identification
- Weight scale integration for filament tracking
- Inventory management and spool catalog
- MQTT-based Bambu Lab printer connectivity
- AMS visualization with slot configuration
- Cloud integration for filament presets
- WebSocket-based real-time updates

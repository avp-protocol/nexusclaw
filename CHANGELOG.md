# Changelog

All notable changes to NexusClaw will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- AVP Protocol v0.1.0 implementation
  - DISCOVER operation for capability query
  - AUTHENTICATE with PIN-based authentication
  - STORE/RETRIEVE/DELETE/LIST/ROTATE for secret management
  - HW_CHALLENGE for device attestation
  - HW_SIGN for hardware-based cryptographic signing
  - HW_ATTEST for signed attestation
- NexusClaw branding and product announcement
- Logo and visual assets

### Changed
- Forked from [tropicsquare/tropic01-stm32u5-usb-devkit-fw](https://github.com/tropicsquare/tropic01-stm32u5-usb-devkit-fw)
- Updated README for NexusClaw product positioning

## [1.0.0] - Original Firmware

### Added
- Initial firmware from Tropic Square USB devkit
- USB CDC serial interface
- SPI-to-TROPIC01 bridge functionality
- Basic command set (HELP, AUTO, CS, PWR, etc.)

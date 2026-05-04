# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Changed
- Transformed project from satellite-specific system to general-purpose embedded EMS framework
- Updated all documentation to use generic terminology instead of domain-specific terms
- Improved README.md with clearer project positioning

## [1.0.0] - 2024-12-XX

### Added
- Complete 5-layer architecture (OSAL, HAL, PCL, PDL, Apps)
- OSAL layer with comprehensive OS abstraction
  - Task management with graceful shutdown (5s timeout)
  - Queue, mutex, semaphore, and event primitives
  - Logging system with rotation support
  - File I/O, network, and time abstractions
- HAL layer with hardware drivers
  - CAN bus driver (SocketCAN)
  - Serial port driver (termios)
  - I2C and SPI bus drivers
  - Concurrent access protection with mutexes
- PDL layer for peripheral management
  - Satellite platform interface service
  - BMC device service (IPMI/Redfish)
  - MCU peripheral service
- PCL layer for device-tree-like configuration
- Unified test framework with 142+ test cases
- Multi-architecture support (x86_64, ARM32, ARM64, RISC-V 64)
- CMake-based build system with presets
- Cross-compilation support

### Security
- Fixed heap allocation integer overflow vulnerability
- Fixed CAN frame DLC boundary check
- Fixed task wrapper race condition
- Added HAL layer concurrent access protection
- Implemented hardware error recovery mechanism
- Added OSAL resource leak detection

### Documentation
- Comprehensive architecture documentation
- API reference for all layers
- Usage guides and examples
- Coding standards (MISRA C compliant)
- Build system documentation
- Cross-compilation guide

[Unreleased]: https://github.com/wanguo99/EMS/compare/v1.0.0...HEAD
[1.0.0]: https://github.com/wanguo99/EMS/releases/tag/v1.0.0

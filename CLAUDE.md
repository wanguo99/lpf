# LPF Project Context

LPF (Linux Peripheral Framework) is now a Linux-focused peripheral framework. Product-specific satellite/PMC business code has been removed.

## Current Architecture

Core modules:

- OSAL
- LPF HW
- LPF Core
- LPF Runtime
- LPF Runtime Config
- PDI
- ACONFIG

Current concrete peripheral/device family:

- MCU
- LED

The framework is still structured for later peripheral expansion. Add new peripheral families by extending LPF runtime config types/accessors, LPF peripheral services/protocol helpers, PDI userspace APIs, Kconfig entries, and tests.

## Common Commands

```bash
make list
make ubuntu_x86_modules_defconfig
make modules
```

## Layering Rules

- Core modules must not depend on product code.
- Product/application code belongs outside shared framework module directories.
- LPF runtime hosts runtime config loading and kernel-side peripheral services.
- PDI exposes the userspace API and wraps LPF UAPI ioctl ABIs.
- LPF HW owns framework hardware access above the SoC adapter; OSAL remains
  the operating-system abstraction layer.

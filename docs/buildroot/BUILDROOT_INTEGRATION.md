# Buildroot Integration

This directory contains a Buildroot package skeleton for building
ES-Middleware as a generic embedded middleware framework with userspace
libraries and kernel modules.

## Defconfig

Use one of the framework defconfigs, for example:

```text
kernel_x86_modules_defconfig
```

## Install Modes

- Target install: userspace libraries are installed through CMake and enabled
  kernel modules are installed under `/lib/modules/<kernel>/extra/es-middleware/`.
- Staging install: development headers and userspace libraries are installed
  for packages that depend on ES-Middleware.

No product-specific init script is installed by default. Product services should be owned by the product layer that adds them.

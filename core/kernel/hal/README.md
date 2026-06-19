# HAL

HAL is the kernel-mode Hardware Abstraction Layer used by PDM.

## Scope

- Builds as `hal.ko` through `make modules`.
- Exports kernel symbols for the public HAL C API under
  `core/kernel/include/hal/`.
- Has no userspace HAL library or userspace HAL test product.

Current kernel implementations:

- CAN uses Linux kernel SocketCAN APIs.
- Serial uses Linux kernel TTY APIs plus kernel file I/O for `/dev/tty*`
  devices.
- GPIO uses Linux kernel GPIO and IRQ APIs.
- I2C uses Linux kernel I2C adapter transfer APIs.
- SPI uses Linux kernel SPI device and transfer APIs.

## Configuration

```text
CONFIG_OSAL=y
CONFIG_HAL=y
CONFIG_HAL_CAN=y
CONFIG_HAL_UART=y
CONFIG_HAL_GPIO=y
CONFIG_HAL_I2C=y
CONFIG_HAL_SPI=y
```

## Build

```bash
make kernel_x86_modules_defconfig
make modules
```

Expected module artifacts include:

```text
_build/modules/osal.ko
_build/modules/hal.ko
_build/modules/pdm.ko
```

## Layout

```text
core/kernel/hal/
├── Config.in
├── Makefile
├── CMakeLists.txt      # header-only interface for host-side consumers
└── src/
    ├── hal.c
    ├── hal_can.c
    ├── hal_serial.c
    ├── hal_gpio.c
    ├── hal_i2c.c
    └── hal_spi.c
```

## Layering

PDM includes HAL headers and calls exported HAL symbols. HAL owns hardware
access details and must use Linux kernel APIs or the small kernel OSAL wrapper.

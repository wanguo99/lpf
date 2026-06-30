# PDM Architecture

PDM is a Linux-focused peripheral access framework. It keeps kernel drivers,
userspace ABI headers, and userspace SDK wrappers separate while using normal
Linux driver-model mechanisms underneath.

## Runtime Shape

Current kernel modules:

- `osal.ko`: kernel OSAL helpers used by PDM kernel code.
- `pdm.ko`: the PDM bus, device model, control node, peripheral drivers,
  and built-in backend registry.

`pdm.ko` registers a Linux `bus_type` named `pdm`. PDM devices appear under
`/sys/bus/pdm` and expose userspace instance nodes under `/dev/pdm/` when their
bound driver creates one. `/dev/pdm_ctl` exposes synchronous discovery snapshots
for userspace.

PDM devices can be created from:

- child nodes of a `vendor,pdm-bus` Device Tree controller,
- MCU nodes below native UART, I2C, and SPI controllers,
- optional synthetic mock devices enabled by `CONFIG_PDM_MOCK_DEVICES`.

## Kernel Layout

- `kernel/pdm/core/pdm_core.c`: `pdm.ko` module entry and lifecycle ordering.
- `kernel/pdm/core/bus/`: Linux `bus_type` integration and bus driver binding.
- `kernel/pdm/core/device/`: `struct pdm_device` lifecycle and Device Tree bus enumeration.
- `kernel/pdm/core/driver/`: linker-section registries for PDM drivers and backends.
- `kernel/pdm/core/chardev/`: `/dev/pdm_ctl` discovery node and per-instance `/dev/pdm/*` client nodes.
- `kernel/pdm/core/diag/`: sysfs, procfs, and debugfs helpers.
- `kernel/pdm/peripheral/`: reusable peripheral drivers such as MCU and LED.
- `kernel/pdm/mock/`: synthetic devices for x86 smoke tests.
- `kernel/include/pdm/core/bus/`: cross-module bus APIs.
- `kernel/include/pdm/core/device/`: cross-module device model APIs.
- `kernel/include/pdm/core/driver/`: backend registration APIs used by transport/control implementations.
- `kernel/include/pdm/core/chardev/`: shared character-device client APIs.
- `kernel/include/pdm/core/diag/`: optional diagnostic interface APIs.
- `uapi/pdm/`: userspace/kernel ioctl ABI headers.
- `user/pdi/`: userspace C wrappers over the PDM UAPI nodes.

## Registration Model

PDM bus drivers use `pdm_driver_register(name, init, exit)`. Entries are placed
in the `pdm_driver_entries` linker section and initialized by the `pdm.ko` module entry. Exit
callbacks run in reverse order during module unload.

Peripheral hardware or transport implementations use `pdm_backend_register()`.
Backends are keyed by PDM device type, backend class, and compatible string.
This keeps feature selection local to backend files and lets Kconfig trim I2C,
SPI, UART, GPIO, PWM, or future implementations without changing core init
code.

The PDM bus owns instance numbering. Enumerators may request a preferred
`pdm,id`, but the bus reserves that id only after a peripheral driver matches
the device. Peripheral drivers then register user-visible client nodes such as
`/dev/pdm/mcu0`; backend files provide transport or control ops and should not
register user nodes directly.

## Userspace Access

PDI should be the application-facing API. PDI opens `/dev/pdm_ctl` for discovery
and per-device nodes such as `/dev/pdm/mcu0` or `/dev/pdm/led0` for business
operations. UAPI headers under `uapi/pdm/` are the stable ABI contract between
kernel nodes and PDI.

## Adding A Peripheral

Add these pieces together when a new peripheral family needs userspace access:

1. A UAPI header under `uapi/pdm/`.
2. A PDI wrapper under `user/pdi/`.
3. A PDM driver under `kernel/pdm/peripheral/<name>/` registered through
   `pdm_driver_register()`.
4. Optional backend files registered through `pdm_backend_register()`.
5. Kconfig and Kbuild entries for each optional backend.
6. Device Tree documentation and ABI/PDI tests.

Keep product policy, init scripts, udev rules, and application business logic
outside this repository.


## Active Backend Registry

Each logical peripheral should have exactly one active backend. PDM treats Device Tree as the hardware source of truth and exports the resolved owner and transport through `/dev/pdm_manager` and PDM sysfs attributes. Applications continue to address devices by logical index or name; they should not pass CAN, UART, SPI, or I2C details.

Logical MCU numbering can be pinned with Device Tree aliases. PDM checks `pdm-mcuN` first, then falls back to `pdm,id`, `pdm,index`, or `reg` depending on the enumerator path.

```dts
/ {
    aliases {
        pdm-mcu0 = &power_mcu;
        pdm-mcu1 = &body_mcu;
    };

    pdm {
        compatible = "vendor,pdm-bus";

        body_mcu: mcu {
            compatible = "pdm,mcu-can";
            reg = <1>;
            pdm,owner = "user";
            can-controller = <&can1>;
            tx-can-id = <0x321>;
            rx-can-id = <0x322>;
        };
    };
};

&ecspi2 {
    power_mcu: mcu {
        compatible = "pdm,mcu-spi";
        reg = <0>;
        pdm,owner = "kernel";
        spi-max-frequency = <1000000>;
    };
};
```

`pdm,owner = "kernel"` is the default and keeps the current kernel backend behavior. `pdm,owner = "user"` reserves the logical MCU id and exposes the node in discovery, but PDM skips kernel backend binding so a user-space MCU daemon can own the transport. User-owned registry entries expose the transport capability only; they do not expose `PDM_MANAGER_DEVICE_CAP_USER_IOCTL` because no `/dev/pdm/mcuN` command endpoint exists for them.

Kernel-owned I2C, SPI, and serdev UART MCU nodes should remain under their native controller nodes so the native Linux bus creates the `i2c_client`, `spi_device`, or `serdev_device` used by the kernel backend. User-owned CAN/UART logical MCU nodes should be placed under `vendor,pdm-bus` and reference the native controller by phandle; do not duplicate those hardware details in a userspace config file.

The manager v2 device record adds owner, transport, OF node path, and controller path fields. Sysfs also exposes `owner`, `transport`, and `controller_path` for diagnostics.

A user-owned MCU entry is a registry contract, not a kernel character device. PDI must not silently open `/dev/pdm/mcuN` for user-owned devices; until a product supplies a user-space transport owner such as `pdm-mcud`, PDI reports the device as discovered but unsupported for command I/O. A production daemon should consume the manager metadata, claim the native CAN/UART interface, implement the shared MCU protocol, and expose a stable PDI-facing endpoint.

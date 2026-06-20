# PCONFIG

PCONFIG is the platform hardware-configuration aggregation layer. It selects a
configuration backend, validates the active platform, and exposes one typed
device list to PDM and other LPF kernel services.

## Current Responsibility

- Select the active configuration backend.
- Keep the built-in static table as the first backend implementation.
- Parse LPF Device Tree configuration when an LPF DT node is present.
- Validate platform identity and per-device configuration.
- Build a normalized enabled-device list for MCU and LED entries.
- Keep hardware configuration data separate from PDM and application logic.

## Backend Selection

PCONFIG supports a `backend` module parameter:

```text
backend=auto    # default: try dt, then static
backend=dt      # require Device Tree backend
backend=static  # require built-in static table backend
```

Explicit backend selection fails if the requested backend is not available.
`auto` is intended for product builds where DT-capable SoC kernels should use
board data from firmware while x86 or lab module builds still fall back to the
compiled static table.

## Public API

```c
const pconfig_platform_config_t *pconfig_get_board(void);
const pconfig_device_config_t *pconfig_get(void);
const pconfig_platform_config_t *pconfig_find(const char *product,
                                             const char *project,
                                             const char *version);
int32_t pconfig_list(const pconfig_platform_config_t **configs, uint32_t *count);
int32_t pconfig_validate(const pconfig_platform_config_t *config);
void pconfig_print(const pconfig_platform_config_t *config);
```

The static backend uses the table symbol from `configs/pconfig_configs.c`:

```c
extern const pconfig_platform_table_t g_pconfig_platform_table;
```

Concrete configs live under:

```text
configs/<product>/<project>/<version>/
```

The Device Tree backend looks for `/lpf`, `linux-peripheral-framework`, or
`lpf,platform-config`. The root node provides platform identity:

```dts
lpf {
    compatible = "linux-peripheral-framework";
    platform-name = "linux";
    chip-name = "am6254";
    project-name = "h200";
    product-name = "gateway";
    config-version = "1.0.0";

    mcu {
        mcu0 {
            label = "mcu0";
            interface = "can";
            device = "can0";
            bitrate = <500000>;
            rx-timeout-ms = <1000>;
            tx-timeout-ms = <1000>;
            tx-id = <0x100>;
            rx-id = <0x101>;
            cmd-timeout-ms = <1000>;
            retry-count = <3>;
        };
    };

    led {
        status {
            label = "status";
            control = "gpio";
            gpio = <42>;
            active-low;
            max-brightness = <1>;
            default-brightness = <0>;
        };
    };
};
```

MCU supports `interface = "can"` and `interface = "serial"`. LED supports
`control = "gpio"` and `control = "pwm"`.

## Typed Accessors

The current header provides inline index-based peripheral accessors:

```c
pconfig_hw_get_mcu(platform, index);
pconfig_hw_get_led(platform, index);
```

## Layering Rules

- `kernel/pconfig` owns backend selection, validation, and normalized device
  enumeration.
- `kernel/pconfig/configs` owns concrete static platform tables.
- LPF peripheral configuration consumes `pconfig_get()` and typed entries; it
  must not know concrete product table symbols or backend implementations.
- New configuration sources should be added as PCONFIG backends. They must
  produce the same `pconfig_platform_config_t` and `pconfig_device_config_t`
  model before LPF peripheral configuration sees them.

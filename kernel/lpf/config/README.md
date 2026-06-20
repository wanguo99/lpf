# LPF Runtime Config

This directory contains the `lpf_config_*` source and type names for the LPF
runtime configuration layer. The code is linked into
`lpf_peripheral_runtime.ko` instead of being built as a standalone config
module.

The runtime configuration layer selects a configuration backend, validates the
active platform, and exposes one typed device list to the LPF peripheral
runtime.

## Current Responsibility

- Select the active configuration backend.
- Keep the built-in static table as the first backend implementation.
- Parse LPF Device Tree configuration when an LPF DT node is present.
- Validate platform identity and per-device configuration.
- Build a normalized enabled-device list for MCU and LED entries.
- Keep hardware configuration data separate from LPF peripheral service and
  application logic.

## Backend Selection

Runtime config supports a `backend` module parameter on
`lpf_peripheral_runtime.ko`:

```text
backend=auto    # default: try dt, then static
backend=dt      # require Device Tree backend
backend=static  # require built-in static table backend
```

Explicit backend selection fails if the requested backend is not available.
`auto` is intended for product builds where DT-capable SoC kernels should use
board data from firmware while x86 or lab module builds still fall back to the
compiled static table.

## Static Config Selection

The static backend supports product-line selection without exposing concrete
table symbols to peripheral services:

```text
config_index=N          # select by compiled table index
config_product=name     # match product-name
config_project=name     # match project-name
config_version=x.y.z    # match config-version
```

If `config_index` is omitted, the backend first tries the identity selectors.
For kernel builds, `CONFIG_PROJECT_NAME` and `CONFIG_PROJECT_VERSION` from the
selected defconfig are used as default project/version selectors when the
matching module parameters are omitted. Module parameters override those
generated defaults. If no effective selector is available, the backend falls
back to `g_lpf_config_platform_table.current_index`.

An explicit `config_index` selects that table entry directly. If `config_index`
is combined with explicit identity module parameters, the selected table entry
must also match those fields.

## Public API

```c
const lpf_config_platform_config_t *lpf_config_get_board(void);
const lpf_config_device_config_t *lpf_config_get(void);
const lpf_config_platform_config_t *lpf_config_find(const char *product,
                                             const char *project,
                                             const char *version);
int32_t lpf_config_list(const lpf_config_platform_config_t **configs, uint32_t *count);
int32_t lpf_config_validate(const lpf_config_platform_config_t *config);
void lpf_config_print(const lpf_config_platform_config_t *config);
```

The static backend uses the table symbol from `configs/lpf_config_configs.c`:

```c
extern const lpf_config_platform_table_t g_lpf_config_platform_table;
```

Concrete configs live under:

```text
configs/<product>/<project>/<version>/
```

The Device Tree backend looks for `/lpf`,
`lpf,linux-peripheral-framework`, `linux-peripheral-framework`, or
`lpf,platform-config`. New DTS files should use
`lpf,linux-peripheral-framework`. The root node provides platform identity:

```dts
lpf {
    compatible = "lpf,linux-peripheral-framework";
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

The formal binding is documented in
`docs/devicetree/bindings/lpf/linux-peripheral-framework.yaml`.

## Typed Accessors

The current header provides inline index-based peripheral accessors:

```c
lpf_config_hw_get_mcu(platform, index);
lpf_config_hw_get_led(platform, index);
```

## Layering Rules

- Runtime config owns backend selection, validation, and normalized device
  enumeration.
- `kernel/lpf/config/configs` owns concrete static platform tables.
- LPF peripheral configuration consumes `lpf_config_get()` and typed entries; it
  must not know concrete product table symbols or backend implementations.
- New configuration sources should be added as runtime config backends. They must
  produce the same `lpf_config_platform_config_t` and `lpf_config_device_config_t`
  model before LPF peripheral configuration sees them.

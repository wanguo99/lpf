# LPF Runtime Config

This directory contains the `lpf_config_*` source and type names for the LPF
runtime configuration layer. Backend, validation, normalization, and parser
objects are linked into `lpf_core.ko`; the selected static board description is
built separately as `lpf_configs.ko`.

The runtime configuration layer selects a configuration backend, validates the
active platform, and exposes a configured-device node table to LPF runtime
config drivers.

## Current Responsibility

- Select the active configuration backend.
- Prefer the static table exported by `lpf_configs.ko` in `backend=auto`.
- Fall back to LPF Device Tree configuration when static config is unavailable
  or invalid.
- Validate platform identity and per-device configuration.
- Provide the active configured-device node table; peripheral-owned runtime
  config drivers parse their own typed node payloads and register LPF devices.
- Keep hardware configuration data separate from LPF peripheral service and
  application logic.

## Backend Selection

Runtime config supports a `backend` module parameter on `lpf_core.ko`:

```text
backend=auto    # default: try lpf_configs.ko static config, then dt
backend=static  # require the lpf_configs.ko static table backend
backend=dt      # require Device Tree backend
```

Explicit backend selection fails if the requested backend is not available,
cannot load, or fails validation. It does not fall back to another backend.
`auto` is intended for product and lab builds where custom static configuration
can override board firmware data while DT remains available as a fallback source.

## Static Config Selection

The static backend gets its board-description table from the already loaded
`lpf_configs.ko` module through `g_lpf_config_platform_table`. The config
module only exports data; it does not register devices and does not depend on
`lpf_core.ko`.

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
back to the first linked static config entry.

An explicit `config_index` selects that table entry directly. If `config_index`
is combined with explicit identity module parameters, the selected table entry
must also match those fields.

## Public API

```c
const lpf_config_platform_config_t *lpf_config_get_board(void);
const lpf_config_platform_config_t *lpf_config_find(const char *product,
                                             const char *project,
                                             const char *version);
int32_t lpf_config_list(const lpf_config_platform_config_t **configs, uint32_t *count);
int32_t lpf_config_validate(const lpf_config_platform_config_t *config);
void lpf_config_print(const lpf_config_platform_config_t *config);
```

The static config module exports the selected table:

```c
extern const lpf_config_static_table_t g_lpf_config_platform_table;
```

Concrete configs live under:

```text
kernel/lpf-runtime/config/configs/<product>/<project>/lpf_config_<product>_<project>_vN.c
```

Configuration version identity remains in the table data (`.version`); source
paths should not add a separate version-number directory.

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

The Linux backend keeps OF tree access in `lpf_config_dt_backend.c` and routes
property parsing through `lpf_config_dt_parser.c`. This keeps the normalized
Device Tree parser testable without requiring the host kernel to expose live OF
overlay support.

## Device Nodes

`lpf_config_load()` validates the active board config and exposes an ordered
readonly node table through:

```c
lpf_config_get_device_nodes(&count);
```

Each node carries a device type, source index, name, status, compatible string,
typed payload pointer, and payload size. Static configs should author the board
description as a first-class ordered node table. The platform config still keeps
per-peripheral arrays as a compatibility fallback for older helpers and
transitional backends, but runtime probing consumes the generic node table.

The current header also keeps inline index-based peripheral accessors for
service-owned typed payload lookups and compatibility:

```c
lpf_config_hw_get_mcu(platform, index);
lpf_config_hw_get_led(platform, index);
```

## Layering Rules

- Runtime config owns backend selection, validation, and configured-device node
  enumeration.
- `lpf_config_normalize_devices()` is retained as a compatibility helper for
  tests and comparisons. Runtime probing uses `lpf_config_get_device_nodes()`.
- Runtime device registration is owned by runtime config drivers registered by
  each peripheral service. Runtime walks nodes generically and dispatches each
  enabled node to the matching config driver. Dispatch first tries a matching
  compatible string when a config driver provides one, then falls back to the
  type-only driver for the node's device type.
- `kernel/lpf-runtime/config/configs` owns concrete static platform tables.
- LPF peripheral configuration consumes its own typed node payloads; it must not
  know concrete product table symbols or backend implementations.
- New configuration sources should be added as runtime config backends. They must
  produce the same board-description model before LPF peripheral configuration
  sees them.

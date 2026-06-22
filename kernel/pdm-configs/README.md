# LPF Configs

`lpf_configs.ko` owns the LPF configuration subsystem.

It is responsible for:

- selecting the active configuration provider
- loading and validating the active platform configuration
- normalizing provider data into the shared configured-device node model
- exporting `lpf_config_*` query APIs used by `lpf_core.ko`
- hosting built-in static configuration data and Device Tree parsing

## Layout

```text
kernel/lpf-configs/
├── core/          # provider-independent config engine
├── parser/
│   ├── static/    # built-in static table provider
│   └── dt/        # Device Tree provider
├── products/      # product-specific static platform data
└── selftest/      # config subsystem self-tests
```

`lpf_core.ko` consumes the public configuration API but does not own config
engine implementation details.

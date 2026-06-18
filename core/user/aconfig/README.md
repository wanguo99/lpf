# ACONFIG

ACONFIG is the application configuration query layer. It reads the product-provided application-facing function mapping separately from hardware configuration.

## Layering

- ACONFIG owns the generic read-only query API for application-level mappings and opaque function metadata.
- Product modules own the concrete `g_aconfig_table` definition.
- PCONFIG owns hardware tables.
- Product-specific interpretation belongs in product modules outside the core framework.
- Core ACONFIG APIs must not depend on a concrete product business layer.
- Runtime code must not register, unregister, reload, or mutate ACONFIG tables through core APIs.

## Public API

```c
const aconfig_config_table_t *ACONFIG_GetTable(void);
```

Products provide the table symbol:

```c
extern const aconfig_config_table_t g_aconfig_table;
```

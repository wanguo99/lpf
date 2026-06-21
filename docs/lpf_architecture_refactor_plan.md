# LPF Architecture Refactor Plan

## Purpose

This is the active refactor plan for LPF. It replaces the older overlapping
kernel refactor, long-term optimization, and roadmap trackers with one
architecture-driven checklist.

LPF's current direction is sound: `lpf_core.ko` owns the LPF device model and
the integrated runtime, including runtime configuration backends, LPF HW,
peripheral services, service-owned transport backends, and configured-device
probing. `lpf_configs.ko` is a DTS-like static board-description provider
loaded before Core. The remaining work is to keep this model explicit: LPF
Core should behave like a small LPF-owned pseudo bus and device model, runtime
peripherals should behave like drivers registered on that model, and configs
should behave like DTS-style board descriptions even when the current backend
is a static C table.

## Target Architecture Model

LPF should follow a simplified Linux driver-model flow without inheriting
unneeded Linux bus complexity:

1. `lpf_core.ko` initializes the LPF device model.
2. Runtime peripheral services register `lpf_driver_t` instances with LPF Core.
3. The selected config backend loads the active board description.
4. Runtime walks the board's configured device nodes.
5. A registered runtime config driver parses each matching node.
6. The config driver creates an `lpf_device_config_t` and calls
   `lpf_device_register()`.
7. LPF Core matches the device with a registered driver and calls `probe()`.

In this model:

- `lpf_core.ko` is the LPF device model / pseudo bus. It owns driver
  registration, device registration, matching, probe/remove, discovery, state,
  events, and shared userspace observability.
- `lpf_runtime/peripheral/` owns peripheral drivers such as MCU and LED. These
  drivers register with LPF Core and own their service-specific parsing,
  transport, char device, and debug behavior.
- `lpf_runtime/config/` owns board-description loading and validation. Static C
  tables, product configuration, or a future Device Tree backend are different
  backends behind the same board-description model.
- `backend=auto` tries custom static configuration first, then falls back to
  Device Tree. Explicit backend selection is strict and does not fall back.
- `configs` are treated as DTS-like board descriptions, not as runtime driver
  logic. They should describe what devices exist and carry typed payloads, while
  peripheral config drivers decide how to translate those payloads into LPF
  devices.

The goal is conceptual alignment with Linux driver registration and device
registration, not a direct copy of Linux's `struct bus_type`, full Device Tree
parser, or driver-core internals.

## Current Architecture Strengths

- Kernel module boundaries are clear: `osal.ko`, `lpf_configs.ko`, and
  `lpf_core.ko`.
- Source layout now follows module ownership with `kernel/lpf-core/` and
  `kernel/lpf-runtime/`.
- Kbuild object selection is used for trim-friendly services, service-owned
  transport backends, HW paths, and runtime self-tests.
- LPF Core provides one device/driver lifecycle, discovery snapshots,
  reference-counted device handles, state/error tracking, and shared node
  helpers.
- LPF Core already follows the critical driver-model path where device
  registration matches a registered driver and calls `probe()`.
- Runtime peripheral services already register drivers, and runtime config
  drivers already own MCU/LED config-to-device creation.
- UAPI and PDI are separated, with ABI-only headers under `uapi/lpf/`.

## Architecture Defects To Fix

1. **Layer dependency inversion**
   - Fixed in the first cleanup pass by moving shared GPIO/PWM/I2C/SPI/CAN/UART
     types into per-capability headers under `kernel/include/lpf/types/`.
   - The rule is now explicit: SoC Adapter is below LPF HW, so it must not
     include LPF HW headers or depend on a single unrelated type bucket.

2. **Centralized runtime device mapping**
   - `kernel/lpf-runtime/runtime/lpf_runtime_config.c` hard-codes MCU and LED
     config-to-device mapping.
   - Adding a peripheral should not require editing runtime core code.

3. **Implicit section ordering**
   - Fixed by replacing the single runtime entry registration API with
     class-specific APIs for core, service, and self-test entries.
   - Runtime initialization now follows class order, while exit runs in reverse
     class order and reverse declaration order within each class.

4. **Core lifecycle ownership is blurred**
   - Fixed by making Core init/deinit private to `lpf_core.ko` module
     init/exit.
   - Public Core APIs now require initialized Core state and return invalid
     state instead of implicitly creating Core global state.

5. **Product security policy is not explicit**
   - Fixed for instance misc devices by adding a configurable
     `CONFIG_LPF_INSTANCE_DEVNODE_MODE` policy with a conservative `0660`
     default.
   - Target-level verification still needs to confirm devtmpfs/udev ownership
     and read-only observability surfaces on product-like kernels.

6. **Board config shape is still partially transitional**
   - Fixed for runtime probing by introducing a generic configured-device node
     table and making runtime traverse nodes instead of peripheral arrays.
   - Fixed for static config authoring by making static C configs publish
     first-class configured-device node tables.
   - Per-peripheral arrays remain only as a compatibility fallback for older
     helpers and transitional backends.

## Refactor Phases

### Phase 1: Repair Layer Dependencies

- [x] Move shared hardware capability types out of `lpf/hw/`.
- [x] Introduce neutral, per-capability type headers under
      `kernel/include/lpf/types/`.
  - `lpf_gpio_types.h`
  - `lpf_pwm_types.h`
  - `lpf_i2c_types.h`
  - `lpf_spi_types.h`
  - `lpf_can_types.h`
  - `lpf_serial_types.h`
- [x] Make both LPF HW and SoC Adapter depend on the neutral capability type
      headers.
- [x] Add or update architecture-boundary tests to catch SoC -> HW include
      regressions.

Acceptance:
- SoC Adapter headers do not include LPF HW headers.
- `make modules` and `make tests` pass.

### Phase 2: Decentralize Runtime Device Mapping

- [x] Define a runtime config driver entry type.
- [x] Add a section-registration helper for config-to-device drivers.
- [x] Move MCU config parsing and device registration into the MCU
      service/config object.
- [x] Move LED config parsing and device registration into the LED
      service/config object.
- [x] Make `lpf_runtime_probe_devices()` use registered config drivers instead
      of a switch or a central normalized-device list.

Acceptance:
- Adding a new peripheral does not require editing
  `kernel/lpf-runtime/runtime/lpf_runtime_config.c`.

### Phase 3: Add Runtime Entry Classes

- [x] Replace single unordered runtime entry traversal with explicit entry
      classes or priorities.
- [x] Keep function-style registration APIs, for example
      `lpf_runtime_core_register()`, `lpf_runtime_service_register()`, and
      `lpf_runtime_selftest_register()`.
- [x] Preserve reverse-order exit within each class.

Acceptance:
- Runtime service order is defined by API class or priority, not by incidental
  Makefile ordering.

### Phase 4: Harden Core Lifecycle And Runtime Ownership

- [x] Make LPF Core public registration APIs require initialized Core state.
- [x] Keep module init/exit as the only owner of Core init/deinit.
- [x] Audit runtime failure paths for symmetric cleanup.
- [x] Document module load order and failure behavior.

Acceptance:
- No public Core registration API implicitly initializes Core.
- Load/unload smoke test passes repeatedly.

### Phase 5: Productization And Security

- [x] Make LPF instance device node permissions configurable.
- [x] Default product builds to a conservative mode such as `0660`.
- [x] Document udev or product policy integration.
- [x] Add target-smoke checks for `/dev/lpf/*` mode policy, read-only
      sysfs/procfs inspection nodes, and writable debugfs command nodes.
- [ ] Verify `/dev/lpf/*`, sysfs, procfs, and debugfs behavior on a real or
      target-like kernel.

Acceptance:
- Product builds do not expose writable peripheral nodes to all users by
  default.

### Phase 6: Validation Expansion

- [x] Extend `make mock-modules-smoke` so it checks loaded mock runtime
      `/dev`, sysfs, procfs, debugfs, and basic PDI/CTL ioctl behavior.
      This includes instance devnode mode validation against the active
      Kconfig policy and rejects world-writable instance nodes.
- [ ] Run `make mock-modules-smoke` on a compatible kernel.
- [ ] Verify `/dev/lpf/mcuN` and `/dev/lpf/ledN` creation.
- [ ] Verify configured and unconfigured PDI ioctl behavior.
- [ ] Keep kernel matrix builds active for supported kernel versions.
- [x] Add ABI/PDI coverage for current MCU and LED peripheral discovery,
      configured/unconfigured paths, and operation marshaling.

Acceptance:
- Module build, unit tests, mock module load tests, and target smoke checks all
  pass for the selected release configuration.

### Phase 7: Make Configs DTS-Like Device Descriptions

- [x] Introduce a generic readonly config device-node descriptor that carries
      stable fields such as type, name, index, enabled/status, compatible
      string, typed payload pointer, and payload size.
- [x] Build or expose the active platform config as an ordered device-node
      table during `lpf_config_load()`.
- [x] Keep existing static C configs as the first backend, but make their data
      consumable through the generic node table.
- [x] Make auto backend selection try static config first and fall back to
      Device Tree without exposing the source to runtime config drivers.
- [x] Change runtime config drivers to parse one matching node at a time rather
      than receiving and scanning the whole platform config.
- [x] Make `lpf_runtime_probe_devices()` walk configured nodes generically and
      dispatch each node to a registered runtime config driver by type or
      compatible.
- [x] Keep MCU and LED payload structs module-owned or config-owned as needed,
      but prevent runtime core from knowing peripheral-specific arrays.
- [x] Add architecture tests that reject new central MCU/LED-style mapping in
      runtime core.
- [x] Move static config authoring from per-peripheral arrays toward
      first-class node tables for local code cleanup; target smoke validation
      remains tracked in Phases 5 and 6.
- [x] Add compatible-string dispatch when multiple drivers need to bind the
      same LPF config device type.

Acceptance:
- Adding a new peripheral requires adding its config type/payload, config
  driver, and peripheral driver, without editing runtime config traversal.
- Static configs look and behave like board device descriptions instead of
  hard-coded runtime mapping tables.
- `make tests` and `make modules` pass.

### Phase 8: Document And Harden The LPF Driver Model

- [x] Document LPF Core as the LPF device model / pseudo bus in the runtime and
      peripheral README files.
- [x] Document the expected ordering: core init, peripheral driver
      registration, config load, device-node traversal, device registration,
      Core match, and driver probe.
- [x] Define match rules explicitly: initially by LPF device type, with a path
      to add compatible-string matching only when needed.
- [x] Audit exported Core APIs so names and comments reflect device-model
      ownership rather than ad hoc registries.
- [x] Avoid introducing a separate physical `bus` or `transport` layer unless a
      concrete future requirement needs it.

Acceptance:
- The code and docs consistently describe one LPF device model.
- Peripheral ownership remains module-local, and no new generic abstraction is
  introduced without a real shared behavior.

## Implementation Rules

- Prefer module-owned private headers unless another module must consume the
  API.
- Keep feature selection at Kbuild object and section-registration boundaries.
- Do not add product-specific behavior under shared framework directories.
- Treat LPF Core as a pseudo bus/device model concept, but do not create extra
  bus-layer directories or KOs unless there is a concrete need.
- Treat configs as DTS-like board descriptions, but do not require a Device
  Tree parser for static configuration.
- Do not split MCU and LED into separate KOs unless the runtime boundary is
  intentionally redesigned.
- Update this plan when implementation intentionally differs from the target.

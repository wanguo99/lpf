# LPF Architecture Refactor Plan

## Purpose

This is the active refactor plan for LPF. It replaces the older overlapping
kernel refactor, long-term optimization, and roadmap trackers with one
architecture-driven checklist.

LPF's current direction is sound: `lpf_core.ko` owns the device and driver
lifecycle, while `lpf_runtime.ko` hosts runtime configuration, LPF HW,
peripheral services, transports, and configured-device probing. The remaining
work is to remove transitional coupling and make new peripherals or SoC
backends extensible without editing central runtime files.

## Current Architecture Strengths

- Kernel module boundaries are clear: `osal.ko`, `lpf_core.ko`, and
  `lpf_runtime.ko`.
- Source layout now follows module ownership with `kernel/lpf-core/` and
  `kernel/lpf-runtime/`.
- Kbuild object selection is used for trim-friendly services, transports, HW
  paths, and runtime self-tests.
- LPF Core provides one device/driver lifecycle, discovery snapshots,
  reference-counted device handles, state/error tracking, and shared node
  helpers.
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
   - LPF Core module initialization and public registration APIs can both call
     `lpf_core_init()`.
   - Public APIs should check readiness instead of implicitly owning module
     initialization.

5. **Product security policy is not explicit**
   - LPF instance misc devices currently default to broad permissions.
   - Device mode should be configurable and default conservatively for product
     builds.

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

- [x] Define a runtime config mapper entry type.
- [x] Add a section-registration helper for config-to-device mappers.
- [x] Move MCU mapping into the MCU service/config object.
- [x] Move LED mapping into the LED service/config object.
- [x] Make `lpf_runtime_probe_devices()` use registered mappers instead of a
      switch over known device types.

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

- [ ] Make LPF Core public registration APIs require initialized Core state.
- [ ] Keep module init/exit as the only owner of Core init/deinit.
- [ ] Audit runtime failure paths for symmetric cleanup.
- [ ] Document module load order and failure behavior.

Acceptance:
- No public Core registration API implicitly initializes Core.
- Load/unload smoke test passes repeatedly.

### Phase 5: Productization And Security

- [ ] Make LPF instance device node permissions configurable.
- [ ] Default product builds to a conservative mode such as `0660`.
- [ ] Document udev or product policy integration.
- [ ] Verify `/dev/lpf/*`, sysfs, procfs, and debugfs behavior on a real or
      target-like kernel.

Acceptance:
- Product builds do not expose writable peripheral nodes to all users by
  default.

### Phase 6: Validation Expansion

- [ ] Run `make mock-modules-smoke` on a compatible kernel.
- [ ] Verify `/dev/lpf/mcuN` and `/dev/lpf/ledN` creation.
- [ ] Verify configured and unconfigured PDI ioctl behavior.
- [ ] Keep kernel matrix builds active for supported kernel versions.
- [ ] Add ABI/PDI coverage whenever a new peripheral is introduced.

Acceptance:
- Module build, unit tests, mock module load tests, and target smoke checks all
  pass for the selected release configuration.

## Implementation Rules

- Prefer module-owned private headers unless another module must consume the
  API.
- Keep feature selection at Kbuild object and section-registration boundaries.
- Do not add product-specific behavior under shared framework directories.
- Do not split MCU and LED into separate KOs unless the runtime boundary is
  intentionally redesigned.
- Update this plan when implementation intentionally differs from the target.

# LPF Long-Term Optimization Plan

## Purpose

LPF is intended to provide one stable framework codebase for different Linux
kernel versions, different SoC families, and different product platforms. The
long-term direction is not to fully depend on the Linux native device model as
the framework center. Instead, LPF should keep a stable upper-layer peripheral
business model and absorb kernel, SoC, and vendor BSP differences in lower
adapter layers.

The target positioning is:

```text
LPF is a Linux peripheral business abstraction framework for multi-kernel,
multi-SoC, and multi-product deployments.
```

## Design Principles

- Keep upper layers stable and move kernel or SoC differences downward.
- Keep an LPF-owned device and peripheral model as the framework center.
- Use the Linux device model, sysfs, debugfs, and device nodes as integration
  outputs, not as the only internal architecture.
- Keep external product logic outside the shared framework.
- Keep peripheral business behavior in LPF peripheral services.
- Keep configuration source replaceable.
- Test backend consistency across kernel versions and SoC adapters.

## Target Architecture

```text
Application / Product
        ↓
PDI - userspace SDK
        ↓
LPF UAPI / ioctl
        ↓
LPF Peripheral Service Layer
        ↓
LPF Core
        ↓
LPF Transport Layer
        ↓
LPF HW API
        ↓
SoC Adapter Layer
        ↓
Kernel Compat Layer
        ↓
Linux Kernel / Vendor BSP / Hardware
```

## Phase 1: Architecture Baseline

Goal: lock down the framework concepts before larger code movement.

Work items:

- Define final responsibilities for OSAL, LPF Core, LPF runtime config, LPF HW
  API, PDI, and UAPI.
- Reframe the old peripheral module responsibilities as the LPF Peripheral
  Service Layer.
- Reframe the old local peripheral bus as the future LPF Core.
- Document allowed dependencies and forbidden dependencies for each layer.
- Document the target directory layout.

Deliverables:

- `docs/LPF_TARGET_ARCHITECTURE.md`
- `docs/LPF_MODULE_BOUNDARIES.md`
- `docs/LPF_REFACTOR_ROADMAP.md`

Acceptance criteria:

- Every module has a clear owner and boundary.
- New code placement can be decided from the documentation.
- Kernel version logic, SoC logic, and peripheral business logic have separate
  target layers.

## Phase 2: LPF Core

Goal: upgrade the old local peripheral bus idea into the formal LPF device
management core.

Work items:

- Add `kernel/lpf/core/`.
- Define `lpf_device`.
- Define `lpf_driver`.
- Define `lpf_device_id`.
- Define `lpf_capability`.
- Define lifecycle state and event handling.
- Support device registration, driver registration, probe, remove, and state
  management.
- Keep internal indexes where useful, but add stable names such as `mcu0` and
  `led0`.
- Add capability query support for userspace discovery.

Deliverables:

- `lpf_core.ko`
- `kernel/include/lpf/lpf_core.h`
- `kernel/include/lpf/lpf_device.h`
- `kernel/include/lpf/lpf_driver.h`

Acceptance criteria:

- MCU and LED can both register as LPF devices through LPF Core.
- Global arrays are no longer the primary device model.
- Device lifecycle has one consistent framework entry.

Current status:

- Started. `lpf_core.ko` now owns the LPF driver and device registry.
- `kernel/include/lpf/lpf_device.h` and `lpf_driver.h` define the first device,
  driver, and capability model.
- LPF peripheral configuration maps LPF_CONFIG entries into
  `lpf_device_config_t`; MCU/LED services register through LPF Core.
- Started device discovery. LPF Core now exposes snapshot APIs for listing
  devices and querying by type/index, name, or capability without exposing
  internal list nodes to callers.
- Started lifecycle hardening. LPF Core now provides reference-counted active
  device handles, name/capability handle lookup, state updates, and kernel
  event notification for register, bind, state, error, remove start, and remove
  completion.
- Started runtime integration. LPF instance character devices acquire an LPF
  active device handle on open and release it on close, so LPF Core removal
  waits for active instance users before invoking peripheral `remove`.
- Remaining work: define the final userspace event delivery model and deepen
  the state/recovery model used by peripheral services.

## Phase 3: Kernel Compat Layer

Goal: isolate Linux kernel API differences behind explicit compatibility
wrappers.

Work items:

- Add `kernel/lpf/compat/`.
- Add feature detection for supported kernel versions.
- Wrap GPIO API differences.
- Wrap PWM API differences.
- Wrap TTY and serdev differences.
- Wrap procfs, debugfs, and sysfs API differences.
- Wrap common time, mutex, atomic, workqueue, and usercopy differences where
  needed.
- Forbid peripheral business code from using `LINUX_VERSION_CODE` directly.

Deliverables:

- `lpf_compat_gpio.h`
- `lpf_compat_pwm.h`
- `lpf_compat_tty.h`
- `lpf_compat_time.h`
- A compatibility policy document.

Acceptance criteria:

- Peripheral service code contains no direct kernel-version branching.
- Selected target kernels compile through the compat layer. Suggested initial
  targets: 4.19, 5.10, and 6.1.

Current status:

- Started. CAN, serial, GPIO, PWM, I2C, and SPI Linux API wrappers now live
  under `kernel/lpf/compat/`.
- Remaining work: add explicit kernel-version feature detection and migrate
  procfs/debugfs/sysfs, workqueue, and usercopy differences.

## Phase 4: SoC Adapter Layer

Goal: isolate SoC and vendor BSP differences behind adapter implementations.

Work items:

- Add `kernel/lpf/soc/`.
- Define SoC adapter interfaces for GPIO, PWM, I2C, SPI, CAN, UART, and other
  required hardware capabilities.
- Provide a generic Linux adapter using standard kernel APIs.
- Add SoC-specific adapters only under the SoC layer.
- Prevent vendor BSP calls from entering LPF HW or peripheral business code.

Deliverables:

- `kernel/include/lpf/lpf_soc_adapter.h`
- `kernel/lpf/soc/generic_linux/`
- Future adapter directories such as `soc/am62x/`, `soc/imx8/`, or
  `soc/rk3568/`.

Acceptance criteria:

- The same LED or MCU business code can compile against different SoC adapters.
- SoC-specific code is limited to adapter implementations.

Current status:

- Started. `kernel/include/lpf/lpf_soc_adapter.h` defines the initial CAN,
  serial, GPIO, PWM, I2C, and SPI adapter interfaces.
- `kernel/lpf/soc/generic_linux/` provides the default Linux backend.
- Started. `kernel/lpf/soc/mock/` provides a Kconfig-selectable mock SoC
  backend for GPIO, PWM, I2C, SPI, CAN, and serial paths, with
  `kernel_x86_mock_modules_defconfig` as a no-hardware module build preset.
- Remaining work: extend the interface to pinctrl, clocks, resets, and SoC
  identity, then add target SoC adapters.

## Phase 5: LPF HW API Layer

Goal: keep useful hardware-capability semantics as LPF-internal HW APIs above
the SoC adapter layer.

Work items:

- Add `kernel/lpf/hw/` for LPF-internal hardware capability APIs.
- Migrate useful hardware state/context handling into:
  - `lpf_hw_gpio`
  - `lpf_hw_pwm`
  - `lpf_hw_transport_can`
  - `lpf_hw_transport_uart`
  - `lpf_hw_bus_i2c`
  - `lpf_hw_bus_spi`
- Make LPF peripheral services call `lpf_hw_*`.
- Make `lpf_hw_*` call the SoC adapter layer internally.
- Delete the old standalone hardware Kconfig symbols and module build target.
- Delete old standalone hardware headers after the API surface has migrated to
  LPF-internal `lpf_hw_*` APIs.
- Remove hard-coded global limits where possible.
- Move mock operation-path coverage from the old hardware selftest to LPF
  HW/SoC adapter tests.

Deliverables:

- `kernel/lpf/hw/`
- LPF HW API reference.
- Removal of the old standalone hardware access directory from the repository.

Acceptance criteria:

- Peripheral business code depends only on LPF-owned HW APIs.
- No LPF peripheral service includes old hardware access headers.
- Module builds no longer produce the old standalone hardware module.
- Hardware behavior stays consistent across supported kernel and SoC backends.

Current status:

- Done. `kernel/lpf/hw/` now owns LPF-internal hardware access APIs for GPIO,
  PWM, CAN, UART, I2C, and SPI.
- Done. LPF peripheral services and MCU transports call `lpf_hw_*` APIs.
- Done. LPF HW paths call LPF SoC Adapter APIs instead of Linux subsystem APIs
  directly.
- Done. The old standalone hardware module target, old hardware Kconfig
  symbols, old hardware access directory, and standalone hardware headers have
  been removed.
- Done. `lpf_hw_mock_selftest.ko` exercises LPF HW GPIO, PWM, CAN, UART, I2C,
  and SPI operation paths over the mock SoC backend when the module is loaded.
- Started. LPF HW GPIO no longer uses a fixed global GPIO table; requested GPIO
  contexts are tracked dynamically by GPIO number.
- Remaining work: continue removing hard-coded global limits where they are not
  ABI or protocol constraints, and broaden LPF HW/SoC adapter self-test
  coverage.

## Phase 6: Runtime Configuration Layer

Goal: move runtime configuration responsibilities into the LPF peripheral runtime as its
configuration input layer, while keeping backend-based configuration loading.

Work items:

- Keep the static table backend for product builds.
- Add a backend abstraction.
- Add a Device Tree backend.
- Add a module parameter backend where useful.
- Add a userspace-loaded configuration backend if runtime configuration is
  required.
- Add a board profile backend for product-line selection.
- Make all backends produce the same `lpf_device_config` model.
- Move per-device validation into per-device validators.
- Delete the standalone configuration Kconfig/module boundary.
- Move source layout to `kernel/lpf/config/`.
- Rename public-internal APIs and types to `lpf_config_*` after the module
  boundary is removed.

Deliverables:

- Runtime-linked config backend objects.
- `lpf_config_backend_ops`
- `lpf_config_static_backend`
- `lpf_config_dt_backend`
- `lpf_config_validator`

Acceptance criteria:

- The same MCU or LED device can be created from static table or Device Tree.
- LPF Core and peripheral services do not care where configuration came from.
- Module builds no longer produce a standalone configuration module.
- Runtime config unloads when `lpf_peripheral_runtime.ko` unloads.

Current status:

- Done. LPF runtime config has a backend abstraction, a static table backend, and
  a separate per-device validator/descriptor file.
- `lpf_config_load()`, `lpf_config_get_board()`, `lpf_config_find()`, and
  `lpf_config_list()` now route through the selected backend instead of directly
  reading `g_lpf_config_platform_table`.
- Backend selection now supports `backend=auto`, `backend=dt`, and
  `backend=static`. `auto` tries Device Tree first and falls back to the static
  table.
- Started the Device Tree backend. It parses LPF root identity, MCU CAN/Serial,
  and LED GPIO/PWM entries into the same normalized LPF config model.
- Done. LPF runtime config backend objects are linked into
  `lpf_peripheral_runtime.ko`, and the standalone configuration module boundary
  has been removed from the build.
- Done. Runtime config source files now live under `kernel/lpf/config/`, public
  kernel-internal headers live under `kernel/include/lpf/`, and APIs/types use
  the `lpf_config_*` namespace.
- Done. The static backend now supports runtime product selection by compiled
  table index or by product/project/version identity fields.
- Done. The current Device Tree format is documented as
  `docs/devicetree/bindings/lpf/linux-peripheral-framework.yaml`.
- Remaining work: add a board-profile backend and broader peripheral coverage.

## Phase 7: Peripheral Service Layer

Goal: make MCU, LED, and future peripherals reusable service implementations
instead of product-specific drivers.

Work items:

- Move current MCU and LED implementations toward:
  - `kernel/lpf/peripheral/mcu/`
  - `kernel/lpf/peripheral/led/`
- For every peripheral type, define:
  - probe and remove
  - capability reporting
  - state machine
  - error handling and recovery
  - debug dump
  - userspace operation dispatch
- Move MCU transport into a separate abstraction:
  - CAN transport
  - UART transport
  - future I2C or SPI transport if needed

Deliverables:

- `kernel/lpf/peripheral/mcu/`
- `kernel/lpf/peripheral/led/`
- `kernel/lpf/transport/mcu/`
- Unified service registration in the existing framework integration module.

Acceptance criteria:

- MCU and LED services do not directly depend on SoC-specific APIs.
- MCU transport can be replaced without changing MCU business logic.
- Peripheral state, error, and recovery behavior are consistent.

Current status:

- Started. MCU and LED service code now live under `kernel/lpf/peripheral/`
  with public kernel service headers at `kernel/include/lpf/`.
- Started. MCU and LED services register as LPF drivers and expose
  `/dev/lpf/mcuN` and `/dev/lpf/ledN`; both remain integrated through
  `lpf_peripheral_runtime.ko` so deployment does not fragment into one KO per
  peripheral.
- Done. Unified peripheral service registration lives in
  `kernel/lpf/peripheral/lpf_peripheral.c`; the LPF peripheral runtime entry
  calls that service entry instead of registering MCU/LED services directly in
  the module shell.
- Done. Runtime config-to-LPF device mapping lives in
  `kernel/lpf/peripheral/lpf_peripheral_config.c`; the LPF peripheral runtime
  calls the LPF peripheral probe entry instead of owning per-device capability
  mapping in the module shell.
- Done. LPF Core initialization, peripheral service registration, and
  configured-device probing are now wrapped by the LPF peripheral runtime
  entry.
- Done. The obsolete public header for the old module shell has been removed;
  runtime version
  reporting is now owned by the LPF peripheral runtime entry.
- Started. MCU CAN/UART implementations have moved behind
  `kernel/lpf/transport/mcu/` and are selected through the LPF MCU transport
  registry instead of direct service dependencies.
- Done. The framed peripheral protocol has moved into the LPF protocol layer
  under `kernel/lpf/protocol/`, with public protocol headers under
  `kernel/include/lpf/` and encode/decode symbols exported by `lpf_core.ko`.
- Done. The old compatibility shell has been renamed to the integrated LPF
  peripheral runtime module `lpf_peripheral_runtime.ko`; the legacy module
  entry has been removed.
- Remaining work: continue hardening the integrated LPF peripheral runtime
  boundary while keeping services inside the framework module instead of
  splitting per-peripheral KOs.

## Phase 8: UAPI And PDI Separation

Goal: decouple stable ABI headers from userspace SDK APIs.

Work items:

- Keep UAPI headers ABI-only.
- Move userspace SDK declarations out of UAPI headers.
- Use this target layout:

```text
uapi/lpf/lpf_mcu.h
uapi/lpf/lpf_led.h
user/pdi/include/pdi/mcu.h
user/pdi/include/pdi/led.h
user/pdi/src/mcu.c
user/pdi/src/led.c
```

- Add device discovery APIs:
  - `pdi_list_devices`
  - `pdi_open_by_name`
  - `pdi_get_capability`
- Standardize error mapping in PDI.

Deliverables:

- New UAPI directory layout.
- New PDI SDK headers.
- Updated ABI rules document.

Acceptance criteria:

- UAPI headers contain only ioctl constants, fixed-layout structures, and ABI
  constants.
- PDI hides ioctl details from application code.
- Applications can open devices by stable name or capability.

Current status:

- Started. The LPF Core-owned `/dev/pdm_ctl` node and `uapi/lpf/lpf_ctl.h`
  expose LPF device discovery snapshots to userspace.
- Started. PDI now provides `pdi_list_devices`,
  `pdi_get_device_by_name`, and `pdi_get_device_by_capability`.
- Started `pdi_open_by_name` through type-specific helpers
  `pdi_mcu_open_by_name` and `pdi_led_open_by_name`; these validate stable LPF
  names through `/dev/pdm_ctl` before opening the current aggregated peripheral
  nodes.
- Done. UAPI headers now live under the final `uapi/lpf/` namespace with
  `LPF_*` ABI types and ioctl constants. MCU and LED UAPI headers are ABI-only,
  while SDK declarations live under `user/pdi/include/pdi/`.
- Done. PDI now uses internal error helpers to standardize its public return
  convention: success returns `0`, failures return `-1` with `errno` set, and
  system-call failures preserve the kernel/libc errno value.
- Done. Instance-aware device nodes are available as `/dev/lpf/mcuN` and
  `/dev/lpf/ledN`, and PDI name-based open helpers resolve stable LPF names to
  those nodes through discovery.
- Remaining work: decide whether LPF device events should be exposed to
  userspace through the control ABI or kept as a kernel-only service mechanism.

## Phase 9: Device Nodes And Observability

Goal: provide maintainable runtime access and debug visibility.

Work items:

- Introduce a unified LPF character-device layer.
- Keep aggregated nodes only if they are useful as management nodes.
- Move toward instance-aware nodes such as:

```text
/dev/lpf/mcu0
/dev/lpf/led0
```

- Move debug-only operations from procfs to debugfs.
- Add sysfs read-only attributes for:
  - name
  - type
  - state
  - capability
  - backend
  - SoC
  - error count

Deliverables:

- `lpf_chrdev`
- `lpf_debugfs`
- `lpf_sysfs`

Acceptance criteria:

- Every peripheral instance can be inspected independently.
- Device permissions can be controlled by userspace policy such as udev.
- Debug interfaces are separate from stable user ABI.

Current status:

- Started. `/dev/pdm_ctl` remains the management/discovery node, is now
  implemented by LPF Core, and configured peripheral instances expose
  `/dev/lpf/mcuN` and `/dev/lpf/ledN` nodes.
- Started. Instance character devices now expose read-only sysfs attributes:
  `name`, `type`, `index`, `state`, `capabilities`, `driver`, `soc`,
  `last_error`, `error_count`, and `open_count`.
- Done. Debug-only MCU and LED write commands now live under debugfs:
  `/sys/kernel/debug/lpf/mcu` and `/sys/kernel/debug/lpf/led`; `/proc/lpf/mcu`
  and `/proc/lpf/led` are read-only status snapshots.
- Done. Runtime ioctl and debugfs command failures update each instance's
  `last_error` and `error_count` sysfs attributes.
- Started. Legacy local character-device, sysfs, and debugfs helper
  implementations have been extracted into LPF infrastructure under
  `kernel/lpf/core/` and are linked into `lpf_core.ko` as `lpf_chrdev`,
  `lpf_sysfs`, and `lpf_debugfs`.
- Done. The control/discovery character device implementation has moved from
  the old module shell into LPF Core as `kernel/lpf/core/lpf_ctl.c`; LPF
  peripheral runtime no longer owns `/dev/pdm_ctl` registration.
- Done. Procfs and OSAL-status-to-errno helpers have been extracted into LPF
  helpers, and migrated peripheral services no longer depend on legacy
  proc/status wrappers.
- Remaining work: migrate any future peripheral services to use these LPF
  helpers directly instead of adding local wrapper headers.

## Phase 10: Test And Validation System

Goal: prove that LPF behaves consistently across kernel versions, SoC adapters,
and configuration backends.

Work items:

- Add a new test layout:

```text
tests/kernel/
tests/user/
tests/mock/
tests/integration/
```

- Add mock LPF HW backend coverage.
- Add dummy peripheral service.
- Add mock MCU transport.
- Add PDI userspace tests.
- Add ABI layout checks.
- Add matrix builds for supported kernel versions.

Acceptance criteria:

- Static config backend is tested.
- Mock SoC adapter is tested.
- MCU mock transport is tested.
- LED mock LPF HW path is tested.
- PDI open, operation, and close paths are tested.
- ABI structure layout changes are detected.

Current status:

- Started. Added the top-level `make tests` entry and CMake/CTest integration.
- Started. Added the `tests/` layout with `tests/user/abi/` as the first
  active test area.
- Done. UAPI ABI layout checks now compile and run through CTest, covering
  LPF control, MCU, and LED UAPI structure sizes, field offsets, ABI versions,
  and ioctl command encodings.
- Started. PDI userspace tests now cover open/close context validation,
  missing-device failures, stable-name input validation, and basic
  validation/error paths for control, MCU, and LED APIs.
- Done. PDI operation-path tests now use an internal syscall mock boundary to
  validate stable-name discovery, generated instance-node paths, MCU ioctl
  payloads, LED ioctl payloads, and close handling without requiring live LPF
  device nodes.
- Started. Added a Kconfig-selectable mock SoC backend and
  `kernel_x86_mock_modules_defconfig` so kernel module builds can validate LPF
  HW and peripheral-service integration paths without live hardware.
- Done. `LPF_HW_MOCK_SELFTEST` builds a load-time kernel self-test module for
  LPF HW operation paths over the mock SoC backend.
- Remaining work: add dummy peripheral services, module-load automation for
  mock self-tests, and multi-kernel matrix builds.

## Recommended Implementation Order

1. Write the target architecture and module boundary documents.
2. Introduce LPF Core.
3. Add the kernel compat layer.
4. Add the SoC adapter layer.
5. Maintain the LPF HW API layer above the SoC adapter.
6. Finish merging LPF_CONFIG naming and headers into the LPF runtime
   configuration layer.
7. Migrate LED first as the simplest complete peripheral service.
8. Migrate MCU and split transport handling inside the LPF framework layers.
9. Split UAPI and PDI.
10. Add debugfs, sysfs, and test coverage.

LED was migrated before MCU because it is simpler and validated the new model
with less protocol and transport complexity. MCU now follows the same model
with a separate transport abstraction.

## Final Acceptance Criteria

- One peripheral business codebase compiles across supported kernel versions.
- One peripheral business codebase works across supported SoC backends.
- Peripheral business code does not directly depend on Linux kernel API
  details.
- Kernel version differences are isolated in the compat layer.
- SoC and vendor BSP differences are isolated in the adapter layer.
- Configuration source is replaceable.
- UAPI and PDI SDK are separated.
- LPF has a unified device model, lifecycle model, state model, and debug
  entry.
- MCU and LED both complete the new architecture path.

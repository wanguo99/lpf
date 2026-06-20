# Kernel Refactor Plan

This document tracks the remaining work for moving LPF HAL/PConfig/PDM
to the kernel-module architecture. Update the checkboxes as each part is
implemented and verified.

Note: this is the historical kernel-module refactor tracker. The long-term LPF
architecture is now tracked in `docs/LPF_LONG_TERM_OPTIMIZATION_PLAN.md`. PDM's
local bus has been promoted into LPF Core, so new work should use LPF Core for
device and driver lifecycle instead of adding new PDM-local bus code.

## Target Architecture

- HAL is a kernel-only module built as `hal.ko`.
- PConfig is a standalone kernel module built as `pconfig.ko`.
- PConfig selects a configuration backend. The current static backend consumes
  platform configs compiled under
  `kernel/pconfig/configs/<product>/<project>/<version>/`.
- LPF Core is a standalone kernel module built as `lpf_core.ko`; it owns the
  framework device and driver registry.
- PDM is a kernel module built as `pdm.ko`; current LPF peripheral services are
  linked into `pdm.ko` while their code lives under `kernel/lpf/peripheral/`.
- PConfig supplies device instances; PDM maps those instances into LPF device
  configs, and LPF Core matches each instance to a driver and owns remove
  ordering.
- PDI remains a userspace library, but it only talks to per-peripheral character
  devices and UAPI headers.
- Each peripheral owns its own PConfig type, PDM driver, LPF UAPI header, PDI
  userspace wrapper, character device, and ioctl namespace.
- Feature selection is handled by Kbuild object selection and registration
  tables, not by business-code `#ifdef CONFIG_*` branches.
- OSAL hides kernel/userspace differences where equivalent semantics are
  practical. Unsupported APIs should be explicitly documented or return
  `OSAL_ERR_NOT_SUPPORTED`.

## Current Completed Work

- [x] Move HAL implementation to kernel-side `kernel/hal/src`.
- [x] Build HAL as `hal.ko` with module entry in `hal.c`.
- [x] Remove HAL userspace implementation from the active module path.
- [x] Rename kernel module entry files from `main.c` to module names.
- [x] Move HAL source files directly under `src` without an extra `kernel`
      subdirectory.
- [x] Add per-module version printing for OSAL, HAL, PConfig, and PDM.
- [x] Keep OSAL version printing in `osal.c` instead of a separate
      `osal_version.c`.
- [x] Add `module_init`/`module_exit` for OSAL.
- [x] Add `module_init`/`module_exit` for PConfig.
- [x] Build PDM as `pdm.ko` with module entry in `pdm.c`.
- [x] Split PDI MCU UAPI into `uapi/lpf/lpf_mcu.h`.
- [x] Split userspace PDI MCU wrapper into `user/pdi/src/pdi_mcu.c`.
- [x] Add `/dev/lpf/mcuN` character-device ioctl boundary.
- [x] Change PDM startup to initialize built-in drivers, load PConfig, iterate
      configured devices, and call matching driver `probe`.
- [x] Add built-in peripheral service registration through LPF Core.
- [x] Move built-in peripheral service registration out of PDM-local wrappers
      into `kernel/lpf/peripheral/lpf_peripheral.c`.
- [x] Replace the PDM-local virtual bus/list manager with LPF Core for driver
      and device binding.
- [x] Keep PDM peripheral drivers linked into `pdm.ko` instead of adding one
      kernel module per peripheral.
- [x] Add HAL built-in initialization table for HAL subdrivers that need global
      module initialization.
- [x] Remove ctest product files from the current tree.
- [x] Create a tag for the last pure userspace implementation and merge the
      refactor branch back to `master`.
- [x] Keep concrete platform configuration inside the original PConfig module
      instead of adding a separate product config kernel module.
- [x] Introduce PCONFIG backend selection and move the original static table
      behind a static backend.
- [x] Add HAL PWM support and LPF LED service support with GPIO/PWM control.

## Phase 1: Make The Current Kernel Modules Loadable

- [x] Add a real product/platform PConfig source for the kernel module build.
  - `pconfig.ko` links `g_pconfig_platform_table` from
    `kernel/pconfig/configs` through the static backend.
  - Acceptance: `pconfig_load()` finds a board config and `pconfig.ko` loads.
- [x] Define the PConfig ownership model.
  - PConfig owns its module lifetime and global config state.
  - PDM reads PConfig and removes only PDM-created devices on exit.
  - Acceptance: unloading `pdm.ko` does not invalidate a loaded `pconfig.ko`
    unexpectedly.
- [ ] Add a basic module load/unload smoke path.
  - Target order: `osal.ko`, `lpf_core.ko`, `pconfig.ko`, `hal.ko`, `pdm.ko`.
  - Acceptance: all modules load and unload cleanly on the target kernel with
    the selected defconfig.

## Phase 2: Finish PConfig As A Peripheral Registry

- [x] Generalize `pconfig_device_type_t` beyond MCU.
  - Keep `PCONFIG_DEVICE_TYPE_INVALID`.
  - LED is added with matching PConfig, PDM, PDI, and UAPI pieces.
- [x] Split PConfig per-peripheral types.
  - Current: `pconfig_mcu.h`.
  - LED follows the `pconfig_xxx.h` pattern.
- [x] Extend `pconfig_platform_config_t` for each supported peripheral.
  - Use `xxx_count` plus `xxx_array` consistently.
- [x] Generalize `pconfig_build_device_list()`.
  - It currently emits MCU and LED device entries.
  - Each enabled config entry should produce one `pconfig_device_config_t`.
- [x] Strengthen `pconfig_validate()`.
  - Validate required platform fields.
  - Validate per-peripheral arrays and counts.
  - LED now validates control type, brightness range, and PWM settings.
- [x] Update PConfig debug print output.
  - Print all configured peripheral groups, not only MCU.

## Phase 3: Finish PDM Driver Model

- [x] Keep the current `init` and `probe` split.
  - `init`: register global resources for a PDM driver type.
  - `probe`: create per-device instances from PConfig entries.
- [x] Audit PDM error-code conversion.
  - Avoid double-negating OSAL status values.
  - Kernel entry points should return Linux negative errno.
  - Internal OSAL-style APIs should return `OSAL_SUCCESS` or positive OSAL
    status codes consistently.
- [x] Define a reusable PDM peripheral driver template.
  - Driver object.
  - `lpf_driver_register`.
  - Per-device context table.
  - `probe`.
  - `remove`.
  - LPF Core-owned device removal.
  - Character-device registration if userspace access is needed.
- [x] Decide whether protocol support is global or per-driver.
  - LPF protocol is a common peripheral communication protocol library under
    `kernel/lpf/protocol`.
  - It has no independent module lifecycle; MCU, FPGA, and future peripheral
    drivers call it to package payloads into standard protocol frames and parse
    received raw frames back into device type, message type, and payload data.
- [x] Revisit PDM device removal.
  - PDM should remove devices before driver exit.
  - PConfig state should remain owned by PConfig.
  - Implemented with LPF Core: device nodes are removed before driver
    unregister/exit, and each bound device calls its driver's `remove`.
- [x] Add LED peripheral support using the established model.
  - Add LPF LED service implementation.
  - Add LPF LED chrdev implementation.
  - Add matching PConfig and PDI pieces.

## Phase 4: Finish PDI/UAPI Split

- [x] MCU UAPI lives in `uapi/lpf/lpf_mcu.h`.
- [x] MCU ioctl definitions are owned by `lpf_mcu.h`.
- [x] LED UAPI lives in `uapi/lpf/lpf_led.h`.
- [x] LED ioctl definitions are owned by `lpf_led.h`.
- [x] Userspace PDI aggregates per-peripheral headers from `pdi.h`.
- [x] Define UAPI rules for all future peripherals.
  - UAPI headers must be usable by both kernel and userspace.
  - Use fixed-width Linux UAPI types in ioctl structures.
  - Keep ioctl magic and command numbers per peripheral.
  - Avoid exposing kernel-only PDM/HAL/PConfig types through PDI.
- [x] Add version/ABI compatibility policy for each PDI peripheral.
  - Keep an ABI version field or `GET_INFO` command.
  - Define compatibility expectations for structure growth.
- [x] Decide whether `CONFIG_COMPAT` support stays local in each chrdev.
  - Current MCU chrdev forwards compat ioctl directly.
  - Future peripherals should follow the same rule or use a common helper.

## Phase 5: Complete Kernel OSAL Coverage

- [x] Add kernel OSAL memory allocation wrappers.
- [x] Add kernel OSAL string wrappers.
- [x] Add kernel OSAL mutex wrappers.
- [x] Add kernel OSAL thread wrappers.
- [x] Add kernel OSAL atomic wrappers.
- [x] Add kernel OSAL logging wrappers.
- [x] Add kernel OSAL CRC helpers.
- [x] Add kernel OSAL errno/status mapping.
- [x] Add kernel OSAL sleep/time header parity.
  - Provide `osal_msleep`, `osal_usleep`, `osal_sleep`,
    `osal_get_monotonic_time`, and related functions with user-compatible names.
- [x] Add kernel OSAL semaphore wrappers.
- [x] Add kernel OSAL condition or wait-event wrappers.
- [x] Add kernel OSAL rwlock wrappers.
- [x] Decide policy for file APIs in kernel OSAL.
  - Either wrap kernel file operations needed by HAL serial, or document them as
    HAL-owned kernel details.
- [x] Decide policy for socket APIs in kernel OSAL.
  - Either wrap kernel socket operations needed by HAL CAN, or document them as
    HAL-owned kernel details.
- [x] Decide policy for usercopy helpers.
  - If PDM character devices should not call `copy_to_user` directly, add
    `osal_copy_to_user` and `osal_copy_from_user`.
- [x] Decide policy for character-device helpers.
  - If repeated PDM chrdev code grows, add common OSAL or PDM-local helpers for
    misc device registration and ioctl dispatch.
- [x] Explicitly list unsupported user OSAL APIs.
  - Examples: process, environment, signal, POSIX shared memory, pty.
  - Unsupported APIs should either be absent from kernel OSAL or return
    `OSAL_ERR_NOT_SUPPORTED` with clear documentation.
- [x] Replace remaining direct kernel primitives in PDM/PConfig where OSAL
      equivalents are intended.
  - Keep direct Linux subsystem calls inside HAL where they are the hardware
    implementation detail.

## Phase 6: Remove Old Test Infrastructure

- [x] Decide whether `user/test_framework` is retained.
  - The ctest product has been removed.
  - If the test framework is also considered part of ctest, delete it.
- [x] Remove `CONFIG_TEST_FRAMEWORK` from root `Config.in` if deleted.
- [x] Remove `test_framework` from root `CMakeLists.txt` if deleted.
- [x] Remove test framework install/build files if deleted.
- [x] Update README and docs after deletion.
- [x] Leave a placeholder for future rewritten tests only if needed.

## Phase 7: Clean Build And Feature Selection

- [x] Remove unused `ccflags-$(CONFIG_...) += -DCONFIG_...` definitions from
      `kernel/Makefile`.
  - Keep Kbuild object selection such as `pdm-$(CONFIG_LPF_MCU_SERVICE)`.
- [x] Keep feature selection at object/list registration boundaries.
  - Kconfig selects objects.
  - Linked objects register themselves.
  - Business code should not branch on feature macros.
- [x] Revisit parameter macros.
  - Capacity parameters such as `CONFIG_LPF_MCU_MAX_DEVICES` may remain.
  - Prefer generated config constants or module parameters if runtime tuning is
    needed.
- [x] Remove or document transitional CMake kernel component logic.
  - Removed PDM/PConfig static/shared transitional Kconfig and CMake branches.
  - HAL CMake remains an interface target for host-side dependency metadata;
    kernel module output is produced by Kbuild.
- [x] Ensure defconfigs match the current module build model.
  - `kernel_x86_modules_defconfig` should enable only valid kernel module
    options.

## Phase 8: Documentation And Naming Cleanup

- [x] Update README to match the current kernel-module architecture.
- [x] Update `docs/KERNEL_USER_SPLIT.md`.
- [x] Update `docs/ARCHITECTURE.md`.
- [x] Remove outdated references to ctest and old userspace HAL/PDM behavior.
- [x] Document module load order and dependencies.
  - OSAL before PConfig/HAL.
  - OSAL before LPF Core.
  - LPF Core, PConfig, and HAL before PDM.
- [x] Document how to add a new peripheral.
  - PConfig type and platform entry.
  - LPF device type/capability mapping.
  - PDM service driver object and LPF Core registration.
  - LPF UAPI header and userspace wrapper.
  - Kbuild object selection.
- [x] Document the feature-selection rule.
  - Build/link selection is allowed.
  - Business-code feature `#ifdef CONFIG_*` should be avoided.

## Phase 9: Verification

- [x] Run formatting/static checks.
  - `git diff --check`
  - `./scripts/validate_configs.sh`
- [x] Build kernel modules.
  - `make kernel_x86_modules_defconfig`
  - `make modules`
- [x] Verify module artifact names.
  - `osal.ko`
  - `pconfig.ko`
  - `hal.ko`
  - `pdm.ko`
- [ ] Run module load/unload smoke test on a compatible kernel.
- [ ] Verify `/dev/lpf/mcuN` appears when MCU is configured.
- [ ] Verify each PDI ioctl returns expected status on configured and
      unconfigured devices.
- [ ] Add rewritten tests when the new test plan is ready.

## Update Rules

- Mark a checkbox only after implementation and basic verification are complete.
- Add a short note under an item if the implementation intentionally differs
  from this plan.
- Keep commits grouped by phase or by tightly related module changes.
- Do not mix documentation-only checklist updates with large code changes unless
  the checklist update describes the same change.

# Kernel Refactor Plan

This document tracks the remaining work for moving ES-Middleware HAL/PConfig/PDM
to the kernel-module architecture. Update the checkboxes as each part is
implemented and verified.

## Target Architecture

- HAL is a kernel-only module built as `hal.ko`.
- PConfig is a standalone kernel module built as `pconfig.ko`.
- PDM is a kernel module built as `pdm.ko`; peripheral drivers are linked into
  `pdm.ko` and registered through built-in driver registration.
- PDI remains a userspace library, but it only talks to per-peripheral character
  devices and UAPI headers.
- Each peripheral owns its own PConfig type, PDM driver, PDI UAPI header, PDI
  userspace wrapper, character device, and ioctl namespace.
- Feature selection is handled by Kbuild object selection and registration
  tables, not by business-code `#ifdef CONFIG_*` branches.
- OSAL hides kernel/userspace differences where equivalent semantics are
  practical. Unsupported APIs should be explicitly documented or return
  `OSAL_ERR_NOT_SUPPORTED`.

## Current Completed Work

- [x] Move HAL implementation to kernel-side `core/kernel/hal/src`.
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
- [x] Split PDI MCU UAPI into `core/uapi/pdi/pdi_mcu.h`.
- [x] Split userspace PDI MCU wrapper into `core/user/pdi/src/pdi_mcu.c`.
- [x] Add `/dev/pdm_mcu` character-device ioctl boundary.
- [x] Change PDM startup to initialize built-in drivers, load PConfig, iterate
      configured devices, and call matching driver `probe`.
- [x] Add PDM built-in driver registration through `pdm_driver_register`.
- [x] Keep PDM peripheral drivers linked into `pdm.ko` instead of adding one
      kernel module per peripheral.
- [x] Add HAL built-in initialization table for HAL subdrivers that need global
      module initialization.
- [x] Remove ctest product files from the current tree.
- [x] Create a tag for the last pure userspace implementation and merge the
      refactor branch back to `master`.

## Phase 1: Make The Current Kernel Modules Loadable

- [ ] Add a real product/platform PConfig source for the kernel module build.
  - `pconfig.ko` currently has only a weak empty `g_pconfig_platform_table`.
  - The selected platform table must be linked into `pconfig.ko` by Kbuild.
  - Acceptance: `pconfig_load()` finds a board config and `pconfig.ko` loads.
- [ ] Define the PConfig ownership model.
  - PConfig should own its module lifetime and global config state.
  - PDM should not unload PConfig-owned global state unless a reference-counted
    API is introduced.
  - Acceptance: unloading `pdm.ko` does not invalidate a loaded `pconfig.ko`
    unexpectedly.
- [ ] Add a basic module load/unload smoke path.
  - Target order: `osal.ko`, `pconfig.ko`, `hal.ko`, `pdm.ko`.
  - Acceptance: all modules load and unload cleanly on the target kernel with
    the selected defconfig.

## Phase 2: Finish PConfig As A Peripheral Registry

- [ ] Generalize `pconfig_device_type_t` beyond MCU.
  - Keep `PCONFIG_DEVICE_TYPE_INVALID`.
  - Add new device types only with their matching config headers and PDM/PDI
    users.
- [ ] Split PConfig per-peripheral types.
  - Current: `pconfig_mcu.h`.
  - Future pattern: `pconfig_xxx.h` for each peripheral.
- [ ] Extend `pconfig_platform_config_t` for each supported peripheral.
  - Use `xxx_count` plus `xxx_array` consistently.
- [ ] Generalize `pconfig_build_device_list()`.
  - It currently emits only MCU device entries.
  - Each enabled config entry should produce one `pconfig_device_config_t`.
- [ ] Strengthen `pconfig_validate()`.
  - Validate required platform fields.
  - Validate per-peripheral arrays and counts.
  - Validate transport-specific settings used by PDM.
- [ ] Update PConfig debug print output.
  - Print all configured peripheral groups, not only MCU.

## Phase 3: Finish PDM Driver Model

- [ ] Keep the current `init` and `probe` split.
  - `init`: register global resources for a PDM driver type.
  - `probe`: create per-device instances from PConfig entries.
- [ ] Audit PDM error-code conversion.
  - Avoid double-negating OSAL status values.
  - Kernel entry points should return Linux negative errno.
  - Internal OSAL-style APIs should return `OSAL_SUCCESS` or positive OSAL
    status codes consistently.
- [ ] Define a reusable PDM peripheral driver template.
  - Driver object.
  - `pdm_driver_register`.
  - Per-device context table.
  - `probe`.
  - `remove_all`.
  - Character-device registration if userspace access is needed.
- [ ] Decide whether protocol support is global or per-driver.
  - Current MCU driver initializes the PDM protocol globally.
  - If future peripherals share the protocol layer, move ownership to PDM core
    or introduce protocol reference counting.
- [ ] Revisit PDM device removal.
  - PDM should remove devices before driver exit.
  - PConfig state should remain owned by PConfig.
- [ ] Add future peripheral support using the established model.
  - Add `pdm_xxx` implementation.
  - Add `pdm_xxx_chrdev.c` if userspace control is required.
  - Add matching PConfig and PDI pieces.

## Phase 4: Finish PDI/UAPI Split

- [x] MCU UAPI lives in `core/uapi/pdi/pdi_mcu.h`.
- [x] MCU ioctl definitions are owned by `pdi_mcu.h`.
- [x] Userspace PDI aggregates per-peripheral headers from `pdi.h`.
- [ ] Define UAPI rules for all future peripherals.
  - UAPI headers must be usable by both kernel and userspace.
  - Use fixed-width Linux UAPI types in ioctl structures.
  - Keep ioctl magic and command numbers per peripheral.
  - Avoid exposing kernel-only PDM/HAL/PConfig types through PDI.
- [ ] Add version/ABI compatibility policy for each PDI peripheral.
  - Keep an ABI version field or `GET_INFO` command.
  - Define compatibility expectations for structure growth.
- [ ] Decide whether `CONFIG_COMPAT` support stays local in each chrdev.
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
- [ ] Add kernel OSAL sleep/time header parity.
  - Provide `osal_msleep`, `osal_usleep`, `osal_sleep`,
    `osal_get_monotonic_time`, and related functions with user-compatible names.
- [ ] Add kernel OSAL semaphore wrappers.
- [ ] Add kernel OSAL condition or wait-event wrappers.
- [ ] Add kernel OSAL rwlock wrappers.
- [ ] Decide policy for file APIs in kernel OSAL.
  - Either wrap kernel file operations needed by HAL serial, or document them as
    HAL-owned kernel details.
- [ ] Decide policy for socket APIs in kernel OSAL.
  - Either wrap kernel socket operations needed by HAL CAN, or document them as
    HAL-owned kernel details.
- [ ] Decide policy for usercopy helpers.
  - If PDM character devices should not call `copy_to_user` directly, add
    `osal_copy_to_user` and `osal_copy_from_user`.
- [ ] Decide policy for character-device helpers.
  - If repeated PDM chrdev code grows, add common OSAL or PDM-local helpers for
    misc device registration and ioctl dispatch.
- [ ] Explicitly list unsupported user OSAL APIs.
  - Examples: process, environment, signal, POSIX shared memory, pty.
  - Unsupported APIs should either be absent from kernel OSAL or return
    `OSAL_ERR_NOT_SUPPORTED` with clear documentation.
- [ ] Replace remaining direct kernel primitives in PDM/PConfig where OSAL
      equivalents are intended.
  - Keep direct Linux subsystem calls inside HAL where they are the hardware
    implementation detail.

## Phase 6: Remove Old Test Infrastructure

- [ ] Decide whether `core/user/test_framework` is retained.
  - The ctest product has been removed.
  - If the test framework is also considered part of ctest, delete it.
- [ ] Remove `CONFIG_TEST_FRAMEWORK` from `core/Config.in` if deleted.
- [ ] Remove `test_framework` from root `CMakeLists.txt` if deleted.
- [ ] Remove test framework install/build files if deleted.
- [ ] Update README and docs after deletion.
- [ ] Leave a placeholder for future rewritten tests only if needed.

## Phase 7: Clean Build And Feature Selection

- [ ] Remove unused `ccflags-$(CONFIG_...) += -DCONFIG_...` definitions from
      `core/kernel/Makefile`.
  - Keep Kbuild object selection such as `pdm-$(CONFIG_PDM_MCU_SUPPORT)`.
- [ ] Keep feature selection at object/list registration boundaries.
  - Kconfig selects objects.
  - Linked objects register themselves.
  - Business code should not branch on feature macros.
- [ ] Revisit parameter macros.
  - Capacity parameters such as `CONFIG_PDM_MCU_MAX_DEVICES` may remain.
  - Prefer generated config constants or module parameters if runtime tuning is
    needed.
- [ ] Remove or document transitional CMake kernel component logic.
  - `core/kernel/pdm/CMakeLists.txt` and `pconfig/CMakeLists.txt` still contain
    static/shared library options.
  - HAL CMake is currently an interface target only.
- [ ] Ensure defconfigs match the final module build model.
  - `kernel_x86_modules_defconfig` should enable only valid kernel module
    options.

## Phase 8: Documentation And Naming Cleanup

- [ ] Update README to match the current kernel-module architecture.
- [ ] Update `docs/KERNEL_USER_SPLIT.md`.
- [ ] Update `docs/ARCHITECTURE.md`.
- [ ] Remove outdated references to ctest and old userspace HAL/PDM behavior.
- [ ] Document module load order and dependencies.
  - OSAL before PConfig/HAL.
  - PConfig and HAL before PDM.
- [ ] Document how to add a new peripheral.
  - PConfig type and platform entry.
  - PDM driver object and registration.
  - PDI UAPI header and userspace wrapper.
  - Kbuild object selection.
- [ ] Document the feature-selection rule.
  - Build/link selection is allowed.
  - Business-code feature `#ifdef CONFIG_*` should be avoided.

## Phase 9: Verification

- [ ] Run formatting/static checks.
  - `git diff --check`
  - `./scripts/validate_configs.sh`
- [ ] Build kernel modules.
  - `make kernel_x86_modules_defconfig`
  - `make modules`
- [ ] Verify module artifact names.
  - `osal.ko`
  - `pconfig.ko`
  - `hal.ko`
  - `pdm.ko`
- [ ] Run module load/unload smoke test on a compatible kernel.
- [ ] Verify `/dev/pdm_mcu` appears when MCU is configured.
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

# LPF to PDM Renaming Plan

## Phase 1: Global Renaming (LPF → PDM)

This document tracks the systematic renaming of all LPF symbols, files, and identifiers to PDM.

### Renaming Strategy

1. **Bottom-up approach**: Start with leaf identifiers, move to aggregate structures
2. **Verify compilation** after each major category
3. **Commit incrementally** to enable easy rollback
4. **Test functionality** after completion

### Symbol Mapping Table

| Category | Old (LPF) | New (PDM) |
|----------|-----------|-----------|
| **Module Names** | | |
| Kernel module | `lpf_core.ko` | `pdm_core.ko` |
| Config module | `lpf_configs.ko` | `pdm_configs.ko` |
| OSAL module | `osal.ko` | `osal.ko` (keep) |
| **Directory Names** | | |
| Root | `lpf/` | `pdm/` |
| Core | `lpf-core/` | `pdm-core/` |
| Configs | `lpf-configs/` | `pdm-configs/` |
| **Core Types** | | |
| Device | `lpf_device_t` | `pdm_device_t` |
| Driver | `lpf_driver_t` | `pdm_driver_t` |
| Device config | `lpf_device_config_t` | `pdm_device_config_t` |
| Device handle | `lpf_device_handle_t` | `pdm_device_handle_t` |
| Device type enum | `lpf_device_type_t` | `pdm_device_type_t` |
| Capability | `lpf_capability_t` | `pdm_capability_t` |
| **Function Prefixes** | | |
| Core functions | `lpf_*` | `pdm_*` |
| Device functions | `lpf_device_*` | `pdm_device_*` |
| Driver functions | `lpf_driver_*` | `pdm_driver_*` |
| **Macro Prefixes** | | |
| General | `LPF_*` | `PDM_*` |
| Device types | `LPF_DEVICE_TYPE_*` | `PDM_DEVICE_TYPE_*` |
| Capabilities | `LPF_DEVICE_CAP_*` | `PDM_DEVICE_CAP_*` |
| **Device Nodes** | | |
| Control node | `/dev/lpf_ctl` | `/dev/pdm_ctl` |
| Instance nodes | `/dev/lpf/mcu0` | `/dev/pdm/mcu0` |
| Instance nodes | `/dev/lpf/led0` | `/dev/pdm/led0` |
| **Sysfs/Procfs** | | |
| Proc directory | `/proc/lpf/` | `/proc/pdm/` |
| Debugfs directory | `/sys/kernel/debug/lpf/` | `/sys/kernel/debug/pdm/` |
| **Include Paths** | | |
| Public headers | `lpf/core/` | `pdm/core/` |
| Config headers | `lpf/config/` | `pdm/config/` |
| HW headers | `lpf/hw/` | `pdm/hw/` |
| **UAPI** | | |
| Control header | `lpf/lpf_ctl.h` | `pdm/pdm_ctl.h` |
| MCU header | `lpf/lpf_mcu.h` | `pdm/pdm_mcu.h` |
| LED header | `lpf/lpf_led.h` | `pdm/pdm_led.h` |
| **Userspace** | | |
| PDI library | `libpdi.so` | `libpdi.so` (keep name) |
| PDI functions | `pdi_*` | `pdi_*` (keep, already good) |

### Execution Plan

#### Step 1: Rename directories
- `kernel/lpf-core/` → `kernel/pdm-core/`
- `kernel/lpf-configs/` → `kernel/pdm-configs/`
- `kernel/include/lpf/` → `kernel/include/pdm/`
- `uapi/lpf/` → `uapi/pdm/`

#### Step 2: Rename files
- All `lpf_*.c` → `pdm_*.c`
- All `lpf_*.h` → `pdm_*.h`
- Update `#include` paths

#### Step 3: Rename symbols in source
- Type definitions: `lpf_*_t` → `pdm_*_t`
- Function names: `lpf_*` → `pdm_*`
- Macros: `LPF_*` → `PDM_*`
- Comments and strings

#### Step 4: Update build system
- Makefile module names
- Kconfig options: `CONFIG_LPF_*` → `CONFIG_PDM_*`
- Config.in entries

#### Step 5: Update documentation
- README.md
- All docs/*.md files
- CLAUDE.md

#### Step 6: Verify and test
- Compile all configurations
- Test on target hardware
- Verify device nodes created
- Run test suite

### Implementation Notes

1. **OSAL stays as-is**: It's a generic OS abstraction, name still makes sense
2. **PDI stays as-is**: "Peripheral Driver Interface" is generic
3. **Keep git history**: Use `git mv` for file renames
4. **Incremental commits**: One commit per step
5. **Tag before rename**: Create `before-pdm-rename` tag

### Validation Checklist

- [ ] All files renamed
- [ ] All `#include` paths updated
- [ ] All symbols renamed
- [ ] Kconfig compiles
- [ ] Kernel modules compile
- [ ] Userspace libraries compile
- [ ] Device nodes created correctly
- [ ] Test suite passes
- [ ] Documentation updated

---

**Status**: Planning complete, ready to execute  
**Branch**: `refactor/lpf-to-pdm-bus`  
**Date**: 2026-06-22

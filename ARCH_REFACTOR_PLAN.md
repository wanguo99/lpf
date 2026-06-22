# PDM Architecture Refactor - From Pseudo-Bus to Linux Bus

## Status

**Current Phase**: ✅ Phase 1 Complete - New bus implementation added  
**Date Started**: 2026-06-22  
**Branch**: `refactor/lpf-to-pdm-bus`

## Overview

This document tracks the refactoring of PDM from a custom pseudo-bus to a standard Linux `bus_type` implementation, based on the reference from `/home/wanguo/Github/pdm`.

## Completed Work

### Phase 0: Global Renaming ✅ DONE
- [x] LPF → PDM renaming (3 commits, 378 files)
- [x] Compilation verified
- [x] All symbols updated

### Phase 1: New Bus Implementation ✅ DONE  
- [x] Created `kernel/pdm-core/bus/pdm_bus.c`
- [x] Created `kernel/pdm-core/bus/pdm_device.c`
- [x] Created `kernel/include/pdm/core/pdm_bus.h`
- [x] Created `kernel/include/pdm/core/pdm_device_new.h`
- [x] Committed (90b6449)

## Remaining Work

### Phase 2: Build System Integration (Next)
- [ ] Update `kernel/pdm-core/Makefile` to include `bus/`
- [ ] Add conditional compilation: `CONFIG_PDM_NEW_BUS`
- [ ] Keep old code active by default during transition
- [ ] Verify new bus compiles alongside old code

### Phase 3: Core Module Update
- [ ] Update `pdm_core.c` to initialize new bus
- [ ] Add `pdm_bus_init()` call in module_init
- [ ] Add `pdm_bus_exit()` call in module_exit
- [ ] Keep old core code for comparison

### Phase 4: Bus Controller (Device Creation from DT)
- [ ] Create `kernel/pdm-core/bus/pdm_bus_controller.c`
- [ ] Implement DT traversal and device creation
- [ ] Register as platform_driver for "vendor,pdm-bus"
- [ ] Create PDM devices for each DT child node

### Phase 5: Peripheral Driver Migration
- [ ] Migrate MCU driver to new bus
  - [ ] Create `of_device_id` table
  - [ ] Update driver structure to use `pdm_driver`
  - [ ] Modify probe signature: `int probe(struct pdm_device *)`
  - [ ] Test MCU functionality
- [ ] Migrate LED driver to new bus
  - [ ] Create `of_device_id` table  
  - [ ] Update driver structure
  - [ ] Modify probe/remove
  - [ ] Test LED functionality

### Phase 6: Configuration System Removal
- [ ] Remove static C configuration (`pdm_configs.ko`)
- [ ] Remove config backend selection logic
- [ ] Update documentation for DT-only config
- [ ] Create DT binding documentation

### Phase 7: Cleanup Old Code
- [ ] Remove old `pdm_core.c` pseudo-bus code
- [ ] Remove `pdm_driver_register()` (old API)
- [ ] Remove `pdm_device_register()` (old API)
- [ ] Remove `pdm_device_type_t` enum
- [ ] Update all internal references

### Phase 8: Testing and Validation
- [ ] Compile all configurations
- [ ] Test on x86 mock modules
- [ ] Test on real hardware (i.MX6ULL)
- [ ] Verify `/sys/bus/pdm/` visibility
- [ ] Verify device matching works
- [ ] Run test suite

## Architecture Comparison

### Old (Pseudo-Bus)
```c
// Custom device model
static LIST_HEAD(g_pdm_drivers);
static LIST_HEAD(g_pdm_devices);

// Custom registration
pdm_driver_register(&mcu_driver);
pdm_device_register(&device_config);

// Manual matching by type enum
if (device->type == PDM_DEVICE_TYPE_MCU)
    driver = find_driver(PDM_DEVICE_TYPE_MCU);
```

### New (Linux Bus)
```c
// Standard Linux bus_type
struct bus_type pdm_bus_type = {
    .name = "pdm",
    .match = pdm_bus_device_match,  // uses of_driver_match_device()
    .probe = pdm_bus_device_probe,
};

// Standard driver registration
static struct pdm_driver mcu_driver = {
    .driver = {
        .name = "pdm-mcu",
        .of_match_table = mcu_of_match,
    },
    .probe = mcu_probe,
};
module_pdm_driver(mcu_driver);

// Automatic matching by Device Tree compatible
```

## Key Benefits

### For Developers
- ✅ Standard Linux driver model (less learning curve)
- ✅ Compatible with kernel documentation
- ✅ Can use standard debugging tools
- ✅ Easier code review and maintenance

### For System Integration
- ✅ Devices visible in `/sys/bus/pdm/`
- ✅ Works with `udevadm`, `lsmod`, standard tools
- ✅ Automatic device lifecycle management
- ✅ Support for runtime PM, suspend/resume (future)

### For Configuration
- ✅ Pure Device Tree configuration
- ✅ Device Tree overlays for product variants
- ✅ No need to recompile for different products
- ✅ Standard DT bindings

## Device Tree Example

### Before (Static C Config)
```c
// kernel/pdm-configs/products/imx6ull/...
static pdm_config_mcu_entry_t imx6ull_mcu[] = {
    {
        .config.interface = PDM_CONFIG_MCU_INTERFACE_UART,
        .config.uart_device = "/dev/ttyS2",
        .config.baudrate = 115200,
    },
};
```

### After (Device Tree)
```dts
/ {
    pdm_bus: pdm@0 {
        compatible = "vendor,pdm-bus";
        #address-cells = <1>;
        #size-cells = <0>;
        
        mcu0: mcu@0 {
            compatible = "vendor,pdm-mcu-uart";
            reg = <0>;
            uart-device = "/dev/ttyS2";
            baudrate = <115200>;
            status = "okay";
        };
        
        led0: led@0 {
            compatible = "vendor,pdm-led-gpio";
            reg = <0>;
            gpios = <&gpio1 5 GPIO_ACTIVE_HIGH>;
            status = "okay";
        };
    };
};
```

## Migration Strategy

### Parallel Implementation (Current Approach)
1. ✅ Keep old code active
2. ✅ Add new code alongside
3. ⏳ Use Kconfig to switch: `CONFIG_PDM_NEW_BUS`
4. ⏳ Test both implementations in parallel
5. ⏳ Once new bus validated, remove old code

### Kconfig Option
```kconfig
config PDM_NEW_BUS
    bool "Use new Linux bus_type implementation (EXPERIMENTAL)"
    depends on PDM_CORE
    default n
    help
      Enable the new standard Linux bus_type implementation.
      This is experimental. Leave disabled for stable operation.
```

## Timeline Estimate

| Phase | Description | Estimated Time | Status |
|-------|-------------|---------------|---------|
| 0 | Global Renaming | 2-4 hours | ✅ Done |
| 1 | New Bus Implementation | 2 hours | ✅ Done |
| 2 | Build System | 1 hour | ⏳ Next |
| 3 | Core Module Update | 2 hours | ⏳ |
| 4 | Bus Controller | 3 hours | ⏳ |
| 5 | Peripheral Migration | 8-12 hours | ⏳ |
| 6 | Config Removal | 2 hours | ⏳ |
| 7 | Cleanup | 4 hours | ⏳ |
| 8 | Testing | 8 hours | ⏳ |
| **Total** | | **32-40 hours** | **6% done** |

## Risk Assessment

### High Risk Items
- ❗ Breaking existing userspace applications
  - **Mitigation**: Keep UAPI unchanged, only change kernel internal
- ❗ Device Tree binding changes
  - **Mitigation**: Provide migration guide and examples

### Medium Risk Items
- ⚠️ Compilation errors during transition
  - **Mitigation**: Use conditional compilation
- ⚠️ Peripheral drivers not working after migration
  - **Mitigation**: Test each driver individually

### Low Risk Items
- ℹ️ Performance regression
  - **Note**: Linux driver model is well-optimized

## Testing Checklist

- [ ] All configurations compile
- [ ] Mock modules load successfully  
- [ ] `/sys/bus/pdm/` directory exists
- [ ] Devices appear in `/sys/bus/pdm/devices/`
- [ ] Drivers appear in `/sys/bus/pdm/drivers/`
- [ ] Device-driver matching works
- [ ] Character devices `/dev/pdm/*` created
- [ ] ioctl commands work
- [ ] sysfs attributes readable
- [ ] procfs entries exist
- [ ] debugfs commands work
- [ ] Module unload clean
- [ ] No memory leaks (checked with kmemleak)

## Reference Files

- `/home/wanguo/Github/pdm/src/core/pdm_bus.c` - Reference bus implementation
- `/home/wanguo/Github/pdm/include/core/pdm_adapter.h` - Adapter pattern
- Linux kernel: `drivers/base/bus.c` - Standard bus implementation
- Linux kernel: `Documentation/driver-api/driver-model/` - Driver model docs

## Next Step

**Immediate action**: Update build system to compile new bus code alongside old code.

```bash
# Edit kernel/pdm-core/Makefile
# Add: obj-$(CONFIG_PDM_NEW_BUS) += bus/
```

---

**Last Updated**: 2026-06-22  
**Current Commit**: 90b6449  
**Progress**: Phase 1 Complete (6%)

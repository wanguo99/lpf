# LPF Kernel Compatibility Policy

## Supported Baseline

LPF kernel modules target Linux kernel `5.10` and newer.

When changing kernel-facing code, validate at least:

- `make kernel_x86_mock_modules_defconfig`
- `make modules`
- `make tests`
- `make kernel-matrix`

Broader CI should cover one kernel from each actively supported product family.
If a product needs a kernel older than `5.10`, the required fallback must be
implemented in LPF compat wrappers before the product enables that target.

## Rules

- Linux version and feature checks belong in `kernel/include/lpf/compat/lpf_compat_*`
  headers or `kernel/lpf/compat/` source files.
- Peripheral services, LPF HW, transports, runtime config, and LPF Core business
  logic must not add direct `LINUX_VERSION_CODE` checks.
- Use feature-style helpers such as `LPF_KERNEL_HAS_SYSFS_EMIT` instead of
  scattering raw version comparisons.
- Prefer small compat wrappers around API shape differences instead of
  duplicating service logic.
- If a compatibility fallback cannot preserve behavior, fail the build with a
  clear `#error` and document the unsupported kernel range here.

## Current Feature Gates

`kernel/include/lpf/compat/lpf_compat_features.h` defines the supported kernel baseline
and currently detected feature gates:

- `LPF_KERNEL_HAS_PROC_OPS`
- `LPF_KERNEL_HAS_SYSFS_EMIT`

`kernel/include/lpf/compat/lpf_compat_sysfs.h` wraps sysfs text emission through
`lpf_compat_sysfs_emit()`. New sysfs attributes should use this wrapper rather
than calling `sysfs_emit()` directly.

Procfs currently requires `struct proc_ops`, which is available within the
supported baseline. Lowering the baseline below that point requires adding a
procfs file-operations fallback in the compat layer first.

Debugfs usage is currently limited to stable `debugfs_create_dir`,
`debugfs_create_file`, `debugfs_remove`, and `debugfs_remove_recursive` entry
points within the supported baseline.

## Matrix Policy

`make kernel-matrix` is the local and CI entry point for compiling LPF kernel
modules against selected kernel build trees. By default it builds the mock
module preset against `KERNEL_SRC`. To cover multiple kernels, pass a
space-separated list:

```bash
make kernel-matrix KERNEL_SRC_LIST="/path/to/linux-5.10/build /path/to/linux-6.6/build"
```

The target writes each kernel's artifacts under `_build/kernel-matrix/` so
results from different kernels do not overwrite each other. The default
defconfig is `kernel_x86_mock_modules_defconfig`; override it with
`LPF_KERNEL_MATRIX_DEFCONFIG=<name>` when validating another module preset.

The target CI matrix should include:

- the oldest supported product kernel,
- the current development workstation kernel,
- one current long-term maintenance kernel used by deployed products.

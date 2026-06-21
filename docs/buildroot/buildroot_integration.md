# Buildroot Integration

This directory contains a Buildroot br2-external package skeleton for LPF.
The package builds LPF userspace libraries with CMake and LPF kernel modules
with the selected Buildroot Linux kernel tree.

## Layout

Copy `package/lpf` into a Buildroot external tree:

```text
buildroot-external-tree/
├── Config.in
├── external.mk
└── package/
    └── lpf/
        ├── Config.in
        ├── lpf.mk
        └── local.mk.example
```

The external tree top-level `Config.in` should include:

```make
source "$BR2_EXTERNAL_<NAME>_PATH/package/lpf/Config.in"
```

The external tree `external.mk` should include:

```make
include $(sort $(wildcard $(BR2_EXTERNAL_<NAME>_PATH)/package/*/*.mk))
```

Replace `<NAME>` with the name from `external.desc`, for example `IMX6ULL`.

## Source Selection

The package defaults to the canonical LPF git repository. For local product
integration, set `LPF_OVERRIDE_SRCDIR` in the board override file referenced by
`BR2_PACKAGE_OVERRIDE_FILE`:

```make
LPF_OVERRIDE_SRCDIR = $(TOPDIR)/../LPF
```

Buildroot will then use the local LPF source tree and skip the git fetch.

## LPF Defconfig

`BR2_PACKAGE_LPF_DEFCONFIG` must point to an LPF defconfig available in the LPF
source tree. Product trees should use a product-specific LPF target. For the
i.MX6ULL EVK bring-up tree, use:

```text
imx6ull_modules_defconfig
```

Do not use the Ubuntu x86 LPF defconfigs for ARM product builds; those targets
are development presets.

## Install Modes

- Target install: userspace libraries are installed under `/usr`, and enabled
  kernel modules are installed under `/lib/modules/<kernel>/extra/lpf/`.
- Staging install: enable `BR2_PACKAGE_LPF_INSTALL_HEADERS` to install
  development headers and libraries to staging for packages that depend on LPF.

No product-specific init script is installed by default. Product services,
udev/devtmpfs policy, and module load order should be owned by the product
external tree.

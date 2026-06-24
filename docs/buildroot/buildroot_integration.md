# Buildroot Integration

This directory contains a Buildroot br2-external package skeleton for PDM.
The package builds PDM userspace libraries with CMake and PDM kernel modules
with the selected Buildroot Linux kernel tree.

## Layout

Copy `package/pdm` into a Buildroot external tree:

```text
buildroot-external-tree/
├── Config.in
├── external.mk
└── package/
    └── pdm/
        ├── Config.in
        ├── pdm.mk
        └── local.mk.example
```

The external tree top-level `Config.in` should include:

```make
source "$BR2_EXTERNAL_<NAME>_PATH/package/pdm/Config.in"
```

The external tree `external.mk` should include:

```make
include $(sort $(wildcard $(BR2_EXTERNAL_<NAME>_PATH)/package/*/*.mk))
```

Replace `<NAME>` with the name from `external.desc`, for example `IMX6ULL`.

## Source Selection

The package defaults to the canonical PDM git repository. For local product
integration, set `PDM_OVERRIDE_SRCDIR` in the board override file referenced by
`BR2_PACKAGE_OVERRIDE_FILE`:

```make
PDM_OVERRIDE_SRCDIR = $(TOPDIR)/../PDM
```

Buildroot will then use the local PDM source tree and skip the git fetch.

## PDM Defconfig

`BR2_PACKAGE_PDM_DEFCONFIG` must point to an PDM defconfig available in the PDM
source tree. Product trees should use a product-specific PDM target. For the
i.MX6ULL bring-up tree, use:

```text
imx6ull_modules_defconfig
```

Do not use the Ubuntu x86 PDM defconfigs for ARM product builds; those targets
are development presets.

## Install Modes

- Target install: userspace libraries are installed under `/usr`, and enabled
  kernel modules are installed under `/lib/modules/<kernel>/extra/pdm/`.
- Staging install: enable `BR2_PACKAGE_PDM_INSTALL_HEADERS` to install
  development headers and libraries to staging for packages that depend on PDM.

No product-specific init script is installed by default. Product services,
udev/devtmpfs policy, and module load order should be owned by the product
external tree.

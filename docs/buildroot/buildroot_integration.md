# Buildroot Integration

This directory contains a Buildroot br2-external package skeleton for PAF.
The package builds PAF userspace components with CMake and PDM kernel modules
with the selected Buildroot Linux kernel tree.

## Layout

Copy `package/paf` into a Buildroot external tree:

```text
buildroot-external-tree/
├── Config.in
├── external.mk
└── package/
    └── paf/
        ├── Config.in
        ├── paf.mk
        └── local.mk.example
```

The external tree top-level `Config.in` should include:

```make
source "$BR2_EXTERNAL_<NAME>_PATH/package/paf/Config.in"
```

The external tree `external.mk` should include:

```make
include $(sort $(wildcard $(BR2_EXTERNAL_<NAME>_PATH)/package/*/*.mk))
```

Replace `<NAME>` with the name from `external.desc`, for example `IMX6ULL`.

## Source Selection

The package defaults to the canonical PAF git repository. For local product
integration, set `PAF_OVERRIDE_SRCDIR` in the board override file referenced by
`BR2_PACKAGE_OVERRIDE_FILE`:

```make
PAF_OVERRIDE_SRCDIR = $(TOPDIR)/../PAF
```

Buildroot will then use the local PAF source tree and skip the git fetch.

## PAF Defconfig

`BR2_PACKAGE_PAF_DEFCONFIG` must point to a PAF defconfig available in the PAF
source tree. Product trees should use a product-specific PAF target. For the
i.MX6ULL bring-up tree, use:

```text
imx6ull_modules_defconfig
```

Do not use the Ubuntu x86 PAF defconfigs for ARM product builds; those targets
are development presets.

## Install Modes

- Target install: userspace libraries are installed under `/usr`, and enabled
  kernel modules are installed under `/lib/modules/<kernel>/extra/pdm/`.
- Staging install: enable `BR2_PACKAGE_PAF_INSTALL_HEADERS` to install
  development headers under `/usr/include/pdm` and libraries to staging for
  packages that depend on PAF.

No product-specific init script is installed by default. Product services,
udev/devtmpfs policy, and module load order should be owned by the product
external tree.

################################################################################
#
# lpf
#
################################################################################

# Default: use the canonical LPF git repository.
# Product trees can set LPF_OVERRIDE_SRCDIR in BR2_PACKAGE_OVERRIDE_FILE for
# local development or board integration testing.
LPF_VERSION = v1.0.0
LPF_SITE = ssh://gitea@192.168.18.254:4022/CSPD/LPF.git
LPF_SITE_METHOD = git
LPF_LICENSE = Proprietary
LPF_INSTALL_TARGET = YES
LPF_ADD_TOOLCHAIN_DEPENDENCY = NO

# Dependencies - Kconfig + CMake/Kbuild build system
LPF_DEPENDENCIES = host-pkgconf host-cmake host-flex host-bison linux

# Kconfig configuration to use (from Buildroot config)
LPF_KCONFIG_DEFCONFIG = $(call qstrip,$(BR2_PACKAGE_LPF_DEFCONFIG))

# Build type from Buildroot configuration
LPF_BUILD_TYPE = $(call qstrip,$(BR2_PACKAGE_LPF_BUILD_TYPE))

# Build in-tree for Buildroot (output/build/lpf/_build)
LPF_BUILD_OUTPUT = $(@D)/_build
LPF_MODULES_OUTPUT = $(@D)/_build/modules
LPF_MODULE_EXTRA_DIR = extra/lpf

LPF_MAKE_OPTS = \
	BUILD_DIR="$(LPF_BUILD_OUTPUT)" \
	MODULES_BUILD_DIR="$(LPF_MODULES_OUTPUT)" \
	BR2_EXTERNAL=1 \
	KERNEL_SRC="$(LINUX_DIR)" \
	ARCH="$(KERNEL_ARCH)" \
	CROSS_COMPILE="$(TARGET_CROSS)" \
	CMAKE_BUILD_TYPE="$(LPF_BUILD_TYPE)" \
	CMAKE_TOOLCHAIN_FILE="$(HOST_DIR)/share/buildroot/toolchainfile.cmake" \
	CMAKE_EXTRA_FLAGS='-DCMAKE_C_COMPILER="$(TARGET_CC)" -DCMAKE_C_FLAGS="$(TARGET_CFLAGS)" -DCMAKE_EXE_LINKER_FLAGS="$(TARGET_LDFLAGS)"' \
	CMAKE_INSTALL_PREFIX="/usr"

# Configure using make (kernel-style interface)
# This loads the defconfig and generates .config + autoconf.h
define LPF_CONFIGURE_CMDS
	@test -n "$(LPF_KCONFIG_DEFCONFIG)" || { \
		echo "LPF: BR2_PACKAGE_LPF_DEFCONFIG must be set"; \
		exit 1; \
	}
	@echo "LPF: Loading defconfig $(LPF_KCONFIG_DEFCONFIG)"
	$(MAKE) -C $(@D) $(LPF_MAKE_OPTS) $(LPF_KCONFIG_DEFCONFIG)
	@echo "LPF: Configuration loaded successfully"
endef

# Build userspace libraries and kernel modules.
define LPF_BUILD_CMDS
	@echo "LPF: Building with configuration $(LPF_KCONFIG_DEFCONFIG)"
	rm -f "$(LPF_BUILD_OUTPUT)/CMakeCache.txt"
	$(TARGET_MAKE_ENV) $(MAKE) -C $(@D) $(LPF_MAKE_OPTS) all
endef

# Clean build artifacts (keep configuration)
define LPF_CLEAN_CMDS
	@echo "LPF: Cleaning build artifacts"
	-$(MAKE) -C $(@D) clean
endef

# Complete clean (remove configuration and build directory)
define LPF_DISTCLEAN_CMDS
	@echo "LPF: Complete clean"
	-$(MAKE) -C $(@D) distclean
endef

# Install userspace libraries and kernel modules to target.
define LPF_INSTALL_TARGET_CMDS
	@echo "LPF: Installing to target"
	$(MAKE) -C $(@D) \
		BUILD_DIR="$(LPF_BUILD_OUTPUT)" \
		CMAKE_INSTALL_PREFIX="/usr" \
		install DESTDIR=$(TARGET_DIR)
	kernel_release="$(LINUX_VERSION_PROBED)"; \
	if [ -z "$$kernel_release" ]; then \
		kernel_release="$$($(MAKE) -C "$(LINUX_DIR)" \
			ARCH="$(KERNEL_ARCH)" \
			CROSS_COMPILE="$(TARGET_CROSS)" \
			--no-print-directory -s kernelrelease)"; \
	fi; \
	module_install_dir="$(TARGET_DIR)/lib/modules/$$kernel_release/$(LPF_MODULE_EXTRA_DIR)"; \
	$(INSTALL) -d "$$module_install_dir"; \
	modules_found=0; \
	for module in "$(LPF_MODULES_OUTPUT)"/*.ko; do \
		[ -e "$$module" ] || continue; \
		modules_found=1; \
		$(INSTALL) -m 0644 "$$module" "$$module_install_dir/$$(basename "$$module")"; \
	done; \
	if [ "$$modules_found" -eq 0 ]; then \
		echo "LPF: no kernel modules found in $(LPF_MODULES_OUTPUT)"; \
		exit 1; \
	fi
	@echo "LPF: Target installation complete"
endef

# Optional: Install development headers and userspace libraries to staging.
ifeq ($(BR2_PACKAGE_LPF_INSTALL_HEADERS),y)
LPF_INSTALL_STAGING = YES
define LPF_INSTALL_STAGING_CMDS
	@echo "LPF: Installing libraries to staging"
	$(MAKE) -C $(@D) \
		BUILD_DIR="$(LPF_BUILD_OUTPUT)" \
		CMAKE_INSTALL_PREFIX="/usr" \
		install DESTDIR=$(STAGING_DIR)
	@echo "LPF: Installing development headers to staging"
	$(MAKE) -C $(@D) \
		BUILD_DIR="$(LPF_BUILD_OUTPUT)" \
		CMAKE_INSTALL_PREFIX="/usr" \
		install_headers DESTDIR=$(STAGING_DIR)
	@echo "LPF: Staging installation complete"
endef
endif

$(eval $(generic-package))

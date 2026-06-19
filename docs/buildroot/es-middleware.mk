################################################################################
#
# es-middleware
#
################################################################################

# Default: Use git repository
# Override with local.mk for local development (see README.md)
ES_MIDDLEWARE_VERSION = v1.0.0
ES_MIDDLEWARE_SITE = ssh://gitea@192.168.18.254:4022/CSPD/ES-Middleware.git
ES_MIDDLEWARE_SITE_METHOD = git
ES_MIDDLEWARE_LICENSE = Proprietary
ES_MIDDLEWARE_INSTALL_TARGET = YES

# Dependencies - Kconfig + CMake/Kbuild build system
ES_MIDDLEWARE_DEPENDENCIES = host-pkgconf host-cmake host-flex host-bison linux

# Optional: Add Kconfig tools if menuconfig support is enabled
ifeq ($(BR2_PACKAGE_ES_MIDDLEWARE_MENUCONFIG),y)
ES_MIDDLEWARE_DEPENDENCIES += host-kconfig-frontends
endif

# Kconfig configuration to use (from Buildroot config)
ES_MIDDLEWARE_KCONFIG_DEFCONFIG = $(call qstrip,$(BR2_PACKAGE_ES_MIDDLEWARE_DEFCONFIG))

# Build type from Buildroot configuration
ES_MIDDLEWARE_BUILD_TYPE = $(call qstrip,$(BR2_PACKAGE_ES_MIDDLEWARE_BUILD_TYPE))

# Build in-tree for Buildroot (output/build/es-middleware/_build)
ES_MIDDLEWARE_BUILD_OUTPUT = $(@D)/_build
ES_MIDDLEWARE_MODULES_OUTPUT = $(@D)/_build/modules
ES_MIDDLEWARE_MODULE_INSTALL_DIR = /lib/modules/$(LINUX_VERSION_PROBED)/extra/es-middleware

ES_MIDDLEWARE_MAKE_OPTS = \
	BUILD_DIR="$(ES_MIDDLEWARE_BUILD_OUTPUT)" \
	MODULES_BUILD_DIR="$(ES_MIDDLEWARE_MODULES_OUTPUT)" \
	KERNEL_SRC="$(LINUX_DIR)" \
	ARCH="$(KERNEL_ARCH)" \
	CROSS_COMPILE="$(TARGET_CROSS)" \
	CMAKE_BUILD_TYPE="$(ES_MIDDLEWARE_BUILD_TYPE)" \
	CMAKE_TOOLCHAIN_FILE="$(HOST_DIR)/share/buildroot/toolchainfile.cmake" \
	CMAKE_INSTALL_PREFIX="/usr"

# Configure using make (kernel-style interface)
# This loads the defconfig and generates .config + autoconf.h
define ES_MIDDLEWARE_CONFIGURE_CMDS
	@echo "ES-Middleware: Loading defconfig $(ES_MIDDLEWARE_KCONFIG_DEFCONFIG)"
	$(MAKE) -C $(@D) $(ES_MIDDLEWARE_MAKE_OPTS) $(ES_MIDDLEWARE_KCONFIG_DEFCONFIG)
	@echo "ES-Middleware: Configuration loaded successfully"
endef

# Build userspace libraries and kernel modules.
define ES_MIDDLEWARE_BUILD_CMDS
	@echo "ES-Middleware: Building with configuration $(ES_MIDDLEWARE_KCONFIG_DEFCONFIG)"
	$(TARGET_MAKE_ENV) $(MAKE) -C $(@D) $(ES_MIDDLEWARE_MAKE_OPTS) all
endef

# Clean build artifacts (keep configuration)
define ES_MIDDLEWARE_CLEAN_CMDS
	@echo "ES-Middleware: Cleaning build artifacts"
	-$(MAKE) -C $(@D) clean
endef

# Complete clean (remove configuration and build directory)
define ES_MIDDLEWARE_DISTCLEAN_CMDS
	@echo "ES-Middleware: Complete clean"
	-$(MAKE) -C $(@D) distclean
endef

# Install userspace libraries and kernel modules to target.
define ES_MIDDLEWARE_INSTALL_TARGET_CMDS
	@echo "ES-Middleware: Installing to target"
	$(MAKE) -C $(@D) \
		BUILD_DIR="$(ES_MIDDLEWARE_BUILD_OUTPUT)" \
		CMAKE_INSTALL_PREFIX="/usr" \
		install DESTDIR=$(TARGET_DIR)
	$(INSTALL) -d "$(TARGET_DIR)$(ES_MIDDLEWARE_MODULE_INSTALL_DIR)"
	modules_found=0; \
	for module in "$(ES_MIDDLEWARE_MODULES_OUTPUT)"/*.ko; do \
		[ -e "$$module" ] || continue; \
		modules_found=1; \
		$(INSTALL) -m 0644 "$$module" \
			"$(TARGET_DIR)$(ES_MIDDLEWARE_MODULE_INSTALL_DIR)/$$(basename "$$module")"; \
	done; \
	if [ "$$modules_found" -eq 0 ]; then \
		echo "ES-Middleware: no kernel modules found in $(ES_MIDDLEWARE_MODULES_OUTPUT)"; \
		exit 1; \
	fi
	@echo "ES-Middleware: Target installation complete"
endef

# Optional: Install development headers and userspace libraries to staging.
ifeq ($(BR2_PACKAGE_ES_MIDDLEWARE_INSTALL_HEADERS),y)
ES_MIDDLEWARE_INSTALL_STAGING = YES
define ES_MIDDLEWARE_INSTALL_STAGING_CMDS
	@echo "ES-Middleware: Installing libraries to staging"
	$(MAKE) -C $(@D) \
		BUILD_DIR="$(ES_MIDDLEWARE_BUILD_OUTPUT)" \
		CMAKE_INSTALL_PREFIX="/usr" \
		install DESTDIR=$(STAGING_DIR)
	@echo "ES-Middleware: Installing development headers to staging"
	$(MAKE) -C $(@D) \
		BUILD_DIR="$(ES_MIDDLEWARE_BUILD_OUTPUT)" \
		CMAKE_INSTALL_PREFIX="/usr" \
		install_headers DESTDIR=$(STAGING_DIR)
	@echo "ES-Middleware: Staging installation complete"
endef
endif

$(eval $(generic-package))

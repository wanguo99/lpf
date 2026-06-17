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

# Dependencies - Kconfig + CMake build system (zero Python dependency)
ES_MIDDLEWARE_DEPENDENCIES = host-pkgconf host-cmake

# Optional: Add Kconfig tools if menuconfig support is enabled
ifeq ($(BR2_PACKAGE_ES_MIDDLEWARE_MENUCONFIG),y)
ES_MIDDLEWARE_DEPENDENCIES += host-kconfig-frontends
endif

# Kconfig configuration to use (from Buildroot config)
ES_MIDDLEWARE_KCONFIG_DEFCONFIG = $(call qstrip,$(BR2_PACKAGE_ES_MIDDLEWARE_DEFCONFIG))

# Build type from Buildroot configuration
ES_MIDDLEWARE_BUILD_TYPE = $(call qstrip,$(BR2_PACKAGE_ES_MIDDLEWARE_BUILD_TYPE))

# Build in-tree for Buildroot (output/build/es-middleware/_build)
# This follows Buildroot convention: build directly in source directory
ES_MIDDLEWARE_BUILD_OUTPUT = $(@D)/_build

# Configure using make (kernel-style interface)
# This loads the defconfig and generates .config + autoconf.h
define ES_MIDDLEWARE_CONFIGURE_CMDS
	@echo "ES-Middleware: Loading defconfig $(ES_MIDDLEWARE_KCONFIG_DEFCONFIG)"
	$(MAKE) -C $(@D) $(ES_MIDDLEWARE_KCONFIG_DEFCONFIG)
	@echo "ES-Middleware: Configuration loaded successfully"
endef

# Build using make (auto-detects cores and calls CMake backend)
# Pass cross-compilation toolchain via CMAKE_TOOLCHAIN_FILE
define ES_MIDDLEWARE_BUILD_CMDS
	@echo "ES-Middleware: Building with configuration $(ES_MIDDLEWARE_KCONFIG_DEFCONFIG)"
	$(TARGET_MAKE_ENV) \
	$(MAKE) -C $(@D) \
		CMAKE_BUILD_TYPE="$(ES_MIDDLEWARE_BUILD_TYPE)" \
		CMAKE_TOOLCHAIN_FILE="$(HOST_DIR)/share/buildroot/toolchainfile.cmake" \
		CMAKE_INSTALL_PREFIX="/usr" \
		$(if $(BR2_PACKAGE_ES_MIDDLEWARE_INSTALL_HEADERS),INSTALL_DEVELOPMENT_HEADERS=ON) \
		all
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

# Install applications and libraries to target using make install
# Note: INSTALL_DEVELOPMENT_HEADERS is intentionally NOT set here
# Headers should only go to staging directory, not target rootfs
define ES_MIDDLEWARE_INSTALL_TARGET_CMDS
	@echo "ES-Middleware: Installing to target"
	$(MAKE) -C $(@D) \
		CMAKE_INSTALL_PREFIX="/usr" \
		INSTALL_DEVELOPMENT_HEADERS=OFF \
		install DESTDIR=$(TARGET_DIR)
	@echo "ES-Middleware: Target installation complete"
endef

# Optional: Install development headers to staging directory
ifeq ($(BR2_PACKAGE_ES_MIDDLEWARE_INSTALL_HEADERS),y)
ES_MIDDLEWARE_INSTALL_STAGING = YES
define ES_MIDDLEWARE_INSTALL_STAGING_CMDS
	@echo "ES-Middleware: Installing libraries to staging"
	$(MAKE) -C $(@D) \
		CMAKE_INSTALL_PREFIX="/usr" \
		install DESTDIR=$(STAGING_DIR)
	@echo "ES-Middleware: Installing development headers to staging"
	$(MAKE) -C $(@D) \
		CMAKE_INSTALL_PREFIX="/usr" \
		install_headers DESTDIR=$(STAGING_DIR)
	@echo "ES-Middleware: Staging installation complete"
endef
endif

# Optional: Install init scripts for PMC applications
ifeq ($(BR2_PACKAGE_ES_MIDDLEWARE_INSTALL_INIT),y)
define ES_MIDDLEWARE_INSTALL_INIT_SYSV
	@echo "ES-Middleware: Installing init scripts"
	$(INSTALL) -D -m 0755 $(ES_MIDDLEWARE_PKGDIR)/S90es-middleware \
		$(TARGET_DIR)/etc/init.d/S90es-middleware
endef
endif

$(eval $(generic-package))

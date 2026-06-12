# ES-Middleware Makefile
# Kernel/Buildroot-style interface with CMake backend
#
# Usage:
#   make <board>_defconfig            Load specific configuration
#   make menuconfig                   Interactive configuration
#   make                              Build (auto-detects cores)
#   make -j$(nproc)                   Parallel build
#   make all                          Build everything
#   make clean                        Remove build artifacts
#   make distclean                    Remove all generated files
#   make savedefconfig                Save minimal configuration
#   make help                         Show this help

# Configuration
KCONFIG_CONFIG ?= .config
CONFIGS_DIR := configs
CMAKE ?= cmake

# Support O= for custom build directory (kernel/Buildroot style)
# Usage: make O=build-arm64 <target>
# This enables parallel builds for different configurations
ifdef O
BUILD_DIR := $(O)
else
BUILD_DIR := _build
endif

# Build type configuration (can be overridden)
CMAKE_BUILD_TYPE ?= Debug

# Installation prefix (can be overridden for Buildroot)
CMAKE_INSTALL_PREFIX ?= /usr

# Don't check config for these targets
NO_CONFIG_TARGETS := help list clean distclean mrproper %_defconfig defconfig
ifneq ($(MAKECMDGOALS),)
ifeq ($(filter-out $(NO_CONFIG_TARGETS),$(MAKECMDGOALS)),)
SKIP_CONFIG_CHECK := y
endif
endif

# Auto-detect number of processors for parallel builds
ifeq ($(filter -j%,$(MAKEFLAGS)),)
NPROC := $(shell nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 1)
PARALLEL_BUILD := -j$(NPROC)
else
PARALLEL_BUILD :=
endif

# Phony targets
.PHONY: all help menuconfig nconfig defconfig savedefconfig oldconfig
.PHONY: clean distclean mrproper list install _build_internal _ensure_build_dir

# Default goal
.DEFAULT_GOAL := all

# Prevent Makefile itself from being considered a target
Makefile: ;

# Check configuration exists (unless skipped)
ifndef SKIP_CONFIG_CHECK
ifeq ($(wildcard $(KCONFIG_CONFIG)),)
$(error Configuration file '$(KCONFIG_CONFIG)' not found! Run 'make <board>_defconfig' first. Try 'make help' for options)
endif
endif

# Default target - build
all: _build_internal

_build_internal:
	@if [ ! -f "$(KCONFIG_CONFIG)" ]; then \
		echo "***"; \
		echo "*** Configuration file '$(KCONFIG_CONFIG)' not found!"; \
		echo "***"; \
		echo "*** Please run 'make <board>_defconfig' first."; \
		echo "*** Try 'make help' for available defconfigs."; \
		echo "***"; \
		exit 1; \
	fi
	@CMAKE_ARGS="-DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) -DCMAKE_INSTALL_PREFIX=$(CMAKE_INSTALL_PREFIX)"; \
	if [ -n "$(CMAKE_TOOLCHAIN_FILE)" ]; then \
		CMAKE_ARGS="$$CMAKE_ARGS -DCMAKE_TOOLCHAIN_FILE=$(CMAKE_TOOLCHAIN_FILE)"; \
		if [ -f "$(BUILD_DIR)/CMakeCache.txt" ]; then \
			if ! grep -q "CMAKE_TOOLCHAIN_FILE.*$(CMAKE_TOOLCHAIN_FILE)" "$(BUILD_DIR)/CMakeCache.txt" 2>/dev/null; then \
				echo "  CLEAN     $(BUILD_DIR) (toolchain changed)"; \
				rm -rf $(BUILD_DIR); \
			fi; \
		fi; \
	fi; \
	if [ ! -f "$(BUILD_DIR)/Makefile" ]; then \
		echo "  CMAKE     $(BUILD_DIR)"; \
		mkdir -p $(BUILD_DIR); \
		cd $(BUILD_DIR) && $(CMAKE) $$CMAKE_ARGS ..; \
	fi
	@$(MAKE) -C $(BUILD_DIR) $(PARALLEL_BUILD)

# Help target
help:
	@echo 'ES-Middleware Build System'
	@echo '==========================='
	@echo ''
	@echo 'Cleaning targets:'
	@echo '  clean              - Remove build artifacts (keep CMake cache)'
	@echo '  distclean          - Remove entire build directory and configuration'
	@echo '  mrproper           - Same as distclean'
	@echo ''
	@echo 'Configuration targets:'
	@echo '  menuconfig         - Interactive ncurses-based configuration'
	@echo '  nconfig            - Interactive ncurses-based alternative'
	@echo '  oldconfig          - Update configuration with new options'
	@echo '  savedefconfig      - Save minimal configuration to defconfig'
	@echo '  list               - List available defconfigs'
	@echo ''
	@echo 'Available defconfigs:'
	@for config in $(sort $(notdir $(wildcard $(CONFIGS_DIR)/*_defconfig))); do \
		printf "  %-30s- %s\\n" $$config "$${config%_defconfig}"; \
	done
	@echo ''
	@echo 'Build targets:'
	@echo '  all                - Build all configured targets (default)'
	@echo '  install            - Install binaries and libraries'
	@echo ''
	@echo 'Build options:'
	@echo '  O=<dir>            - Use custom build directory (default: _build)'
	@echo '  CMAKE_BUILD_TYPE=<type> - Set build type: Debug, Release, RelWithDebInfo'
	@echo '  CMAKE_INSTALL_PREFIX=<path> - Set installation prefix (default: /usr)'
	@echo '  DESTDIR=<path>     - Stage installation to alternate root (for install target)'
	@echo ''
	@echo 'Examples:'
	@echo '  # Standard workflow'
	@echo '  make tests_x86_minimal_defconfig'
	@echo '  make -j$$(nproc)'
	@echo '  make install DESTDIR=/tmp/staging'
	@echo ''
	@echo '  # Multiple parallel configurations using O='
	@echo '  make O=build-debug tests_x86_full_defconfig'
	@echo '  make O=build-debug CMAKE_BUILD_TYPE=Debug'
	@echo ''
	@echo '  make O=build-arm64 ccm_h200_100p_am625_release_defconfig'
	@echo '  make O=build-arm64 CMAKE_BUILD_TYPE=Release CMAKE_TOOLCHAIN_FILE=toolchain.cmake'
	@echo ''
	@echo '  # Cross-compilation for Buildroot'
	@echo '  make ccm_h200_100p_am625_release_defconfig'
	@echo '  make CMAKE_TOOLCHAIN_FILE=$$(BUILDROOT)/host/share/buildroot/toolchainfile.cmake'
	@echo '  make install DESTDIR=$$(BUILDROOT)/target'

# List available configurations
list:
	@echo 'Available defconfigs:'
	@echo ''
	@for config in $(sort $(notdir $(wildcard $(CONFIGS_DIR)/*_defconfig))); do \
		echo "  $$config"; \
	done

# Default configuration (first one found)
defconfig:
	@config="$(firstword $(wildcard $(CONFIGS_DIR)/*_defconfig))"; \
	if [ -z "$$config" ]; then \
		echo "Error: No defconfig found in $(CONFIGS_DIR)"; \
		exit 1; \
	fi; \
	echo "  DEFCONFIG $$(basename $$config)"; \
	cp "$$config" $(KCONFIG_CONFIG)

# Load specific defconfig
%_defconfig:
	@config="$(CONFIGS_DIR)/$@"; \
	if [ ! -f "$$config" ]; then \
		echo "***"; \
		echo "*** Error: Configuration file '$$config' not found!"; \
		echo "***"; \
		echo "*** Available defconfigs:"; \
		for cfg in $(sort $(notdir $(wildcard $(CONFIGS_DIR)/*_defconfig))); do \
			echo "***   $$cfg"; \
		done; \
		echo "***"; \
		exit 1; \
	fi; \
	echo "  DEFCONFIG $@"; \
	cp "$$config" $(KCONFIG_CONFIG)

# Ensure build directory is initialized with CMake
.PHONY: _ensure_build_dir
_ensure_build_dir:
	@if [ ! -f "$(BUILD_DIR)/Makefile" ]; then \
		echo "  CMAKE     $(BUILD_DIR)"; \
		mkdir -p $(BUILD_DIR); \
		cd $(BUILD_DIR) && $(CMAKE) -DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) -DCMAKE_INSTALL_PREFIX=$(CMAKE_INSTALL_PREFIX) .. >/dev/null; \
	fi

# Interactive configuration (menuconfig)
menuconfig: _ensure_build_dir
	@$(MAKE) -C $(BUILD_DIR) menuconfig
	@echo "  SYNC      $(KCONFIG_CONFIG)"

# Interactive configuration (nconfig)
nconfig: _ensure_build_dir
	@$(MAKE) -C $(BUILD_DIR) nconfig
	@echo "  SYNC      $(KCONFIG_CONFIG)"

# Update old configuration
oldconfig: _ensure_build_dir
	@$(MAKE) -C $(BUILD_DIR) oldconfig
	@echo "  SYNC      $(KCONFIG_CONFIG)"

# Save minimal configuration
savedefconfig: _ensure_build_dir
	@$(MAKE) -C $(BUILD_DIR) savedefconfig
	@echo "  SAVE      $(BUILD_DIR)/defconfig"
	@echo ''
	@echo 'Minimal configuration saved to $(BUILD_DIR)/defconfig'
	@echo 'To save as a board defconfig:'
	@echo '  cp $(BUILD_DIR)/defconfig $(CONFIGS_DIR)/<board>_defconfig'

# Clean build artifacts (keep CMake cache for faster reconfiguration)
clean:
	@if [ -f "$(BUILD_DIR)/Makefile" ]; then \
		echo "  CLEAN     $(BUILD_DIR) (objects only)"; \
		$(MAKE) -C $(BUILD_DIR) clean; \
	else \
		echo "  CLEAN     $(BUILD_DIR) (no build directory)"; \
	fi

# Complete clean (remove entire build directory)
distclean mrproper:
	@echo "  CLEAN     $(BUILD_DIR) (complete)"
	@rm -rf $(BUILD_DIR)
	@echo "  CLEAN     $(KCONFIG_CONFIG)"
	@rm -f $(KCONFIG_CONFIG)

# Install target (supports DESTDIR for Buildroot staging)
# Usage:
#   make install                          Install to /usr
#   make install DESTDIR=/tmp/staging     Install to /tmp/staging/usr
install:
	@if [ ! -f "$(BUILD_DIR)/Makefile" ]; then \
		echo "***"; \
		echo "*** Build directory '$(BUILD_DIR)' not found!"; \
		echo "*** Please run 'make' first to build the project."; \
		echo "***"; \
		exit 1; \
	fi
	@echo "  INSTALL   to $(if $(DESTDIR),$(DESTDIR))$(CMAKE_INSTALL_PREFIX)"
	@$(MAKE) -C $(BUILD_DIR) install $(if $(DESTDIR),DESTDIR=$(DESTDIR))

# Catch-all: forward to CMake build system
%: _ensure_build_dir
	@if [ ! -f "$(KCONFIG_CONFIG)" ]; then \
		echo "***"; \
		echo "*** Configuration file '$(KCONFIG_CONFIG)' not found!"; \
		echo "***"; \
		echo "*** Please run 'make <board>_defconfig' first."; \
		echo "*** Try 'make help' for available defconfigs."; \
		echo "***"; \
		exit 1; \
	fi
	@$(MAKE) -C $(BUILD_DIR) $@

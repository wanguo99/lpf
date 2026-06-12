# ES-Middleware Makefile
# Buildroot-style configuration and build system
# Makefile handles all Kconfig operations, CMake handles compilation only
#
# Architecture:
#   Makefile: Configuration management (menuconfig, defconfig, etc.) using native Kconfig tools
#   CMake:    Compilation and dependency management only
#
# Usage:
#   make <board>_defconfig            Load specific configuration
#   make menuconfig                   Interactive configuration
#   make                              Build (auto-detects cores)
#   make clean                        Remove build artifacts
#   make distclean                    Remove all generated files

#--------------------------------------------------------------
# Standard Makefile setup (Buildroot pattern)
#--------------------------------------------------------------

# Delete default rules to save processing time
.SUFFIXES:

# Ensure bash as shell for consistent behavior
SHELL := $(shell if [ -x "$$BASH" ]; then echo $$BASH; \
	 else if [ -x /bin/bash ]; then echo /bin/bash; \
	 else echo sh; fi; fi)

#--------------------------------------------------------------
# Core paths and configuration
#--------------------------------------------------------------

# Top-level directory (Buildroot convention)
TOPDIR := $(CURDIR)

# Kconfig infrastructure
CONFIG := $(TOPDIR)/scripts/kconfig
CONFIG_CONFIG_IN := Kconfig

# Configuration file
KCONFIG_CONFIG ?= .config
CONFIGS_DIR := configs

# CMake executable
CMAKE ?= cmake

# Host compiler (for building Kconfig tools)
HOSTCC ?= gcc
HOSTCXX ?= g++

#--------------------------------------------------------------
# Out-of-tree build support (O= variable)
#--------------------------------------------------------------

# Support O= for custom build directory (kernel/Buildroot style)
ifdef O
BUILD_DIR := $(O)
else
BUILD_DIR := _build
endif

# Normalize BUILD_DIR
override BUILD_DIR := $(patsubst %/,%,$(BUILD_DIR))

#--------------------------------------------------------------
# Build configuration
#--------------------------------------------------------------

CMAKE_BUILD_TYPE ?= Debug
CMAKE_INSTALL_PREFIX ?= /usr

#--------------------------------------------------------------
# Kconfig tools paths
#--------------------------------------------------------------

# Native Kconfig tools built from scripts/kconfig/
KCONFIG_TOOLS_DIR := $(BUILD_DIR)/kconfig-tools
KCONFIG_CONF := $(KCONFIG_TOOLS_DIR)/conf
KCONFIG_MCONF := $(KCONFIG_TOOLS_DIR)/mconf
KCONFIG_NCONF := $(KCONFIG_TOOLS_DIR)/nconf

# Kconfig output files
KCONFIG_AUTOCONF_DIR := $(BUILD_DIR)/include/generated
KCONFIG_AUTOCONF_H := $(KCONFIG_AUTOCONF_DIR)/autoconf.h
KCONFIG_AUTOCONFIG := $(BUILD_DIR)/include/config/auto.conf
KCONFIG_AUTOHEADER := $(BUILD_DIR)/include/config/autoconf.h
KCONFIG_TRISTATE := $(BUILD_DIR)/include/config/tristate.conf

#--------------------------------------------------------------
# Verbose build control (Buildroot/kernel convention)
#--------------------------------------------------------------

ifeq ("$(origin V)", "command line")
  KBUILD_VERBOSE = $(V)
endif
ifndef KBUILD_VERBOSE
  KBUILD_VERBOSE = 0
endif

ifeq ($(KBUILD_VERBOSE),1)
  Q =
  KCONFIG_VERBOSE =
ifndef VERBOSE
  VERBOSE = 1
endif
export VERBOSE
else
  Q = @
  KCONFIG_VERBOSE = -s
endif

export KBUILD_VERBOSE

#--------------------------------------------------------------
# Common Kconfig environment (Buildroot COMMON_CONFIG_ENV pattern)
#--------------------------------------------------------------

# Centralized environment for all Kconfig operations
COMMON_CONFIG_ENV = \
	KCONFIG_CONFIG=$(abspath $(BUILD_DIR)/$(KCONFIG_CONFIG)) \
	KCONFIG_AUTOCONFIG=$(KCONFIG_AUTOCONFIG) \
	KCONFIG_AUTOHEADER=$(KCONFIG_AUTOHEADER) \
	KCONFIG_TRISTATE=$(KCONFIG_TRISTATE) \
	CONFIG_=CONFIG_ \
	srctree=$(CURDIR) \
	KCONFIG_SEED=$(KCONFIG_SEED)

#--------------------------------------------------------------
# Target classification
#--------------------------------------------------------------

# Targets that don't require .config (Buildroot naming)
noconfig_targets := help list clean distclean mrproper \
	menuconfig nconfig oldconfig syncconfig olddefconfig \
	defconfig %_defconfig savedefconfig update-defconfig \
	silentoldconfig allnoconfig allyesconfig alldefconfig \
	randconfig listnewconfig

# Check if we need .config
ifneq ($(MAKECMDGOALS),)
ifeq ($(filter-out $(noconfig_targets),$(MAKECMDGOALS)),)
SKIP_CONFIG_CHECK := y
endif
endif

#--------------------------------------------------------------
# Parallel build auto-detection
#--------------------------------------------------------------

ifeq ($(filter -j%,$(MAKEFLAGS)),)
NPROC := $(shell nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 1)
PARALLEL_BUILD := -j$(NPROC)
else
PARALLEL_BUILD :=
endif

#--------------------------------------------------------------
# Phony targets
#--------------------------------------------------------------

.PHONY: all help menuconfig nconfig defconfig savedefconfig oldconfig syncconfig olddefconfig
.PHONY: silentoldconfig allnoconfig allyesconfig alldefconfig randconfig listnewconfig
.PHONY: clean distclean mrproper list install update-defconfig
.PHONY: _build_kconfig_tools _ensure_kconfig_tools _build_cmake

#--------------------------------------------------------------
# Default goal
#--------------------------------------------------------------

.DEFAULT_GOAL := all

# Prevent Makefile itself from being a target
Makefile: ;

#--------------------------------------------------------------
# Configuration validation
#--------------------------------------------------------------

ifndef SKIP_CONFIG_CHECK
ifeq ($(wildcard $(BUILD_DIR)/$(KCONFIG_CONFIG)),)
$(error Configuration file '$(BUILD_DIR)/$(KCONFIG_CONFIG)' not found! Run 'make <board>_defconfig' first. Try 'make list' for available configs)
endif
endif

#--------------------------------------------------------------
# Build Kconfig tools (Buildroot-style pattern rule)
#--------------------------------------------------------------

# Pattern rule to build Kconfig tools: conf, mconf, nconf, gconf, qconf
# Matches: _build/kconfig-tools/conf, _build/kconfig-tools/mconf, etc.
$(KCONFIG_TOOLS_DIR)/%onf:
	@mkdir -p $(@D)/lxdialog
	@$(MAKE) CC="$(HOSTCC)" HOSTCC="$(HOSTCC)" \
		obj=$(abspath $(@D)) -C $(CONFIG) -f Makefile.br $(@F)

_ensure_kconfig_tools: $(KCONFIG_CONF)

#--------------------------------------------------------------
# Configuration targets (Buildroot-style, pure Makefile implementation)
#--------------------------------------------------------------

# Interactive configuration (menuconfig)
menuconfig: $(KCONFIG_MCONF)
	@$(COMMON_CONFIG_ENV) $(KCONFIG_MCONF) $(CONFIG_CONFIG_IN)
	@echo "  GEN       $(KCONFIG_AUTOCONF_H)"
	@$(CURDIR)/scripts/gen_autoconf.sh $(BUILD_DIR)/$(KCONFIG_CONFIG) $(KCONFIG_AUTOCONF_H)

# Alternative interactive configuration (nconfig)
nconfig: $(KCONFIG_NCONF)
	@$(COMMON_CONFIG_ENV) $(KCONFIG_NCONF) $(CONFIG_CONFIG_IN)
	@echo "  GEN       $(KCONFIG_AUTOCONF_H)"
	@$(CURDIR)/scripts/gen_autoconf.sh $(BUILD_DIR)/$(KCONFIG_CONFIG) $(KCONFIG_AUTOCONF_H)

# Update configuration with prompts for new symbols
oldconfig: $(KCONFIG_CONF)
	$(Q)$(COMMON_CONFIG_ENV) $(KCONFIG_CONF) $(KCONFIG_VERBOSE) --oldconfig $(CONFIG_CONFIG_IN)
	@echo "  GEN       $(KCONFIG_AUTOCONF_H)"
	$(Q)$(CURDIR)/scripts/gen_autoconf.sh $(BUILD_DIR)/$(KCONFIG_CONFIG) $(KCONFIG_AUTOCONF_H)

# Synchronize configuration (Buildroot/kernel standard)
syncconfig: $(KCONFIG_CONF)
	$(Q)mkdir -p $(KCONFIG_AUTOCONF_DIR)
	$(Q)mkdir -p $(dir $(KCONFIG_AUTOCONFIG))
	$(Q)$(COMMON_CONFIG_ENV) $(KCONFIG_CONF) $(KCONFIG_VERBOSE) --syncconfig $(CONFIG_CONFIG_IN)

# Update with defaults for new symbols (no prompts)
olddefconfig silentoldconfig: $(KCONFIG_CONF)
	$(Q)$(COMMON_CONFIG_ENV) $(KCONFIG_CONF) $(KCONFIG_VERBOSE) --olddefconfig $(CONFIG_CONFIG_IN)

# All yes config
allyesconfig: $(KCONFIG_CONF)
	$(Q)$(COMMON_CONFIG_ENV) $(KCONFIG_CONF) $(KCONFIG_VERBOSE) --allyesconfig $(CONFIG_CONFIG_IN)

# All no config
allnoconfig: $(KCONFIG_CONF)
	$(Q)$(COMMON_CONFIG_ENV) $(KCONFIG_CONF) $(KCONFIG_VERBOSE) --allnoconfig $(CONFIG_CONFIG_IN)

# All default config
alldefconfig: $(KCONFIG_CONF)
	$(Q)$(COMMON_CONFIG_ENV) $(KCONFIG_CONF) $(KCONFIG_VERBOSE) --alldefconfig $(CONFIG_CONFIG_IN)

# Random config
randconfig: $(KCONFIG_CONF)
	$(Q)$(COMMON_CONFIG_ENV) $(KCONFIG_CONF) $(KCONFIG_VERBOSE) --randconfig $(CONFIG_CONFIG_IN)

# List new config symbols
listnewconfig: $(KCONFIG_CONF)
	$(Q)$(COMMON_CONFIG_ENV) $(KCONFIG_CONF) $(KCONFIG_VERBOSE) --listnewconfig $(CONFIG_CONFIG_IN)

#--------------------------------------------------------------
# defconfig targets (Buildroot pattern with conf tool)
#--------------------------------------------------------------

# Default configuration (first one found)
defconfig: $(KCONFIG_CONF)
	$(Q)config="$(firstword $(wildcard $(CONFIGS_DIR)/*_defconfig))"; \
	if [ -z "$$config" ]; then \
		echo "Error: No defconfig found in $(CONFIGS_DIR)"; \
		exit 1; \
	fi; \
	echo "  DEFCONFIG $$(basename $$config)"; \
	$(COMMON_CONFIG_ENV) $(KCONFIG_CONF) --defconfig="$$config" $(CONFIG_CONFIG_IN)

# Load specific defconfig (Buildroot multi-directory search pattern)
%_defconfig: $(KCONFIG_CONF)
	$(Q)config="$(CONFIGS_DIR)/$@"; \
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
	$(COMMON_CONFIG_ENV) $(KCONFIG_CONF) --defconfig="$$config" $(CONFIG_CONFIG_IN); \
	if [ -f "$(KCONFIG_CONFIG)" ] && [ "$(abspath $(KCONFIG_CONFIG))" != "$(abspath $(BUILD_DIR)/$(KCONFIG_CONFIG))" ]; then \
		mkdir -p $(BUILD_DIR); \
		mv -f $(KCONFIG_CONFIG) $(BUILD_DIR)/$(KCONFIG_CONFIG); \
	fi
	@echo "  GEN       $(KCONFIG_AUTOCONF_H)"
	$(Q)$(CURDIR)/scripts/gen_autoconf.sh $(BUILD_DIR)/$(KCONFIG_CONFIG) $(KCONFIG_AUTOCONF_H)

# Save minimal configuration
savedefconfig: $(KCONFIG_CONF)
	$(Q)$(COMMON_CONFIG_ENV) $(KCONFIG_CONF) \
		--savedefconfig=$(BUILD_DIR)/defconfig $(CONFIG_CONFIG_IN)
	@echo "  SAVE      $(BUILD_DIR)/defconfig"
	@echo ''
	@echo 'Minimal configuration saved to $(BUILD_DIR)/defconfig'
	@echo 'To save as a board defconfig:'
	@echo '  cp $(BUILD_DIR)/defconfig $(CONFIGS_DIR)/<board>_defconfig'

# Alias for savedefconfig (Buildroot convention)
update-defconfig: savedefconfig

#--------------------------------------------------------------
# Build target
#--------------------------------------------------------------

all: _build_cmake

_build_cmake:
	$(Q)if [ ! -f "$(BUILD_DIR)/$(KCONFIG_CONFIG)" ]; then \
		echo "***"; \
		echo "*** Configuration file '$(BUILD_DIR)/$(KCONFIG_CONFIG)' not found!"; \
		echo "***"; \
		echo "*** Please run 'make <board>_defconfig' first."; \
		echo "*** Available configs: run 'make list'"; \
		echo "***"; \
		exit 1; \
	fi
	$(Q)CMAKE_ARGS="-DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) -DCMAKE_INSTALL_PREFIX=$(CMAKE_INSTALL_PREFIX)"; \
	CMAKE_ARGS="$$CMAKE_ARGS -DCONFIG_FILE=$(abspath $(BUILD_DIR)/$(KCONFIG_CONFIG))"; \
	CMAKE_ARGS="$$CMAKE_ARGS -DAUTOCONF_H=$(abspath $(KCONFIG_AUTOCONF_H))"; \
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
	$(Q)$(MAKE) -C $(BUILD_DIR) $(PARALLEL_BUILD)

#--------------------------------------------------------------
# Cleaning targets
#--------------------------------------------------------------

clean:
	$(Q)if [ -f "$(BUILD_DIR)/Makefile" ]; then \
		echo "  CLEAN     $(BUILD_DIR) (objects only)"; \
		$(MAKE) -C $(BUILD_DIR) clean; \
	else \
		echo "  CLEAN     $(BUILD_DIR) (no build directory)"; \
	fi

distclean mrproper:
	@echo "  CLEAN     $(BUILD_DIR) (complete)"
	$(Q)rm -rf $(BUILD_DIR)
	@echo "  CLEAN     $(KCONFIG_CONFIG)"
	$(Q)rm -f $(KCONFIG_CONFIG) $(KCONFIG_CONFIG).old

#--------------------------------------------------------------
# Installation target
#--------------------------------------------------------------

install:
	$(Q)if [ ! -f "$(BUILD_DIR)/Makefile" ]; then \
		echo "***"; \
		echo "*** Build directory '$(BUILD_DIR)' not found!"; \
		echo "*** Please run 'make' first to build the project."; \
		echo "***"; \
		exit 1; \
	fi
	@echo "  INSTALL   to $(if $(DESTDIR),$(DESTDIR))$(CMAKE_INSTALL_PREFIX)"
	$(Q)$(MAKE) -C $(BUILD_DIR) install $(if $(DESTDIR),DESTDIR=$(DESTDIR))

#--------------------------------------------------------------
# Help and information
#--------------------------------------------------------------

help:
	@echo 'ES-Middleware Build System'
	@echo '==========================='
	@echo ''
	@echo 'Configuration targets:'
	@echo '  menuconfig         - Interactive ncurses-based configuration'
	@echo '  nconfig            - Interactive ncurses-based alternative'
	@echo '  oldconfig          - Update configuration (prompt for new options)'
	@echo '  syncconfig         - Update configuration and dependencies (no prompts)'
	@echo '  olddefconfig       - Update configuration with defaults (no prompts)'
	@echo '  silentoldconfig    - Alias for olddefconfig'
	@echo '  defconfig          - Load default configuration (first found)'
	@echo '  <board>_defconfig  - Load specific board configuration'
	@echo '  savedefconfig      - Save minimal configuration'
	@echo '  update-defconfig   - Alias for savedefconfig'
	@echo '  allyesconfig       - Enable all options'
	@echo '  allnoconfig        - Disable all options'
	@echo '  alldefconfig       - All options set to default'
	@echo '  randconfig         - Random configuration'
	@echo '  listnewconfig      - List new configuration options'
	@echo '  list               - List available defconfigs'
	@echo ''
	@echo 'Build targets:'
	@echo '  all                - Build all configured targets (default)'
	@echo '  install            - Install binaries and libraries'
	@echo '  clean              - Remove build artifacts (keep CMake cache)'
	@echo '  distclean          - Remove entire build directory and configuration'
	@echo '  mrproper           - Same as distclean'
	@echo ''
	@echo 'Build options:'
	@echo '  V=0|1              - 0: quiet build (default), 1: verbose'
	@echo '  O=<dir>            - Use custom build directory (default: _build)'
	@echo '  CMAKE_BUILD_TYPE=<type>'
	@echo '                     - Set build type: Debug, Release, RelWithDebInfo'
	@echo '  CMAKE_INSTALL_PREFIX=<path>'
	@echo '                     - Set installation prefix (default: /usr)'
	@echo '  DESTDIR=<path>     - Stage installation to alternate root'
	@echo '  CMAKE_TOOLCHAIN_FILE=<file>'
	@echo '                     - Specify CMake toolchain for cross-compilation'
	@echo ''
	@echo 'Available defconfigs:'
	@for config in $(sort $(notdir $(wildcard $(CONFIGS_DIR)/*_defconfig))); do \
		printf "  %-30s\n" $$config; \
	done
	@echo ''
	@echo 'Examples:'
	@echo '  # Standard workflow'
	@echo '  make tests_x86_minimal_defconfig'
	@echo '  make -j$$(nproc)'
	@echo '  make install DESTDIR=/tmp/staging'
	@echo ''
	@echo '  # Interactive configuration'
	@echo '  make menuconfig'
	@echo '  make'
	@echo ''
	@echo '  # Verbose build'
	@echo '  make V=1'
	@echo ''
	@echo '  # Cross-compilation for Buildroot'
	@echo '  make ccm_h200_100p_am625_release_defconfig'
	@echo '  make CMAKE_TOOLCHAIN_FILE=$$BUILDROOT/host/share/buildroot/toolchainfile.cmake'
	@echo '  make install DESTDIR=$$BUILDROOT/target'

list:
	@echo 'Available defconfigs:'
	@echo ''
	@for config in $(sort $(notdir $(wildcard $(CONFIGS_DIR)/*_defconfig))); do \
		echo "  $$config"; \
	done

#--------------------------------------------------------------
# Catch-all: forward to CMake
#--------------------------------------------------------------

%:
	$(Q)if [ ! -f "$(KCONFIG_CONFIG)" ]; then \
		echo "***"; \
		echo "*** Configuration file '$(KCONFIG_CONFIG)' not found!"; \
		echo "***"; \
		echo "*** Please run 'make <board>_defconfig' first."; \
		echo "*** Try 'make list' for available defconfigs."; \
		echo "***"; \
		exit 1; \
	fi
	$(Q)if [ ! -f "$(BUILD_DIR)/Makefile" ]; then \
		echo "***"; \
		echo "*** Build directory not initialized. Run 'make' first."; \
		echo "***"; \
		exit 1; \
	fi
	$(Q)$(MAKE) -C $(BUILD_DIR) $@

#--------------------------------------------------------------
# End of Makefile
#--------------------------------------------------------------

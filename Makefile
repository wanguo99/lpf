# ==============================================================================
# LPF Makefile
# ==============================================================================
# This Makefile provides a clean wrapper layer between user commands and the
# CMake build system, inspired by Linux kernel and Buildroot architecture.
#
# Responsibilities:
#   - Kconfig configuration management (menuconfig, defconfig, etc.)
#   - Build Kconfig tools (conf, mconf, nconf)
#   - Generate autoconf.h from .config
#   - Invoke CMake for compilation
#   - Provide user-friendly interface
#
# Design principles:
#   - Configuration and compilation are separate phases
#   - CMake handles compilation only (no config management)
#   - Clear error messages and validation
#   - Compatible with Buildroot/Yocto integration
# ==============================================================================

VERSION = 1
PATCHLEVEL = 0
SUBLEVEL = 0
EXTRAVERSION =
NAME = LPF

# Version string
export VERSION PATCHLEVEL SUBLEVEL EXTRAVERSION
export VERSION_STRING := $(VERSION).$(PATCHLEVEL).$(SUBLEVEL)$(EXTRAVERSION)

# Do not print "Entering directory ..."
MAKEFLAGS += --no-print-directory

# To put more focus on warnings, be less verbose as default
# Use 'make V=1' to see the full commands

ifdef V
  ifeq ("$(origin V)", "command line")
    KBUILD_VERBOSE = $(V)
  endif
endif
ifndef KBUILD_VERBOSE
  KBUILD_VERBOSE = 0
endif

# kbuild supports saving output files in a separate directory.
# For LPF, we use BUILD_DIR variable for CMake output directory.
# This is simpler than full out-of-tree build support.

# Build directory (for CMake build artifacts)
BUILD_DIR ?= _build
TEST_BUILD_DIR ?= _build/tests

# Kconfig directory
KCONFIG_DIR := scripts/kconfig

# Normalize BUILD_DIR
override BUILD_DIR := $(patsubst %/,%,$(BUILD_DIR))
override TEST_BUILD_DIR := $(patsubst %/,%,$(TEST_BUILD_DIR))

# CMake configuration
CMAKE ?= cmake
CMAKE_BUILD_TYPE ?= Release
# Installation prefix - use /usr for Buildroot, /usr/local for standalone
CMAKE_INSTALL_PREFIX ?= $(if $(BR2_EXTERNAL),/usr,/usr/local)
DESTDIR ?=

# Kernel module build configuration
KERNEL_SRC ?= /lib/modules/$(shell uname -r)/build
MODULES_BUILD_DIR ?= _build/modules
override MODULES_BUILD_DIR := $(patsubst %/,%,$(MODULES_BUILD_DIR))
MODULES_SRC_DIR ?= $(srctree)/kernel
MODULES_OUTPUT_DIR ?= $(MODULES_BUILD_DIR)
MODULES_LIST ?= $(strip \
	$(if $(filter y m,$(CONFIG_OSAL)),osal) \
	$(if $(filter y m,$(CONFIG_LPF_CONFIGS)),lpf_configs) \
	$(if $(filter y m,$(CONFIG_LPF_CORE)),lpf_core) \
	$(if $(filter y m,$(CONFIG_LPF_HW_MOCK_SELFTEST)),lpf_hw_mock_selftest) \
	$(if $(filter y m,$(CONFIG_LPF_DUMMY_SERVICE_SELFTEST)),lpf_dummy_service_selftest))
MODULES_ARTIFACTS = $(addprefix $(MODULES_OUTPUT_DIR)/,$(addsuffix .ko,$(MODULES_LIST)))

# Parallel build auto-detection
ifeq ($(filter -j%,$(MAKEFLAGS)),)
NPROC := $(shell nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 1)
PARALLEL_BUILD := -j$(NPROC)
else
PARALLEL_BUILD :=
endif

# KBUILD_SRC is set on invocation of make in OBJ directory
# KBUILD_SRC is not intended to be used by the regular user (for now)
ifeq ($(KBUILD_SRC),)

# That's our default target when none is given on the command line
PHONY := _all
_all:

endif # ifeq ($(KBUILD_SRC),)

# We process the rest of the Makefile if this is the final invocation of make
ifeq ($(skip-makefile),)

# If building an external module we do not care about the all: rule
# but instead _all depend on modules
PHONY += all
_all: all

srctree		:= $(if $(KBUILD_SRC),$(KBUILD_SRC),$(CURDIR))
TOPDIR		:= $(srctree)
objtree		:= $(CURDIR)
src		:= $(srctree)
obj		:= $(objtree)

VPATH		:= $(srctree)

# srcroot is used by Linux 7.0 Kbuild - it's the relative source root
srcroot		:= $(srctree)

export srctree objtree srcroot VPATH TOPDIR

# SHELL used by kbuild
# Prefer BASH env var, then search PATH, finally fallback to sh
CONFIG_SHELL := $(shell if [ -x "$$BASH" ]; then echo $$BASH; \
	  else command -v bash 2>/dev/null || echo sh; fi)

# Host compilation flags for building tools (like kconfig)
KBUILD_HOSTCFLAGS := -I$(srctree)/scripts/include
export KBUILD_HOSTCFLAGS

# Make variables (CC, etc...)
AS		= $(CROSS_COMPILE)as
LD		= $(CROSS_COMPILE)ld
CC		= $(CROSS_COMPILE)gcc
CPP		= $(CC) -E
AR		= $(CROSS_COMPILE)ar
NM		= $(CROSS_COMPILE)nm
STRIP		= $(CROSS_COMPILE)strip
OBJCOPY		= $(CROSS_COMPILE)objcopy
OBJDUMP		= $(CROSS_COMPILE)objdump
AWK		= awk
LEX		= flex
YACC		= bison

HOSTCC  	= gcc
HOSTCXX  	= g++
HOSTCFLAGS	:=
HOSTCXXFLAGS	:=

export HOSTCC HOSTCXX HOSTCFLAGS HOSTCXXFLAGS LEX YACC

# We need some generic definitions
include $(srctree)/scripts/Kbuild.include

HOSTCFLAGS	+= $(call hostcc-option,-Wall -Wstrict-prototypes -O2 -fomit-frame-pointer,)
HOSTCXXFLAGS	+= -O2

# For maximum performance (+ possibly random breakage, uncomment
# the following)
MAKEFLAGS += -rR

export HOSTCC HOSTCXX HOSTCFLAGS HOSTCXXFLAGS

#########################################################################

KBUILD_MODULES :=
KBUILD_BUILTIN := 1

export KBUILD_MODULES KBUILD_BUILTIN
export KBUILD_CHECKSRC KBUILD_SRC

# We need some generic definitions (do not try to remake the file).
$(srctree)/scripts/Kbuild.include: ;
include $(srctree)/scripts/Kbuild.include

# Make "config" the default target if there is no configuration file or
# "all" the target if there is already one.
ifeq ($(wildcard .config),)
  KBUILD_DEFCONFIG := defconfig
else
  KBUILD_DEFCONFIG :=
endif

# Basic helpers built in scripts/
PHONY += scripts_basic
scripts_basic:
	$(Q)$(MAKE) $(build)=scripts/basic

# To avoid any implicit rule to kick in, define an empty command.
scripts/basic/%: scripts_basic ;

#########################################################################

# Look for make include files relative to root of kernel src
MAKEFLAGS += --include-dir=$(srctree)

ifeq ($(KBUILD_VERBOSE),1)
  quiet =
  Q =
else
  quiet=quiet_
  Q = @
endif

# If the user is running make -s (silent mode), suppress echoing of
# commands

ifneq ($(findstring s,$(MAKEFLAGS)),)
  quiet=silent_
endif

export quiet Q KBUILD_VERBOSE

#########################################################################

# To make sure we do not include .config for any of the *config targets
# catch them early, and hand them over to scripts/kconfig/Makefile
# It is allowed to specify more targets when calling make, including
# mixing *config targets and build targets.
# For example 'make oldconfig all'.
# Detect when mixed targets is specified, and make a second invocation
# of make so .config is not included in this case either (for *config).

no-dot-config-targets := clean mrproper distclean \
			 help list

config-targets := 0
mixed-targets  := 0
dot-config     := 1

ifneq ($(filter $(no-dot-config-targets), $(MAKECMDGOALS)),)
	ifeq ($(filter-out $(no-dot-config-targets), $(MAKECMDGOALS)),)
		dot-config := 0
	endif
endif

ifneq ($(filter config %config,$(MAKECMDGOALS)),)
        config-targets := 1
        ifneq ($(filter-out config %config,$(MAKECMDGOALS)),)
                mixed-targets := 1
        endif
endif

ifeq ($(mixed-targets),1)
# ===========================================================================
# We're called with mixed targets (*config and build targets).
# Handle them one by one.

%:: FORCE
	$(Q)$(MAKE) -C $(srctree) KBUILD_SRC= $@

else
ifeq ($(config-targets),1)
# ===========================================================================
# *config targets only - make sure prerequisites are updated, and descend
# in scripts/kconfig to make the *config target

# KBUILD_DEFCONFIG may point out an alternative default configuration
# used for 'make defconfig'
export KBUILD_DEFCONFIG

# Use Config.in instead of Kconfig
export KBUILD_KCONFIG := Config.in

config: scripts_basic FORCE
	$(Q)mkdir -p include
	$(Q)$(MAKE) $(build)=scripts/kconfig $@
	@if [ -f .config ]; then \
		$(srctree)/scripts/gen_version.sh; \
		echo ""; \
		echo "==================================================================="; \
		echo "Configuration file generated: $(CURDIR)/.config"; \
		echo "==================================================================="; \
		echo ""; \
		echo "Next steps:"; \
		echo "  make all          - Build the project with this configuration"; \
		echo "  make menuconfig   - Modify configuration interactively"; \
		echo "==================================================================="; \
		echo ""; \
	fi

%config: scripts_basic FORCE
	$(Q)mkdir -p include
	@target=$@; \
	case "$$target" in \
		menuconfig|nconfig|xconfig|gconfig) \
			$(MAKE) $(build)=scripts/kconfig $$target; \
			;; \
		*) \
			if [ "$(KBUILD_VERBOSE)" = "1" ]; then \
				if echo "$$target" | grep -q "_defconfig$$"; then \
					$(MAKE) $(build)=scripts/kconfig $$target; \
				else \
					$(MAKE) $(build)=scripts/kconfig $$target; \
				fi; \
			else \
				logfile=/tmp/kconfig.$$$$.log; \
				if echo "$$target" | grep -q "_defconfig$$"; then \
					$(MAKE) --no-print-directory $(build)=scripts/kconfig $$target 2>&1 | \
					while IFS= read -r line; do \
						case "$$line" in \
							"  HOSTCC  "*|"  HOSTLD  "*|"  SHIPPED "*) \
								echo "$$line" ;; \
							*"error:"*|*"Error:"*|*"ERROR:"*) \
								echo "$$line" >&2 ;; \
							*) \
								echo "$$line" >> $$logfile ;; \
						esac; \
					done; \
				else \
					$(MAKE) --no-print-directory $(build)=scripts/kconfig $$target 2>&1 | \
					while IFS= read -r line; do \
						case "$$line" in \
							"  HOSTCC  "*|"  HOSTLD  "*|"  SHIPPED "*) \
								echo "$$line" ;; \
							*"error:"*|*"Error:"*|*"ERROR:"*) \
								echo "$$line" >&2 ;; \
							*) \
								echo "$$line" >> $$logfile ;; \
						esac; \
					done; \
				fi; \
				status=$$?; \
				if [ $$status -ne 0 ]; then \
					if [ -f $$logfile ]; then \
						cat $$logfile >&2; \
						rm -f $$logfile; \
					fi; \
					exit $$status; \
				fi; \
				rm -f $$logfile; \
			fi; \
			;; \
	esac
	@if [ -f .config ]; then \
		$(srctree)/scripts/gen_version.sh; \
		echo ""; \
		echo "==================================================================="; \
		echo "Configuration file generated: $(CURDIR)/.config"; \
		echo "==================================================================="; \
		echo ""; \
		echo "Next steps:"; \
		echo "  make all          - Build the project with this configuration"; \
		echo "  make menuconfig   - Modify configuration interactively"; \
		echo "==================================================================="; \
		echo ""; \
	fi

else
# ===========================================================================
# Build targets only - this includes all build targets
# targets and others. In general all targets except *config targets.

prepare: scripts_basic include/generated/gen_autoconf.h include/generated/gen_version.h

ifeq ($(dot-config),1)
# In this section, we need .config

# Read in dependencies to all Config.in* files, make sure to run
# oldconfig if changes are detected.
-include .kconfig.d

-include .config

# If .config needs to be updated, it will be done via the dependency
# that autoconf has on .config.
# To avoid any implicit rule to kick in, define an empty command
.config .kconfig.d: ;

# If .config is newer than include/generated/gen_autoconf.h, someone tinkered
# with it and forgot to run make oldconfig.
# If kconfig.d is missing then we are probably in a cleaned tree so
# we execute the config step to be sure to catch updated Config.in files
include/generated/gen_autoconf.h: .config $(wildcard .kconfig.d)
	$(Q)$(MAKE) -f $(srctree)/Makefile syncconfig

# Generate gen_version.h with build information
include/generated/gen_version.h: .config FORCE
	$(Q)$(srctree)/scripts/gen_version.sh

else
# Dummy target needed, because used as prerequisite
include/generated/gen_autoconf.h: ;
include/generated/gen_version.h: ;
endif

# The all: target is the default when no target is given on the command line.
# Build userspace libraries first, then kernel modules. Keep these serialized
# because both paths refresh generated version/config headers.
PHONY += all
all:
	@echo ""
	@echo "==================================================================="
	@echo "LPF Full Build"
	@echo "==================================================================="
	$(Q)$(MAKE) libs
	$(Q)$(MAKE) modules
	@echo ""
	@echo "==================================================================="
	@echo "Full build completed successfully!"
	@echo "==================================================================="
	@echo "Library output: $(BUILD_DIR)/lib/"
	@echo "Module output:  $(MODULES_OUTPUT_DIR)/"
	@echo ""

PHONY += libs
libs: _check_config _validate_config include/generated/gen_autoconf.h include/generated/gen_version.h _cmake_configure
	@echo ""
	@echo "==================================================================="
	@echo "LPF Library Build"
	@echo "==================================================================="
	@echo ""
	@echo "Configuration: $(CURDIR)/.config"
	@echo "Building with CMake..."
	@echo ""
	@echo "  BUILD    LPF libraries"
	$(Q)$(MAKE) -C $(BUILD_DIR) $(PARALLEL_BUILD)
	@echo ""
	@echo "==================================================================="
	@echo "Library build completed successfully!"
	@echo "==================================================================="
	@echo "Output directory: $(BUILD_DIR)"
	@echo "Libraries: $(BUILD_DIR)/lib/"
	@echo ""
	@echo "Next steps:"
	@echo "  make modules              - Build kernel modules"
	@echo "  make install              - Install libraries to system"
	@echo ""

PHONY += tests
tests: _check_config _validate_config include/generated/gen_autoconf.h include/generated/gen_version.h
	@echo ""
	@echo "==================================================================="
	@echo "LPF Test Build"
	@echo "==================================================================="
	@echo ""
	@echo "Configuration: $(CURDIR)/.config"
	@echo "Test output:   $(TEST_BUILD_DIR)"
	@echo ""
	@echo "  CMAKE    Configuring test build"
	$(Q)mkdir -p $(TEST_BUILD_DIR)
	$(Q)cd $(TEST_BUILD_DIR) && $(CMAKE) \
		-DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) \
		-DCMAKE_INSTALL_PREFIX=$(CMAKE_INSTALL_PREFIX) \
		-DCONFIG_FILE=$(abspath $(CURDIR)/.config) \
		-DAUTOCONF_H=$(abspath $(CURDIR)/include/generated/gen_autoconf.h) \
		-DLPF_BUILD_TESTS=ON \
		$(if $(CMAKE_TOOLCHAIN_FILE),-DCMAKE_TOOLCHAIN_FILE=$(CMAKE_TOOLCHAIN_FILE)) \
		$(CMAKE_EXTRA_FLAGS) \
		$(CURDIR)
	@echo "  BUILD    LPF tests"
	$(Q)$(MAKE) -C $(TEST_BUILD_DIR) $(PARALLEL_BUILD)
	@echo "  CTEST    LPF tests"
	$(Q)cd $(TEST_BUILD_DIR) && ctest --output-on-failure
	@echo ""
	@echo "==================================================================="
	@echo "Test build completed successfully!"
	@echo "==================================================================="
	@echo ""

PHONY += modules
modules: _check_config include/generated/gen_autoconf.h include/generated/gen_version.h _modules_check_environment _modules_prepare
	@echo ""
	@echo "==================================================================="
	@echo "LPF Kernel Module Build"
	@echo "==================================================================="
	@echo ""
	@echo "Kernel source: $(KERNEL_SRC)"
	@echo "Module source: $(abspath $(MODULES_SRC_DIR))"
	@echo "Output dir:    $(abspath $(MODULES_OUTPUT_DIR))"
	@echo "ARCH:          $(ARCH)"
	@echo "CROSS_COMPILE: $(CROSS_COMPILE)"
	@echo "CC:            $(CC)"
	@echo ""
	@echo "  KBUILD   $(addsuffix .ko,$(MODULES_LIST))"
	$(Q)$(MAKE) -C $(KERNEL_SRC) \
		ARCH="$(ARCH)" \
		CROSS_COMPILE="$(CROSS_COMPILE)" \
		CC="$(CC)" \
		M=$(abspath $(MODULES_SRC_DIR)) \
		MO=$(abspath $(MODULES_OUTPUT_DIR)) \
		modules
	$(Q)if [ "$(abspath $(MODULES_OUTPUT_DIR))" != "$(abspath $(MODULES_SRC_DIR))" ]; then \
		for module in $(MODULES_LIST); do \
			src="$(abspath $(MODULES_SRC_DIR))/$$module.ko"; \
			dst="$(abspath $(MODULES_OUTPUT_DIR))/$$module.ko"; \
			if [ ! -f "$$dst" ] && [ -f "$$src" ]; then \
				cp -f "$$src" "$$dst"; \
			fi; \
		done; \
	fi
	@echo ""
	@echo "==================================================================="
	@echo "Kernel module build completed successfully!"
	@for artifact in $(MODULES_ARTIFACTS); do \
		if [ ! -f "$$artifact" ]; then \
			echo "ERROR: Missing expected artifact: $$artifact"; \
			exit 1; \
		fi; \
		echo "Artifact: $$(readlink -f $$artifact)"; \
	done
	@echo "==================================================================="
	@echo ""

PHONY += mock-modules-smoke
mock-modules-smoke:
	@echo ""
	@echo "==================================================================="
	@echo "LPF Mock Kernel Module Smoke Test"
	@echo "==================================================================="
	@echo ""
	$(Q)LPF_MODULE_DIR="$(abspath $(MODULES_OUTPUT_DIR))" \
		LPF_EXPECT_INSTANCE_DEVNODE_MODE=$(CONFIG_LPF_INSTANCE_DEVNODE_MODE) \
		$(CONFIG_SHELL) $(srctree)/scripts/lpf_mock_module_smoke.sh
	@echo ""
	@echo "==================================================================="
	@echo "Mock kernel module smoke test completed successfully!"
	@echo "==================================================================="
	@echo ""

PHONY += kernel-matrix
kernel-matrix:
	@echo ""
	@echo "==================================================================="
	@echo "LPF Kernel Module Matrix Build"
	@echo "==================================================================="
	@echo ""
	$(Q)KERNEL_SRC="$(KERNEL_SRC)" \
		KERNEL_SRC_LIST="$(KERNEL_SRC_LIST)" \
		LPF_KERNEL_MATRIX_DEFCONFIG="$(LPF_KERNEL_MATRIX_DEFCONFIG)" \
		LPF_KERNEL_MATRIX_BUILD_ROOT="$(LPF_KERNEL_MATRIX_BUILD_ROOT)" \
		$(CONFIG_SHELL) $(srctree)/scripts/lpf_kernel_matrix_build.sh
	@echo ""
	@echo "==================================================================="
	@echo "Kernel module matrix build completed successfully!"
	@echo "==================================================================="
	@echo ""

PHONY += _modules_check_environment
_modules_check_environment:
	@if [ -z "$(strip $(MODULES_LIST))" ]; then \
		echo ""; \
		echo "==================================================================="; \
		echo "ERROR: No kernel modules are enabled in the current configuration."; \
		echo "==================================================================="; \
		echo ""; \
		echo "Enable CONFIG_OSAL, CONFIG_LPF_CORE, and/or CONFIG_LPF_CONFIGS before invoking make modules."; \
		echo "For example, run make menuconfig or load a defconfig that enables"; \
		echo "the kernel modules you want to build."; \
		echo "==================================================================="; \
		echo ""; \
		exit 1; \
	fi
	@if [ ! -f "$(KERNEL_SRC)/Makefile" ]; then \
		echo ""; \
		echo "==================================================================="; \
		echo "ERROR: Kernel build tree not found!"; \
		echo "==================================================================="; \
		echo ""; \
		echo "Kernel source/build dir: $(KERNEL_SRC)"; \
		echo "Expected: a kernel tree with a top-level Makefile"; \
		echo ""; \
		echo "Set KERNEL_SRC=/path/to/linux-build-tree when invoking make modules."; \
		echo "==================================================================="; \
		echo ""; \
		exit 1; \
	fi
	@if [ ! -f "$(MODULES_SRC_DIR)/Makefile" ]; then \
		echo ""; \
		echo "==================================================================="; \
		echo "ERROR: Kernel module Makefile missing!"; \
		echo "==================================================================="; \
		echo ""; \
		echo "Module directory: $(MODULES_SRC_DIR)"; \
		echo ""; \
		echo "The kernel module source tree has not been created correctly."; \
		echo "==================================================================="; \
		echo ""; \
		exit 1; \
	fi

PHONY += _modules_prepare
_modules_prepare:
	$(Q)mkdir -p $(MODULES_OUTPUT_DIR)

PHONY += _check_config
_check_config:
	@if [ ! -f .config ]; then \
		echo ""; \
		echo "==================================================================="; \
		echo "ERROR: No configuration found!"; \
		echo "==================================================================="; \
		echo ""; \
		echo "Please run a configuration command first:"; \
		echo "  make menuconfig          - Interactive menu-based configuration"; \
		echo "  make <name>_defconfig    - Load a predefined configuration"; \
		echo ""; \
		echo "Available configurations:"; \
		echo "  make list                - List all available defconfigs"; \
		echo "==================================================================="; \
		echo ""; \
		exit 1; \
	fi

PHONY += _validate_config
_validate_config:
	@if ! grep -q "CONFIG_PROJECT_NAME=" .config 2>/dev/null; then \
		echo ""; \
		echo "==================================================================="; \
		echo "WARNING: Configuration may be incomplete"; \
		echo "==================================================================="; \
		echo ""; \
		echo "CONFIG_PROJECT_NAME not found in .config"; \
		echo "This may indicate the configuration was not properly generated."; \
		echo ""; \
		echo "Try running:"; \
		echo "  make syncconfig          - Regenerate configuration"; \
		echo "  make <name>_defconfig    - Load a fresh configuration"; \
		echo "==================================================================="; \
		echo ""; \
	fi

PHONY += _cmake_configure
_cmake_configure:
	$(Q)if [ ! -f "$(BUILD_DIR)/Makefile" ] || \
		[ ! -f "$(BUILD_DIR)/CMakeCache.txt" ] || \
		[ .config -nt "$(BUILD_DIR)/CMakeCache.txt" ] || \
		[ include/generated/gen_autoconf.h -nt "$(BUILD_DIR)/CMakeCache.txt" ]; then \
		echo "  CMAKE    Configuring build system"; \
		mkdir -p $(BUILD_DIR); \
		cd $(BUILD_DIR) && $(CMAKE) \
			-DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) \
			-DCMAKE_INSTALL_PREFIX=$(CMAKE_INSTALL_PREFIX) \
			-DCONFIG_FILE=$(abspath $(CURDIR)/.config) \
			-DAUTOCONF_H=$(abspath $(CURDIR)/include/generated/gen_autoconf.h) \
			$(if $(CMAKE_TOOLCHAIN_FILE),-DCMAKE_TOOLCHAIN_FILE=$(CMAKE_TOOLCHAIN_FILE)) \
			$(if $(INSTALL_DEVELOPMENT_HEADERS),-DINSTALL_DEVELOPMENT_HEADERS=$(INSTALL_DEVELOPMENT_HEADERS)) \
			$(CMAKE_EXTRA_FLAGS) \
			$(CURDIR) || { \
				echo ""; \
				echo "==================================================================="; \
				echo "ERROR: CMake configuration failed!"; \
				echo "==================================================================="; \
				echo ""; \
				echo "This usually means:"; \
				echo "  - Missing dependencies (cmake, gcc, etc.)"; \
				echo "  - Invalid .config file"; \
				echo "  - Toolchain issues"; \
				echo ""; \
				echo "Check the error messages above for details."; \
				echo "==================================================================="; \
				echo ""; \
				exit 1; \
			}; \
	fi

endif # ifeq ($(config-targets),1)
endif # ifeq ($(mixed-targets),1)

# ===========================================================================
# Cleaning targets

PHONY += clean distclean mrproper

# clean: Remove build artifacts
clean:
	@echo "  CLEAN   build artifacts"
	$(Q)rm -rf $(BUILD_DIR)

# distclean: Remove build artifacts and configuration
distclean: clean
	@echo "  CLEAN   configuration"
	$(Q)rm -f .config .config.old .kconfig.d
	$(Q)rm -rf include

# mrproper: Remove everything including kconfig tools
mrproper: distclean
	@echo "  CLEAN   kconfig tools"
	$(Q)$(MAKE) -C scripts/basic \
		srctree=$(abspath $(srctree)) \
		objtree=$(abspath $(objtree)) \
		srcroot=$(abspath $(srctree)) \
		clean
	$(Q)$(MAKE) -C $(KCONFIG_DIR) \
		srctree=$(abspath $(srctree)) \
		objtree=$(abspath $(objtree)) \
		srcroot=$(abspath $(srctree)) \
		clean

# ===========================================================================
# Installation target

PHONY += install
install:
	@if [ ! -d "$(BUILD_DIR)" ] || [ ! -f "$(BUILD_DIR)/Makefile" ]; then \
		echo ""; \
		echo "===================================================================";\
		echo "ERROR: Build directory not found or not configured!";\
		echo "===================================================================";\
		echo "Build directory: $(BUILD_DIR)";\
		echo "";\
		echo "Please run 'make libs' or 'make all' first to build the libraries.";\
		echo "===================================================================";\
		echo "";\
		exit 1;\
	fi
	@echo "  INSTALL $(if $(DESTDIR),$(DESTDIR))$(CMAKE_INSTALL_PREFIX)"
	$(Q)$(MAKE) -C $(BUILD_DIR) install \
		$(if $(DESTDIR),DESTDIR=$(DESTDIR)) \
		$(if $(INSTALL_DEVELOPMENT_HEADERS),INSTALL_DEVELOPMENT_HEADERS=$(INSTALL_DEVELOPMENT_HEADERS))
	@echo ""
	@echo "==================================================================="
	@echo "Installation completed!"
	@echo "==================================================================="
	@echo "Install prefix: $(if $(DESTDIR),$(DESTDIR))$(CMAKE_INSTALL_PREFIX)"
	@echo ""

# Install headers only (using CMake component installation)
PHONY += install_headers
install_headers:
	@if [ ! -d "$(BUILD_DIR)" ] || [ ! -f "$(BUILD_DIR)/Makefile" ]; then \
		echo ""; \
		echo "===================================================================";\
		echo "ERROR: Build directory not found or not configured!";\
		echo "===================================================================";\
		echo "Build directory: $(BUILD_DIR)";\
		echo "";\
		echo "Please run 'make libs' or 'make all' first to configure the build directory.";\
		echo "===================================================================";\
		echo "";\
		exit 1;\
	fi
	@echo "  INSTALL_HEADERS $(if $(DESTDIR),$(DESTDIR))$(CMAKE_INSTALL_PREFIX)/include"
	$(Q)cd $(BUILD_DIR) && $(CMAKE) \
		-DCMAKE_INSTALL_PREFIX=$(CMAKE_INSTALL_PREFIX) \
		-DCMAKE_INSTALL_COMPONENT=headers \
		$(if $(DESTDIR),-DCMAKE_INSTALL_DO_STRIP=0) \
		-P cmake_install.cmake
	@echo ""
	@echo "==================================================================="
	@echo "Headers installed successfully!"
	@echo "==================================================================="
	@echo "Install prefix: $(if $(DESTDIR),$(DESTDIR))$(CMAKE_INSTALL_PREFIX)/include"
	@echo ""

# ===========================================================================
# Help and information targets

PHONY += help list

help:
	@echo 'LPF Build System'
	@echo '===================================='
	@echo ''
	@echo 'Configuration targets:'
	@echo '  config          - Update current config utilising a line-oriented program'
	@echo '  menuconfig      - Update current config utilising a menu based program'
	@echo '  oldconfig       - Update current config utilising a provided .config as base'
	@echo '  syncconfig      - Same as oldconfig, but quietly (internal use)'
	@echo '  randconfig      - New config with random answer to all options'
	@echo '  defconfig       - New config with default answer to all options'
	@echo '  allnoconfig     - New config where all options are answered with no'
	@echo '  allyesconfig    - New config where all options are accepted with yes'
	@echo '  alldefconfig    - New config where all options are set to default'
	@echo '  <board>_defconfig - Load specific board configuration from configs/'
	@echo ''
	@echo 'Build targets:'
	@echo '  all             - Build libraries and kernel modules (default)'
	@echo '  libs            - Build userspace libraries via CMake'
	@echo '  modules         - Build kernel modules via kbuild'
	@echo '  mock-modules-smoke'
	@echo '                  - Load/unload mock kernel modules and LPF selftests'
	@echo '  kernel-matrix   - Build kernel modules against KERNEL_SRC_LIST'
	@echo '  tests           - Build and run LPF test targets'
	@echo '  install         - Install binaries and libraries'
	@echo '  install_headers - Install development headers only'
	@echo '  clean           - Remove build artifacts'
	@echo '  distclean       - Remove build artifacts and configuration'
	@echo '  mrproper        - Same as distclean'
	@echo ''
	@echo 'Information targets:'
	@echo '  help            - Display this help message'
	@echo '  list            - List all available defconfigs'
	@echo '  version         - Display version information'
	@echo ''
	@echo 'Build options:'
	@echo '  V=0|1           - 0: quiet build (default), 1: verbose'
	@echo '  BUILD_DIR=<dir> - Use custom build directory (default: _build)'
	@echo '  TEST_BUILD_DIR=<dir> - Use custom test build directory (default: _build/tests)'
	@echo '  KERNEL_SRC=<dir> - Kernel build tree for modules target'
	@echo '  KERNEL_SRC_LIST="<dirs>" - Space-separated kernel build trees for kernel-matrix'
	@echo '  MODULES_BUILD_DIR=<dir> - Output directory for module artifacts'
	@echo '  MODULES_SRC_DIR=<dir> - Kernel module source directory'
	@echo '  LPF_KERNEL_MATRIX_DEFCONFIG=<name> - Defconfig for kernel-matrix'
	@echo '  LPF_KERNEL_MATRIX_BUILD_ROOT=<dir> - Output root for kernel-matrix'
	@echo '  MODULES_LIST="<list>" - Expected modules derived from enabled kernel module options'
	@echo '  CMAKE_BUILD_TYPE=<type>'
	@echo '                  - Set build type: Debug, Release, RelWithDebInfo, MinSizeRel'
	@echo '  CMAKE_INSTALL_PREFIX=<path>'
	@echo '                  - Set installation prefix (default: /usr/local)'
	@echo '  DESTDIR=<path>  - Stage installation to alternate root'
	@echo '  CMAKE_TOOLCHAIN_FILE=<file>'
	@echo '                  - Specify CMake toolchain for cross-compilation'
	@echo ''
	@echo 'Available defconfigs:'
	@for subdir in $(sort $(notdir $(wildcard $(srctree)/configs/*/))); do \
		echo ""; \
		echo "  $$subdir configurations:"; \
		for config in $(sort $(notdir $(wildcard $(srctree)/configs/$$subdir/*_defconfig))); do \
			printf "    %-30s\n" $$config; \
		done; \
	done
	@echo ''
	@echo 'Examples:'
	@echo '  # Standard workflow'
	@echo '  make ubuntu_x86_modules_defconfig'
	@echo '  make all'
	@echo '  make install DESTDIR=/tmp/staging'
	@echo ''
	@echo '  # Interactive configuration'
	@echo '  make menuconfig'
	@echo '  make'
	@echo ''
	@echo '  # Verbose build'
	@echo '  make V=1 all'
	@echo ''
	@echo '  # Cross-compilation for Buildroot'
	@echo '  make ubuntu_x86_modules_defconfig'
	@echo '  make CMAKE_TOOLCHAIN_FILE=$$BUILDROOT/host/share/buildroot/toolchainfile.cmake'
	@echo '  make install DESTDIR=$$BUILDROOT/target'
	@echo ''
	@echo 'For more information, see docs/BUILD_SYSTEM.md'

list:
	@echo 'Available defconfigs:'
	@echo ''
	@cd $(srctree)/configs && for subdir in */; do \
		subdir=$${subdir%/}; \
		echo "$$subdir configurations:"; \
		for config in $$subdir/*_defconfig; do \
			config=$$(basename $$config); \
			printf "  %-35s\n" $$config; \
		done; \
		echo ""; \
	done

PHONY += version
version:
	@echo "LPF - Linux Peripheral Framework"
	@echo "Version: $(VERSION_STRING)"
	@if [ -d .git ]; then \
		echo "Git commit: $$(git rev-parse --short HEAD 2>/dev/null || echo 'unknown')"; \
		echo "Git branch: $$(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo 'unknown')"; \
	fi

# ===========================================================================

# Declare the contents of the .PHONY variable as phony.  We keep that
# information in a variable so we can use it in if_changed and friends.
.PHONY: $(PHONY)

FORCE:
.PHONY: FORCE

endif # ifeq ($(skip-makefile),)

.PHONY: savedefconfig
savedefconfig:
	@if [ ! -f .config ]; then \
		echo "Error: No .config found. Run a defconfig first."; \
		exit 1; \
	fi
	@if [ -z "$(DEFCONFIG)" ]; then \
		echo "Usage: make savedefconfig DEFCONFIG=configs/xxx_defconfig"; \
		exit 1; \
	fi
	@mkdir -p $$(dirname $(DEFCONFIG))
	$(Q)$(KCONFIG_DIR)/conf --savedefconfig=$(DEFCONFIG) Config.in
	@echo ""
	@echo "==================================================================="
	@echo "Configuration saved to $(DEFCONFIG)"
	@echo "==================================================================="

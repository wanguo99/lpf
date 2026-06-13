# ES-Middleware Makefile
# Based on busybox configuration system

VERSION = 1
PATCHLEVEL = 0
SUBLEVEL = 0
EXTRAVERSION =
NAME = ES-Middleware

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
# For ES-Middleware, we use BUILD_DIR variable for CMake output directory.
# This is simpler than full out-of-tree build support.

# Build directory (for CMake build artifacts)
BUILD_DIR ?= _build

# Kconfig directory
KCONFIG_DIR := scripts/kconfig

# Normalize BUILD_DIR
override BUILD_DIR := $(patsubst %/,%,$(BUILD_DIR))

# CMake configuration
CMAKE ?= cmake
CMAKE_BUILD_TYPE ?= Release
CMAKE_INSTALL_PREFIX ?= /usr
DESTDIR ?=

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

export srctree objtree VPATH TOPDIR

# SHELL used by kbuild
CONFIG_SHELL := $(shell if [ -x "$$BASH" ]; then echo $$BASH; \
	  else if [ -x /bin/bash ]; then echo /bin/bash; \
	  else echo sh; fi ; fi)

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

HOSTCC  	= gcc
HOSTCXX  	= g++
HOSTCFLAGS	:=
HOSTCXXFLAGS	:=

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
	if [ "$(KBUILD_VERBOSE)" = "1" ]; then \
		$(MAKE) $(build)=scripts/kconfig $$target; \
	else \
		logfile=/tmp/kconfig.$$$$.log; \
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
		status=$$?; \
		if [ $$status -ne 0 ]; then \
			if [ -f $$logfile ]; then \
				cat $$logfile >&2; \
				rm -f $$logfile; \
			fi; \
			exit $$status; \
		fi; \
		rm -f $$logfile; \
	fi
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

scripts_basic: include/autoconf.h include/version.h

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

# If .config is newer than include/autoconf.h, someone tinkered
# with it and forgot to run make oldconfig.
# If kconfig.d is missing then we are probably in a cleaned tree so
# we execute the config step to be sure to catch updated Config.in files
include/autoconf.h: .config $(wildcard .kconfig.d)
	$(Q)$(MAKE) -f $(srctree)/Makefile silentoldconfig

# Generate version.h with build information
include/version.h: .config FORCE
	$(Q)$(srctree)/scripts/gen_version.sh

else
# Dummy target needed, because used as prerequisite
include/autoconf.h: ;
include/version.h: ;
endif

# The all: target is the default when no target is given on the command line.
# It requires .config to exist, otherwise it will fail with an error message.
PHONY += all
all: _check_config include/autoconf.h include/version.h _cmake_configure
	@echo ""
	@echo "==================================================================="
	@echo "ES-Middleware Build System"
	@echo "==================================================================="
	@echo ""
	@echo "Configuration loaded from: $(CURDIR)/.config"
	@echo "Building with CMake..."
	@echo ""
	@echo "  BUILD    ES-Middleware"
	$(Q)$(MAKE) -C $(BUILD_DIR) $(PARALLEL_BUILD)
	@echo ""
	@echo "==================================================================="
	@echo "Build completed successfully!"
	@echo "==================================================================="
	@echo "Output directory: $(BUILD_DIR)"
	@echo "Binaries: $(BUILD_DIR)/bin/"
	@echo "Libraries: $(BUILD_DIR)/lib/"
	@echo ""

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

PHONY += _cmake_configure
_cmake_configure:
	$(Q)if [ ! -f "$(BUILD_DIR)/Makefile" ]; then \
		echo "  CMAKE    $(BUILD_DIR)"; \
		mkdir -p $(BUILD_DIR); \
		cd $(BUILD_DIR) && $(CMAKE) \
			-DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) \
			-DCMAKE_INSTALL_PREFIX=$(CMAKE_INSTALL_PREFIX) \
			-DCONFIG_FILE=$(abspath $(CURDIR)/.config) \
			-DAUTOCONF_H=$(abspath $(CURDIR)/include/autoconf.h) \
			$(if $(CMAKE_TOOLCHAIN_FILE),-DCMAKE_TOOLCHAIN_FILE=$(CMAKE_TOOLCHAIN_FILE)) \
			$(CMAKE_EXTRA_FLAGS) \
			$(CURDIR); \
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
	$(Q)rm -f include/autoconf.h

# distclean: Remove build artifacts and configuration
distclean: clean
	@echo "  CLEAN   configuration"
	$(Q)rm -f .config .config.old .kconfig.d
	$(Q)rm -rf include/config include/generated
	$(Q)rm -f include/autoconf.h include/version.h

# mrproper: Remove everything including kconfig tools
mrproper: distclean
	@echo "  CLEAN   kconfig tools"
	$(Q)$(MAKE) -C $(KCONFIG_DIR) clean

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
		echo "Please run 'make all' first to build the project.";\
		echo "===================================================================";\
		echo "";\
		exit 1;\
	fi
	@echo "  INSTALL $(if $(DESTDIR),$(DESTDIR))$(CMAKE_INSTALL_PREFIX)"
	$(Q)$(MAKE) -C $(BUILD_DIR) install $(if $(DESTDIR),DESTDIR=$(DESTDIR))
	@echo ""
	@echo "==================================================================="
	@echo "Installation completed!"
	@echo "==================================================================="
	@echo "Install prefix: $(if $(DESTDIR),$(DESTDIR))$(CMAKE_INSTALL_PREFIX)"
	@echo ""

# ===========================================================================
# Help and information targets

PHONY += help list

help:
	@echo 'ES-Middleware Configuration System'
	@echo '===================================='
	@echo ''
	@echo 'Configuration targets:'
	@echo '  config          - Update current config utilising a line-oriented program'
	@echo '  menuconfig      - Update current config utilising a menu based program'
	@echo '  oldconfig       - Update current config utilising a provided .config as base'
	@echo '  silentoldconfig - Same as oldconfig, but quietly'
	@echo '  randconfig      - New config with random answer to all options'
	@echo '  defconfig       - New config with default answer to all options'
	@echo '  allnoconfig     - New config where all options are answered with no'
	@echo '  allyesconfig    - New config where all options are accepted with yes'
	@echo '  alldefconfig    - New config where all options are set to default'
	@echo '  <board>_defconfig - Load specific board configuration from configs/'
	@echo ''
	@echo 'Build targets:'
	@echo '  all             - Build all configured targets (default)'
	@echo '  install         - Install binaries and libraries'
	@echo '  clean           - Remove build artifacts'
	@echo '  distclean       - Remove build artifacts and configuration'
	@echo '  mrproper        - Same as distclean'
	@echo ''
	@echo 'Options:'
	@echo '  V=0|1           - 0: quiet build (default), 1: verbose'
	@echo '  BUILD_DIR=<dir> - Use custom build directory (default: _build)'
	@echo '  CMAKE_BUILD_TYPE=<type>'
	@echo '                  - Set build type: Debug, Release, RelWithDebInfo, MinSizeRel'
	@echo '  CMAKE_INSTALL_PREFIX=<path>'
	@echo '                  - Set installation prefix (default: /usr)'
	@echo '  DESTDIR=<path>  - Stage installation to alternate root'
	@echo '  CMAKE_TOOLCHAIN_FILE=<file>'
	@echo '                  - Specify CMake toolchain for cross-compilation'
	@echo ''
	@echo 'Available defconfigs (make list):'
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
	@echo '  make tests_x86_full_defconfig'
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
	@cd $(srctree)/configs && for subdir in */; do \
		subdir=$${subdir%/}; \
		echo "$$subdir configurations:"; \
		for config in $$subdir/*_defconfig; do \
			config=$$(basename $$config); \
			printf "  %-35s\n" $$config; \
		done; \
		echo ""; \
	done

# ===========================================================================

# Declare the contents of the .PHONY variable as phony.  We keep that
# information in a variable so we can use it in if_changed and friends.
.PHONY: $(PHONY)

FORCE:
.PHONY: FORCE

endif # ifeq ($(skip-makefile),)

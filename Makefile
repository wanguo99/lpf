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
# To locate output files in a separate directory two syntaxes are supported.
# In both cases the working directory must be the root of the kernel src.
# 1) O=
# Use "make O=dir/to/store/output/files/"
#
# 2) Set KBUILD_OUTPUT
# Set the environment variable KBUILD_OUTPUT to point to the directory
# where the output files shall be placed.
# export KBUILD_OUTPUT=dir/to/store/output/files/
# make
#
# The O= assignment takes precedence over the KBUILD_OUTPUT environment
# variable.

# KBUILD_SRC is set on invocation of make in OBJ directory
# KBUILD_SRC is not intended to be used by the regular user (for now)
ifeq ($(KBUILD_SRC),)

# OK, Make called in directory where kernel src resides
# Do we want to locate output files in a separate directory?
ifdef O
  ifeq ("$(origin O)", "command line")
    KBUILD_OUTPUT := $(O)
  endif
endif

# That's our default target when none is given on the command line
PHONY := _all
_all:

ifneq ($(KBUILD_OUTPUT),)
# Invoke a second make in the output directory, passing relevant variables
# check that the output directory actually exists
saved-output := $(KBUILD_OUTPUT)
KBUILD_OUTPUT := $(shell cd $(KBUILD_OUTPUT) && /bin/pwd)
$(if $(KBUILD_OUTPUT),, \
     $(error output directory "$(saved-output)" does not exist))

PHONY += $(MAKECMDGOALS)

$(filter-out _all,$(MAKECMDGOALS)) _all:
	$(if $(KBUILD_VERBOSE:1=),@)$(MAKE) -C $(KBUILD_OUTPUT) \
	KBUILD_SRC=$(CURDIR) \
	-f $(CURDIR)/Makefile $@

# Leave processing to above invocation of make
skip-makefile := 1
endif # ifneq ($(KBUILD_OUTPUT),)
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

%config: scripts_basic FORCE
	$(Q)mkdir -p include
	$(Q)$(MAKE) $(build)=scripts/kconfig $@

else
# ===========================================================================
# Build targets only - this includes all build targets
# targets and others. In general all targets except *config targets.

scripts_basic: include/autoconf.h

ifeq ($(dot-config),1)
# In this section, we need .config

# Read in dependencies to all Kconfig* files, make sure to run
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
# we execute the config step to be sure to catch updated Kconfig files
include/autoconf.h: .kconfig.d .config FORCE
	$(Q)$(MAKE) -f $(srctree)/Makefile silentoldconfig

else
# Dummy target needed, because used as prerequisite
include/autoconf.h: ;
endif

# The all: target is the default when no target is given on the command line.
all: include/autoconf.h
	@echo ""
	@echo "Configuration is ready. Build with CMake:"
	@echo "  mkdir -p _build && cd _build"
	@echo "  cmake .."
	@echo "  make -j\$$(nproc)"
	@echo ""

endif # ifeq ($(config-targets),1)
endif # ifeq ($(mixed-targets),1)

# ===========================================================================
# Cleaning targets

PHONY += clean mrproper distclean
clean:
	@echo "  CLEAN   build artifacts"
	$(Q)rm -rf _build
	$(Q)rm -f include/autoconf.h

mrproper distclean: clean
	@echo "  CLEAN   configuration"
	$(Q)rm -f .config .config.old .kconfig.d
	$(Q)rm -rf include/config include/generated

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
	@echo '  all             - Show build instructions (default)'
	@echo '  clean           - Remove build artifacts'
	@echo '  distclean       - Remove build artifacts and configuration'
	@echo '  mrproper        - Same as distclean'
	@echo ''
	@echo 'Options:'
	@echo '  V=0|1           - 0: quiet build (default), 1: verbose'
	@echo '  O=<dir>         - Use custom build directory'
	@echo ''
	@echo 'Available defconfigs (make list):'
	@for config in $(sort $(notdir $(wildcard $(srctree)/configs/*_defconfig))); do \
		printf "  %-30s\n" $$config; \
	done
	@echo ''
	@echo 'Examples:'
	@echo '  make menuconfig'
	@echo '  make ccm_h200_100p_am625_debug_defconfig'
	@echo '  make'

list:
	@echo 'Available defconfigs:'
	@echo ''
	@for config in $(sort $(notdir $(wildcard $(srctree)/configs/*_defconfig))); do \
		echo "  $$config"; \
	done

# ===========================================================================

# Declare the contents of the .PHONY variable as phony.  We keep that
# information in a variable so we can use it in if_changed and friends.
.PHONY: $(PHONY)

FORCE:
.PHONY: FORCE

endif # ifeq ($(skip-makefile),)

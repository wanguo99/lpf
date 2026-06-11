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
BUILD_DIR := _build
CONFIGS_DIR := configs
CMAKE ?= cmake

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
.PHONY: clean distclean mrproper list _build_internal

# Default goal
.DEFAULT_GOAL := all

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
	@if [ ! -f "$(BUILD_DIR)/Makefile" ]; then \
		echo "  CMAKE     $(BUILD_DIR)"; \
		mkdir -p $(BUILD_DIR); \
		cd $(BUILD_DIR) && $(CMAKE) .. >/dev/null; \
	fi
	@$(MAKE) -C $(BUILD_DIR) $(PARALLEL_BUILD)

# Help target
help:
	@echo 'Cleaning targets:'
	@echo '  clean              - Remove build artifacts (keep configuration)'
	@echo '  distclean          - Remove all generated files'
	@echo '  mrproper           - Same as distclean'
	@echo ''
	@echo 'Configuration targets:'
	@echo '  defconfig          - Load default configuration'
	@echo '  menuconfig         - Interactive ncurses-based configuration'
	@echo '  nconfig            - Interactive ncurses-based alternative'
	@echo '  oldconfig          - Update configuration with new options'
	@echo '  savedefconfig      - Save minimal configuration to defconfig'
	@echo '  list               - List available defconfigs'
	@echo ''
	@for config in $(sort $(notdir $(wildcard $(CONFIGS_DIR)/*_defconfig))); do \
		printf "  %-30s- Build for %s\\n" $$config "$${config%_defconfig}"; \
	done
	@echo ''
	@echo 'Build targets:'
	@echo '  all                - Build all targets (default)'
	@echo '  (no target)        - Same as all'
	@echo ''
	@echo 'Other targets:'
	@echo '  help               - This help text'
	@echo ''
	@echo 'Execute "make" or "make all" to build.'
	@echo 'Execute "make <board>_defconfig" first to configure.'
	@echo ''
	@echo 'Example:'
	@echo '  make tests_x86_minimal_defconfig'
	@echo '  make -j$$(nproc)'

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

# Interactive configuration (menuconfig)
menuconfig:
	@if [ ! -f "$(BUILD_DIR)/Makefile" ]; then \
		echo "  CMAKE     $(BUILD_DIR)"; \
		mkdir -p $(BUILD_DIR); \
		cd $(BUILD_DIR) && $(CMAKE) .. >/dev/null; \
	fi
	@$(MAKE) -C $(BUILD_DIR) menuconfig
	@echo "  SYNC      $(KCONFIG_CONFIG)"

# Interactive configuration (nconfig)
nconfig:
	@if [ ! -f "$(BUILD_DIR)/Makefile" ]; then \
		echo "  CMAKE     $(BUILD_DIR)"; \
		mkdir -p $(BUILD_DIR); \
		cd $(BUILD_DIR) && $(CMAKE) .. >/dev/null; \
	fi
	@$(MAKE) -C $(BUILD_DIR) nconfig
	@echo "  SYNC      $(KCONFIG_CONFIG)"

# Update old configuration
oldconfig:
	@if [ ! -f "$(BUILD_DIR)/Makefile" ]; then \
		echo "  CMAKE     $(BUILD_DIR)"; \
		mkdir -p $(BUILD_DIR); \
		cd $(BUILD_DIR) && $(CMAKE) .. >/dev/null; \
	fi
	@$(MAKE) -C $(BUILD_DIR) oldconfig
	@echo "  SYNC      $(KCONFIG_CONFIG)"

# Save minimal configuration
savedefconfig:
	@if [ ! -f "$(BUILD_DIR)/Makefile" ]; then \
		echo "  CMAKE     $(BUILD_DIR)"; \
		mkdir -p $(BUILD_DIR); \
		cd $(BUILD_DIR) && $(CMAKE) .. >/dev/null; \
	fi
	@$(MAKE) -C $(BUILD_DIR) savedefconfig
	@echo "  SAVE      $(BUILD_DIR)/defconfig"
	@echo ''
	@echo 'Minimal configuration saved to $(BUILD_DIR)/defconfig'
	@echo 'To save as a board defconfig:'
	@echo '  cp $(BUILD_DIR)/defconfig $(CONFIGS_DIR)/<board>_defconfig'

# Clean build artifacts
clean:
	@echo "  CLEAN     $(BUILD_DIR)"
	@rm -rf $(BUILD_DIR)

# Complete clean
distclean mrproper: clean
	@echo "  CLEAN     $(KCONFIG_CONFIG)"
	@rm -f $(KCONFIG_CONFIG)

# Catch-all: forward to CMake build system
%:
	@if [ ! -f "$(KCONFIG_CONFIG)" ]; then \
		echo "***"; \
		echo "*** Configuration file '$(KCONFIG_CONFIG)' not found!"; \
		echo "***"; \
		echo "*** Please run 'make <board>_defconfig' first."; \
		echo "*** Try 'make help' for available defconfigs."; \
		echo "***"; \
		exit 1; \
	fi
	@if [ ! -f "$(BUILD_DIR)/Makefile" ]; then \
		echo "  CMAKE     $(BUILD_DIR)"; \
		mkdir -p $(BUILD_DIR); \
		cd $(BUILD_DIR) && $(CMAKE) .. >/dev/null; \
	fi
	@$(MAKE) -C $(BUILD_DIR) $@

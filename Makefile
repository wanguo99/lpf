# ============================================================================
# EMS 顶层 Makefile - Linux 内核风格
# ============================================================================

# 输出目录处理（支持 O= 和 OUTPUT= 两种形式）
ifeq ($(origin O), command line)
  OUTPUT_DIR := $(O)
endif
ifeq ($(origin OUTPUT), command line)
  OUTPUT_DIR := $(OUTPUT)
endif

# 默认输出到源码目录（Linux 内核风格）
OUTPUT_DIR ?= $(CURDIR)

# 转换为绝对路径
override OUTPUT_DIR := $(abspath $(OUTPUT_DIR))

# 导出给子 Makefile
export OUTPUT_DIR
export srctree := $(CURDIR)
export objtree := $(OUTPUT_DIR)

# ============================================================================
# Kconfig 配置
# ============================================================================
# 配置文件路径（支持输出目录重定向）
KCONFIG_CONFIG := $(OUTPUT_DIR)/.config
CONFIG_DIR := $(OUTPUT_DIR)/include/config
GENERATED_DIR := $(OUTPUT_DIR)/include/generated
AUTOCONF_H := $(GENERATED_DIR)/autoconf.h

export KCONFIG_CONFIG
export GENERATED_DIR

# Kconfig 工具
CONF := scripts/kconfig/conf
MCONF := scripts/kconfig/mconf

# ============================================================================
# 主要目标
# ============================================================================
.PHONY: all core products clean distclean mrproper install install-all headers_install help

# 默认目标
all: core products

# 核心模块（按依赖顺序）
core: $(AUTOCONF_H)
	@echo "Building core modules..."
	@$(MAKE) -f $(srctree)/core/Makefile

# 产品模块（依赖所有核心模块）
products: core
	@echo "Building product modules..."
	@$(MAKE) -f $(srctree)/products/Makefile

# ============================================================================
# Kconfig 配置目标
# ============================================================================
.PHONY: menuconfig config defconfig savedefconfig oldconfig syncconfig

menuconfig: $(MCONF)
	@mkdir -p $(CONFIG_DIR) $(GENERATED_DIR)
	@echo "Starting menuconfig..."
	@KCONFIG_CONFIG=$(KCONFIG_CONFIG) \
		KCONFIG_AUTOHEADER=$(AUTOCONF_H) \
		KCONFIG_AUTOCONFIG=$(CONFIG_DIR)/auto.conf \
		$< Kconfig
	@$(MAKE) syncconfig

config: $(CONF)
	@mkdir -p $(CONFIG_DIR) $(GENERATED_DIR)
	@KCONFIG_CONFIG=$(KCONFIG_CONFIG) \
		KCONFIG_AUTOHEADER=$(AUTOCONF_H) \
		KCONFIG_AUTOCONFIG=$(CONFIG_DIR)/auto.conf \
		$< --oldaskconfig Kconfig
	@$(MAKE) syncconfig

defconfig: $(CONF)
	@mkdir -p $(CONFIG_DIR) $(GENERATED_DIR)
	@echo "Loading default configuration..."
	@KCONFIG_CONFIG=$(KCONFIG_CONFIG) \
		KCONFIG_AUTOHEADER=$(AUTOCONF_H) \
		KCONFIG_AUTOCONFIG=$(CONFIG_DIR)/auto.conf \
		$< --defconfig=defconfig Kconfig
	@$(MAKE) syncconfig

%_defconfig: configs/%_defconfig $(CONF)
	@mkdir -p $(CONFIG_DIR) $(GENERATED_DIR)
	@echo "Loading configuration: $*"
	@KCONFIG_CONFIG=$(KCONFIG_CONFIG) \
		KCONFIG_AUTOHEADER=$(AUTOCONF_H) \
		KCONFIG_AUTOCONFIG=$(CONFIG_DIR)/auto.conf \
		$(CONF) --defconfig=$< Kconfig
	@$(MAKE) syncconfig

savedefconfig: $(CONF)
	@echo "Saving minimal configuration to defconfig..."
	@KCONFIG_CONFIG=$(KCONFIG_CONFIG) \
		KCONFIG_AUTOHEADER=$(AUTOCONF_H) \
		KCONFIG_AUTOCONFIG=$(CONFIG_DIR)/auto.conf \
		$< --savedefconfig=defconfig Kconfig
	@echo "Configuration saved."

oldconfig: $(CONF)
	@mkdir -p $(CONFIG_DIR) $(GENERATED_DIR)
	@KCONFIG_CONFIG=$(KCONFIG_CONFIG) \
		KCONFIG_AUTOHEADER=$(AUTOCONF_H) \
		KCONFIG_AUTOCONFIG=$(CONFIG_DIR)/auto.conf \
		$< --oldconfig Kconfig
	@$(MAKE) syncconfig

syncconfig: $(CONF)
	@mkdir -p $(CONFIG_DIR) $(GENERATED_DIR)
	@KCONFIG_CONFIG=$(KCONFIG_CONFIG) \
		KCONFIG_AUTOHEADER=$(AUTOCONF_H) \
		KCONFIG_AUTOCONFIG=$(CONFIG_DIR)/auto.conf \
		$< --syncconfig Kconfig
	@echo "Configuration synchronized."

# ============================================================================
# 构建 Kconfig 工具
# ============================================================================
$(CONF) $(MCONF):
	@echo "Building kconfig tools..."
	@$(MAKE) -C scripts/kconfig $(notdir $@)

# ============================================================================
# 清理目标
# ============================================================================
clean:
	@echo "Cleaning build artifacts..."
	@if [ "$(OUTPUT_DIR)" != "$(CURDIR)" ]; then \
		rm -rf $(OUTPUT_DIR)/core $(OUTPUT_DIR)/products; \
		rm -rf $(OUTPUT_DIR)/lib $(OUTPUT_DIR)/bin; \
	else \
		find core products -type f \( -name '*.o' -o -name '*.so' -o -name '*.a' \) -delete 2>/dev/null || true; \
		rm -rf lib bin; \
	fi
	@echo "Clean complete."

distclean: clean
	@echo "Cleaning configuration..."
	@rm -f $(KCONFIG_CONFIG) $(KCONFIG_CONFIG).old
	@rm -rf $(CONFIG_DIR) $(GENERATED_DIR)
	@echo "Distclean complete."

mrproper: distclean
	@echo "Cleaning kconfig tools..."
	@$(MAKE) -C scripts/kconfig clean
	@echo "Mrproper complete."

# ============================================================================
# 安装目标
# ============================================================================
install:
	@$(MAKE) -f $(srctree)/scripts/install.mk install

headers_install:
	@$(MAKE) -f $(srctree)/scripts/install.mk headers_install

install-all:
	@$(MAKE) -f $(srctree)/scripts/install.mk install-all

# ============================================================================
# 帮助信息
# ============================================================================
help:
	@echo "EMS Build System - Linux Kernel Style"
	@echo ""
	@echo "Usage: make [O=<output_dir>] [target]"
	@echo ""
	@echo "Configuration targets:"
	@echo "  menuconfig       - Interactive configuration (ncurses)"
	@echo "  config           - Text-based configuration"
	@echo "  defconfig        - Load default configuration"
	@echo "  <name>_defconfig - Load preset from configs/<name>_defconfig"
	@echo "  savedefconfig    - Save minimal config to defconfig"
	@echo "  syncconfig       - Synchronize and generate headers"
	@echo ""
	@echo "Build targets:"
	@echo "  all              - Build all modules (default)"
	@echo "  core             - Build core modules only"
	@echo "  products         - Build product modules"
	@echo ""
	@echo "Install targets:"
	@echo "  install          - Install libraries and binaries"
	@echo "  headers_install  - Install header files (kernel style)"
	@echo "  install-all      - Install everything (libs + bins + headers)"
	@echo ""
	@echo "Clean targets:"
	@echo "  clean            - Remove build artifacts"
	@echo "  distclean        - Remove build and config"
	@echo "  mrproper         - Full clean including tools"
	@echo ""
	@echo "Examples:"
	@echo "  make menuconfig"
	@echo "  make -j$$(nproc)"
	@echo "  make O=/tmp/build defconfig"
	@echo "  make O=/tmp/build -j8"
	@echo "  make install INSTALL_PREFIX=/opt/ems"
	@echo "  make headers_install INSTALL_PREFIX=/opt/ems"
	@echo "  make install-all DESTDIR=/staging INSTALL_PREFIX=/usr"
	@echo ""
	@echo "Available presets:"
	@ls -1 configs/*_defconfig 2>/dev/null | sed 's|configs/||' | sed 's|^|  |' || echo "  (none)"

.DEFAULT_GOAL := all

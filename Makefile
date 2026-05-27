# =============================================================================
# EMS Makefile - Kconfig 配置 + CMake 构建
# =============================================================================

.PHONY: all menuconfig defconfig savedefconfig clean help

# 默认目标：提示使用 CMake
all:
	@echo "EMS 项目使用 CMake 构建系统"
	@echo ""
	@echo "配置步骤："
	@echo "  1. 配置功能模块: make menuconfig"
	@echo "  2. 生成构建文件: cmake -B build-cmake"
	@echo "  3. 编译项目:     cmake --build build-cmake -j\$$(nproc)"
	@echo ""
	@echo "快速开始："
	@echo "  make menuconfig && cmake -B build-cmake && cmake --build build-cmake"

# Kconfig 配置工具
KCONFIG_DIR := scripts/kconfig
CONF := $(KCONFIG_DIR)/conf
MCONF := $(KCONFIG_DIR)/mconf

# 构建 Kconfig 工具
$(CONF):
	@$(MAKE) -C $(KCONFIG_DIR) conf

$(MCONF):
	@$(MAKE) -C $(KCONFIG_DIR) mconf

# menuconfig - 图形化配置界面
menuconfig: $(MCONF)
	@$(MCONF) Kconfig

# defconfig - 加载默认配置
defconfig: $(CONF)
	@if [ -f configs/defconfig ]; then \
		$(CONF) --defconfig=configs/defconfig Kconfig; \
	else \
		echo "错误: configs/defconfig 不存在"; \
		exit 1; \
	fi

# 加载指定的 defconfig
%_defconfig: $(CONF)
	@if [ -f configs/$@ ]; then \
		$(CONF) --defconfig=configs/$@ Kconfig; \
	else \
		echo "错误: configs/$@ 不存在"; \
		exit 1; \
	fi

# savedefconfig - 保存最小化配置
savedefconfig: $(CONF)
	@$(CONF) --savedefconfig=defconfig Kconfig
	@echo "配置已保存到 defconfig"

# 清理 Kconfig 工具
clean:
	@$(MAKE) -C $(KCONFIG_DIR) clean
	@rm -f .config .config.old
	@rm -rf include/config include/generated

# 帮助信息
help:
	@echo "EMS 项目 Makefile"
	@echo ""
	@echo "配置目标："
	@echo "  menuconfig          - 图形化配置界面"
	@echo "  defconfig           - 加载默认配置"
	@echo "  xxx_defconfig       - 加载指定配置（如 x86_64_full_defconfig）"
	@echo "  savedefconfig       - 保存最小化配置"
	@echo ""
	@echo "清理目标："
	@echo "  clean               - 清理 Kconfig 工具和配置文件"
	@echo ""
	@echo "构建项目："
	@echo "  cmake -B build-cmake              - 生成构建文件"
	@echo "  cmake --build build-cmake         - 编译项目"
	@echo "  cmake --install build-cmake       - 安装项目"

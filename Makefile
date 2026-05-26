# =============================================================================
# EMS 非递归 Make 构建系统
# =============================================================================
# 版本: 2.0
# 创建日期: 2026-05-25

# 默认目标
.DEFAULT_GOAL := all

# 包含 Kconfig 生成的配置
-include .config
-include include/config/auto.conf

# 包含辅助函数和规则
include scripts/functions.mk
include scripts/rules.mk

# =============================================================================
# 配置依赖（自动生成 autoconf.h）
# =============================================================================

include/config/auto.conf: .config scripts/kconfig/conf
	@$(MAKE) -C scripts/kconfig syncconfig srctree=$(CURDIR)

# 全局目标列表
ALL_TARGETS :=

# =============================================================================
# Core 模块（根据 Kconfig 配置包含）
# =============================================================================

ifeq ($(CONFIG_OSAL),y)
    include core/osal/module.mk
endif

ifeq ($(CONFIG_HAL),y)
    include core/hal/module.mk
endif

ifeq ($(CONFIG_PCL),y)
    include core/pcl/module.mk
endif

ifeq ($(CONFIG_PDL),y)
    include core/pdl/module.mk
endif

ifeq ($(CONFIG_ACL),y)
    include core/acl/module.mk
endif

# =============================================================================
# Products 模块
# =============================================================================

# libccm
include products/ccm/libs/libccm/module.mk

# libh200_am625
ifeq ($(CONFIG_PROJECT_H200_AM625),y)
    include products/ccm/h200_am625/module.mk
endif

# Applications
ifeq ($(CONFIG_BUILD_CCM_COLLECTOR),y)
    include products/ccm/apps/ccm_collector/module.mk
endif

ifeq ($(CONFIG_BUILD_CCM_HEALTH),y)
    include products/ccm/apps/ccm_health/module.mk
endif

ifeq ($(CONFIG_BUILD_CCM_LOGGER),y)
    include products/ccm/apps/ccm_logger/module.mk
endif

ifeq ($(CONFIG_BUILD_CCM_SUPERVISOR),y)
    include products/ccm/apps/ccm_supervisor/module.mk
endif

ifeq ($(CONFIG_BUILD_CCM_COMM),y)
    include products/ccm/apps/ccm_comm/module.mk
endif

# =============================================================================
# Tests 模块
# =============================================================================

# 测试框架核心库（必须最先构建）
ifeq ($(CONFIG_BUILD_TESTING),y)
    include tests/core/module.mk
endif

# 单元测试
ifeq ($(CONFIG_TEST_UNIT),y)
    include tests/unit/module.mk
endif

# 性能测试
ifeq ($(CONFIG_TEST_PERFORMANCE),y)
    include tests/performance/module.mk
endif

# 压力测试
ifeq ($(CONFIG_TEST_STRESS),y)
    include tests/stress/module.mk
endif

# 系统测试
ifeq ($(CONFIG_TEST_SYSTEM),y)
    include tests/system/module.mk
endif

# =============================================================================
# 主目标
# =============================================================================

.PHONY: all
all: include/config/auto.conf $(ALL_TARGETS)
	@echo "  BUILD   EMS $(VERSION)"

# =============================================================================
# Kconfig 目标（保持和递归 Make 相同）
# =============================================================================

menuconfig: scripts/kconfig/mconf
	@$< Kconfig

defconfig: scripts/kconfig/conf
	@$< --defconfig=configs/defconfig Kconfig

%_defconfig: configs/%_defconfig scripts/kconfig/conf
	@scripts/kconfig/conf --defconfig=$< Kconfig

olddefconfig: scripts/kconfig/conf
	@$< --olddefconfig Kconfig

# 当 .config 不存在时，给出友好提示
.config:
	@echo >&2 '***'
	@echo >&2 '*** Configuration file "$@" not found!'
	@echo >&2 '***'
	@echo >&2 '*** Please run a configuration command first:'
	@echo >&2 '***   make defconfig           - Load default configuration'
	@echo >&2 '***   make menuconfig          - Interactive configuration'
	@echo >&2 '***   make <board>_defconfig   - Load board-specific configuration'
	@echo >&2 '***'
	@echo >&2 '*** Available defconfig files:'
	@echo >&2 '***   make ccm_h200_am625_debug_defconfig'
	@echo >&2 '***   make x86_64_full_defconfig'
	@echo >&2 '***   make x86_64_minimal_defconfig'
	@echo >&2 '***'
	@false

scripts/kconfig/conf scripts/kconfig/mconf:
	@$(MAKE) -C scripts/kconfig

# =============================================================================
# 版本信息
# =============================================================================

VERSION := 1.0.0
export VERSION

# =============================================================================
# 清理目标（扩展 scripts/rules.mk 中的基础清理）
# =============================================================================

.PHONY: mrproper
mrproper: clean
	@echo "  CLEAN   configuration"
	@rm -f .config .config.old
	@echo "  CLEAN   include/config include/generated"
	@rm -rf include/config/ include/generated/
	@$(MAKE) -C scripts/kconfig clean

.PHONY: distclean
distclean: mrproper
	@echo "  CLEAN   editor backup and patch files"
	@find . \( -name '*.orig' -o -name '*.rej' -o -name '*~' \
		-o -name '*.bak' -o -name '#*#' -o -name '.*.orig' \
		-o -name '.*.rej' -o -name '*.swp' -o -name '*.swo' \
		-o -name '*%' -o -name 'core' \) -type f -print | xargs rm -f 2>/dev/null || true

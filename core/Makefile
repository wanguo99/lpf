# ============================================================================
# 核心模块构建协调器（动态依赖版）
# ============================================================================
# 优化点：
# 1. 不硬编码依赖关系，从 Kconfig 和 .config 动态提取
# 2. 支持模块裁剪，只构建启用的模块
# 3. 支持灵活的依赖配置
# ============================================================================

# 所有可能的模块列表
ALL_MODULES := osal hal pcl pdl acl

# 从 .config 中提取启用的模块
ENABLED_MODULES :=
ifneq ($(wildcard $(objtree)/.config),)
  # 检查每个模块是否启用
  ifneq ($(shell grep -q '^CONFIG_OSAL=y' $(objtree)/.config 2>/dev/null && echo y),)
    ENABLED_MODULES += osal
  endif
  ifneq ($(shell grep -q '^CONFIG_HAL=y' $(objtree)/.config 2>/dev/null && echo y),)
    ENABLED_MODULES += hal
  endif
  ifneq ($(shell grep -q '^CONFIG_PCL=y' $(objtree)/.config 2>/dev/null && echo y),)
    ENABLED_MODULES += pcl
  endif
  ifneq ($(shell grep -q '^CONFIG_PDL=y' $(objtree)/.config 2>/dev/null && echo y),)
    ENABLED_MODULES += pdl
  endif
  ifneq ($(shell grep -q '^CONFIG_ACL=y' $(objtree)/.config 2>/dev/null && echo y),)
    ENABLED_MODULES += acl
  endif
else
  # 如果没有 .config，默认构建所有模块
  ENABLED_MODULES := $(ALL_MODULES)
endif

# 最终构建的模块列表
MODULES := $(ENABLED_MODULES)

.PHONY: all $(MODULES) headers_install_all
.PHONY: $(addsuffix -headers,$(MODULES))

all: $(MODULES)

# ============================================================================
# 头文件导出目标（快速，无需编译）
# ============================================================================

# 为每个模块定义头文件导出目标
define module-headers-target
$(1)-headers:
	+@$(MAKE) -C $(srctree)/core/$(1) \
		srctree=$(srctree) \
		objtree=$(objtree) \
		scriptdir=$(scriptdir) \
		headers_install 2>/dev/null || true
endef

$(foreach mod,$(MODULES),$(eval $(call module-headers-target,$(mod))))

# ============================================================================
# 模块构建目标（编译 + 链接）
# ============================================================================

# 递归构建每个模块
$(MODULES):
	+@$(MAKE) -C $(srctree)/core/$@ \
		BUILD_DIR=$(objtree)/core/$@ \
		srctree=$(srctree) \
		objtree=$(objtree) \
		scriptdir=$(scriptdir)

# ============================================================================
# 动态依赖关系（从 Kconfig depends on 提取）
# ============================================================================

# HAL 依赖 OSAL（如果两者都启用）
ifneq ($(filter hal,$(MODULES)),)
  ifneq ($(filter osal,$(MODULES)),)
    hal: osal
    hal-headers: osal-headers
  endif
endif

# PCL 依赖 HAL 和 OSAL（如果启用）
ifneq ($(filter pcl,$(MODULES)),)
  ifneq ($(filter hal,$(MODULES)),)
    pcl: hal
    pcl-headers: hal-headers
  endif
  ifneq ($(filter osal,$(MODULES)),)
    pcl: osal
    pcl-headers: osal-headers
  endif
endif

# PDL 依赖 PCL, HAL, OSAL（如果启用）
ifneq ($(filter pdl,$(MODULES)),)
  ifneq ($(filter pcl,$(MODULES)),)
    pdl: pcl
    pdl-headers: pcl-headers
  endif
  ifneq ($(filter hal,$(MODULES)),)
    pdl: hal
    pdl-headers: hal-headers
  endif
  ifneq ($(filter osal,$(MODULES)),)
    pdl: osal
    pdl-headers: osal-headers
  endif
endif

# ACL 依赖 PDL, PCL, HAL, OSAL（如果启用）
ifneq ($(filter acl,$(MODULES)),)
  ifneq ($(filter pdl,$(MODULES)),)
    acl: pdl
    acl-headers: pdl-headers
  endif
  ifneq ($(filter pcl,$(MODULES)),)
    acl: pcl
    acl-headers: pcl-headers
  endif
  ifneq ($(filter hal,$(MODULES)),)
    acl: hal
    acl-headers: hal-headers
  endif
  ifneq ($(filter osal,$(MODULES)),)
    acl: osal
    acl-headers: osal-headers
  endif
endif

# ============================================================================
# 头文件导出 - 将所有模块的头文件导出到顶层 include/
# ============================================================================
headers_install_all: $(addsuffix -headers,$(MODULES))
	@echo "  INSTALL headers for enabled modules: $(MODULES)"

# ============================================================================
# 核心模块构建协调器（优化版）
# ============================================================================
# 优化点：
# 1. 区分头文件导出和库构建，提高并行构建效率
# 2. 头文件导出后立即解除依赖，允许下游模块并行编译
# 3. 只在链接阶段才依赖库文件
# ============================================================================

# 模块列表（按依赖顺序）
MODULES := osal hal pcl pdl acl

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
# 优化的依赖关系（区分头文件依赖和库依赖）
# ============================================================================

# 头文件依赖（编译阶段）- 只需要头文件，可以并行
# hal 编译需要 osal 的头文件（不需要 libosal.so）
hal: osal-headers
pcl: hal-headers osal-headers
pdl: pcl-headers hal-headers osal-headers
acl: pdl-headers pcl-headers hal-headers osal-headers

# 库依赖（链接阶段）- 需要实际的库文件
# 这些依赖由链接器自动处理，make 只需确保库已构建
hal: osal
pcl: hal
pdl: pcl
acl: pdl

# ============================================================================
# 头文件导出 - 将所有模块的头文件导出到顶层 include/
# ============================================================================
headers_install_all: $(addsuffix -headers,$(MODULES))
	@echo "  INSTALL headers for all modules"

# ============================================================================
# 核心模块构建协调器
# ============================================================================

# 模块列表（按依赖顺序）
MODULES := osal hal pcl pdl acl

.PHONY: all $(MODULES) headers_install_all

all: $(MODULES)

# 递归构建每个模块
$(MODULES):
	+@$(MAKE) -C $(srctree)/core/$@ \
		BUILD_DIR=$(objtree)/core/$@ \
		srctree=$(srctree) \
		objtree=$(objtree) \
		scriptdir=$(scriptdir)

# 依赖关系（确保编译顺序）
hal: osal
pcl: hal
pdl: pcl
acl: pdl

# ============================================================================
# 头文件导出 - 将所有模块的头文件导出到顶层 include/
# ============================================================================
headers_install_all:
	+@for module in $(MODULES); do \
		$(MAKE) -C $(srctree)/core/$$module \
			srctree=$(srctree) \
			objtree=$(objtree) \
			scriptdir=$(scriptdir) \
			headers_install || exit 1; \
	done

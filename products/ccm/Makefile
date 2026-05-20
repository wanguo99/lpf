# ============================================================================
# CCM 产品层构建
# ============================================================================

# 产品配置
PRODUCT_CONFIG := h200_am625

# 应用列表
APPS := ccm_supervisor ccm_logger ccm_comm ccm_collector ccm_health

# 默认目标
.PHONY: all clean libs product apps

all: libs product apps

# 构建产品库
libs:
	+@$(MAKE) -C $(srctree)/products/ccm/libs \
		srctree=$(srctree) \
		objtree=$(objtree) \
		GENERATED_DIR=$(GENERATED_DIR)

# 构建产品配置
product: libs
	+@$(MAKE) -C $(srctree)/products/ccm/$(PRODUCT_CONFIG) \
		srctree=$(srctree) \
		objtree=$(objtree) \
		GENERATED_DIR=$(GENERATED_DIR)

# 构建所有应用
apps: product
	+@for app in $(APPS); do \
		$(MAKE) -C $(srctree)/products/ccm/apps/$$app \
			srctree=$(srctree) \
			objtree=$(objtree) \
			GENERATED_DIR=$(GENERATED_DIR) || exit 1; \
	done

# 清理
clean:
	+@$(MAKE) -C $(srctree)/products/ccm/libs clean \
		srctree=$(srctree) \
		objtree=$(objtree)
	+@$(MAKE) -C $(srctree)/products/ccm/$(PRODUCT_CONFIG) clean \
		srctree=$(srctree) \
		objtree=$(objtree)
	+@for app in $(APPS); do \
		$(MAKE) -C $(srctree)/products/ccm/apps/$$app clean \
			srctree=$(srctree) \
			objtree=$(objtree); \
	done

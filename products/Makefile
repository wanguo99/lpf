# ============================================================================
# 产品层构建
# ============================================================================

# 产品列表
PRODUCTS := ccm

# 默认目标
.PHONY: all clean

all:
	+@for product in $(PRODUCTS); do \
		$(MAKE) -C $(srctree)/products/$$product \
			srctree=$(srctree) \
			objtree=$(objtree) \
			GENERATED_DIR=$(GENERATED_DIR) || exit 1; \
	done

# 清理
clean:
	+@for product in $(PRODUCTS); do \
		$(MAKE) -C $(srctree)/products/$$product clean \
			srctree=$(srctree) \
			objtree=$(objtree); \
	done

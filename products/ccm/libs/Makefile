# ============================================================================
# CCM 库层构建
# ============================================================================

# 库列表
LIBS := libccm

# 默认目标
.PHONY: all clean

all:
	@echo "Building CCM libraries"
	@for lib in $(LIBS); do \
		echo "Building $$lib..."; \
		$(MAKE) -C $(srctree)/products/ccm/libs/$$lib \
			srctree=$(srctree) \
			objtree=$(objtree) \
			GENERATED_DIR=$(GENERATED_DIR) || exit 1; \
	done
	@echo "CCM libraries built successfully"

# 清理
clean:
	@echo "Cleaning CCM libraries"
	@for lib in $(LIBS); do \
		$(MAKE) -C $(srctree)/products/ccm/libs/$$lib clean \
			srctree=$(srctree) \
			objtree=$(objtree); \
	done

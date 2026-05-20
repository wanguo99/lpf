# ============================================================================
# CCM 库层构建 - Kbuild 框架
# ============================================================================

# 子目录列表
subdir-y += libccm/

# 默认目标
.PHONY: all clean

all:
	@echo "Building CCM libraries"
	@$(MAKE) -C libccm \
		srctree=$(srctree) \
		objtree=$(objtree) \
		scriptdir=$(scriptdir) || exit 1
	@echo "CCM libraries built successfully"

# 清理
clean:
	@echo "Cleaning CCM libraries"
	@$(MAKE) -C libccm clean \
		srctree=$(srctree) \
		objtree=$(objtree) \
		scriptdir=$(scriptdir)


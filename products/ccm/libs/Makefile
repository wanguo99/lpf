# ============================================================================
# CCM 库层构建 - Kbuild 框架
# ============================================================================

# 子目录列表
subdir-y += libccm/

# 默认目标
.PHONY: all clean

all:
	+@$(MAKE) -C libccm \
		srctree=$(srctree) \
		objtree=$(objtree) \
		scriptdir=$(scriptdir) || exit 1

# 清理
clean:
	+@$(MAKE) -C libccm clean \
		srctree=$(srctree) \
		objtree=$(objtree) \
		scriptdir=$(scriptdir)

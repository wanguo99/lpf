# =============================================================================
# OSAL Bare Metal 平台源文件配置
# =============================================================================
# 本文件由 core/osal/module.mk 自动包含
# 只需维护本平台的源文件列表

# 基础库文件（最小集）
osal_SRCS += \
	core/osal/src/bare/lib/osal_errno.c \
	core/osal/src/bare/lib/osal_heap.c \
	core/osal/src/bare/lib/osal_string.c

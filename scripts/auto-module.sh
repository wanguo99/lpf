#!/bin/bash
# =============================================================================
# 自动生成 module.mk
# =============================================================================

set -e

if [ $# -ne 1 ]; then
    echo "用法: $0 <模块目录>"
    echo "示例: $0 core/osal"
    exit 1
fi

MODULE_DIR="$1"
MODULE_NAME=$(basename "$MODULE_DIR")
MODULE_MK="${MODULE_DIR}/module.mk"

if [ ! -d "$MODULE_DIR" ]; then
    echo "错误：目录不存在: $MODULE_DIR"
    exit 1
fi

if [ ! -f "${MODULE_DIR}/Makefile" ]; then
    echo "错误：Makefile 不存在: ${MODULE_DIR}/Makefile"
    exit 1
fi

echo "正在生成 ${MODULE_MK}..."

# 提取源文件列表
SRCS=$(grep -E "^obj-y.*\.o" "${MODULE_DIR}/Makefile" | \
       sed 's/obj-y += //' | \
       sed 's/\.o/.c/' | \
       tr '\n' ' ')

# 提取编译标志
CFLAGS=$(grep -E "^ccflags-y" "${MODULE_DIR}/Makefile" | \
         sed 's/ccflags-y += //' | \
         tr '\n' ' ')

# 提取链接标志
LDFLAGS=$(grep -E "^ldflags-y" "${MODULE_DIR}/Makefile" | \
          sed 's/ldflags-y += //' | \
          tr '\n' ' ')

# 生成 module.mk
cat > "$MODULE_MK" << EOF
# =============================================================================
# ${MODULE_NAME} 模块构建配置
# =============================================================================
# 自动生成于: $(date)
# 生成工具: scripts/auto-module.sh

# 模块名称
${MODULE_NAME}_MODULE := ${MODULE_DIR}

# 源文件列表
${MODULE_NAME}_SRCS := \\
${SRCS}

# 目标文件列表
${MODULE_NAME}_OBJS := \$(call srcs_to_objs,\$(${MODULE_NAME}_SRCS))

# 编译标志
${MODULE_NAME}_CFLAGS := ${CFLAGS}

# 链接标志
${MODULE_NAME}_LDFLAGS := ${LDFLAGS}

# 目标库
${MODULE_NAME}_TARGET := \$(STAGING_DIR)/lib/lib${MODULE_NAME}.so
${MODULE_NAME}_STATIC := \$(STAGING_DIR)/lib/lib${MODULE_NAME}.a

# 添加到全局目标列表
ALL_TARGETS += \$(${MODULE_NAME}_TARGET)

# 定义构建规则
\$(eval \$(call build_shared_lib,\$(${MODULE_NAME}_TARGET),\$(${MODULE_NAME}_OBJS),\$(${MODULE_NAME}_LDFLAGS)))
\$(eval \$(call build_static_lib,\$(${MODULE_NAME}_STATIC),\$(${MODULE_NAME}_OBJS)))

# 编译标志（针对此模块的 .o 文件）
\$(${MODULE_NAME}_OBJS): CFLAGS += \$(${MODULE_NAME}_CFLAGS)
EOF

echo "✓ 生成完成: ${MODULE_MK}"
echo ""
echo "请检查并手动调整："
echo "  1. 源文件列表是否完整"
echo "  2. 编译标志是否正确"
echo "  3. 依赖关系是否正确"

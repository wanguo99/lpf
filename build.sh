#!/bin/bash

# ============================================================================
# EMS 构建脚本 - CMake Presets 包装器
# ============================================================================

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

# 默认配置
PRESET="release"
CLEAN=0
JOBS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

# 帮助信息
show_help() {
    cat << EOF
用法: $0 [选项]

这是一个简化的包装脚本，内部调用 CMake Presets。
推荐直接使用 CMake 命令以获得更好的体验。

选项:
    -h, --help          显示此帮助信息
    -d, --debug         Debug 模式编译（默认 Release）
    -c, --clean         清理构建目录
    -a, --arch ARCH     交叉编译架构 (arm32/arm64/riscv64)
    -j, --jobs N        并行编译任务数（默认：CPU核心数）

示例:
    $0                  # Release 模式编译
    $0 -d               # Debug 模式编译
    $0 -c               # 清理构建
    $0 -a arm32         # ARM32 交叉编译

推荐使用 CMake 命令:
    cmake --preset release && cmake --build --preset release
    cmake --preset debug && cmake --build --preset debug
    cmake --preset arm32 && cmake --build build/arm32

详细文档: docs/QUICK_BUILD.md

EOF
}

# 解析命令行参数
while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_help
            exit 0
            ;;
        -d|--debug)
            PRESET="debug"
            shift
            ;;
        -c|--clean)
            CLEAN=1
            shift
            ;;
        -a|--arch)
            PRESET="$2"
            shift 2
            ;;
        -j|--jobs)
            JOBS="$2"
            shift 2
            ;;
        *)
            echo -e "${RED}错误: 未知选项 $1${NC}"
            show_help
            exit 1
            ;;
    esac
done

# 清理构建
if [ $CLEAN -eq 1 ]; then
    echo -e "${YELLOW}清理构建目录...${NC}"
    rm -rf build
    echo -e "${GREEN}清理完成${NC}"
    exit 0
fi

# 验证 preset 是否存在
VALID_PRESETS="debug release arm32 arm64 riscv64"
if ! echo "$VALID_PRESETS" | grep -wq "$PRESET"; then
    echo -e "${RED}错误: 无效的配置 '$PRESET'${NC}"
    echo "可用配置: $VALID_PRESETS"
    exit 1
fi

# 配置
echo -e "${GREEN}配置构建 (preset: $PRESET)...${NC}"
cmake --preset "$PRESET"

# 编译
echo -e "${GREEN}开始编译 (并行任务: $JOBS)...${NC}"
cmake --build "build/$PRESET" -j "$JOBS"

# 显示结果
BUILD_DIR="build/$PRESET"
echo ""
echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}编译完成！${NC}"
echo -e "${GREEN}========================================${NC}"
echo -e "配置: ${YELLOW}$PRESET${NC}"
echo -e "输出目录: ${YELLOW}$BUILD_DIR${NC}"
echo ""

# 显示可执行文件
if [ -d "$BUILD_DIR/bin" ]; then
    echo -e "可执行文件:"
    ls -lh "$BUILD_DIR/bin" | tail -n +2 | awk '{printf "  \033[0;32m%s\033[0m (%s)\n", $9, $5}'
fi

# 显示库文件
if [ -d "$BUILD_DIR/lib" ]; then
    echo ""
    echo -e "库文件:"
    ls -lh "$BUILD_DIR/lib" | tail -n +2 | awk '{printf "  \033[0;32m%s\033[0m (%s)\n", $9, $5}'
fi

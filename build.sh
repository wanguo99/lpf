#!/bin/bash

# ============================================================================
# EMS 构建脚本 - 精简版
# 所有参数写死，只支持编译和清理
# ============================================================================

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

# ============================================================================
# 构建参数配置（写死，不支持命令行修改）
# ============================================================================
BUILD_TYPE="Release"                    # 构建类型：Release/Debug
BUILD_DIR="build"                       # 构建目录
INSTALL_PREFIX="/usr/local"             # 安装路径
BUILD_TESTING="ON"                      # 是否编译测试：ON/OFF
BUILD_SHARED_LIBS="ON"                  # 是否编译动态库：ON/OFF
INSTALL_HEADERS="ON"                    # 是否安装头文件：ON/OFF
BUILD_SAMPLE_APP="ON"                   # 是否编译 sample_app：ON/OFF
BUILD_WATCHDOG_APP="ON"                 # 是否编译 watchdog_app：ON/OFF

# 交叉编译工具链（留空表示本地编译）
# 示例：TOOLCHAIN_FILE="cmake/toolchains/arm32-linux-gnueabihf.cmake"
TOOLCHAIN_FILE=""

# 并行编译任务数
JOBS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

# ============================================================================
# 帮助信息
# ============================================================================
show_help() {
    cat << EOF
用法: $0 [clean]

命令:
    (无参数)    编译项目
    clean       清理构建目录

当前配置（在脚本中修改）:
    BUILD_TYPE         = $BUILD_TYPE
    BUILD_DIR          = $BUILD_DIR
    INSTALL_PREFIX     = $INSTALL_PREFIX
    BUILD_TESTING      = $BUILD_TESTING
    BUILD_SHARED_LIBS  = $BUILD_SHARED_LIBS
    INSTALL_HEADERS    = $INSTALL_HEADERS
    BUILD_SAMPLE_APP   = $BUILD_SAMPLE_APP
    BUILD_WATCHDOG_APP = $BUILD_WATCHDOG_APP
    TOOLCHAIN_FILE     = ${TOOLCHAIN_FILE:-"(native)"}
    JOBS               = $JOBS

等效的 CMake 命令:
    cmake -B $BUILD_DIR \\
        -DCMAKE_BUILD_TYPE=$BUILD_TYPE \\
        -DCMAKE_INSTALL_PREFIX=$INSTALL_PREFIX \\
        -DBUILD_TESTING=$BUILD_TESTING \\
        -DBUILD_SHARED_LIBS=$BUILD_SHARED_LIBS \\
        -DINSTALL_HEADERS=$INSTALL_HEADERS \\
        -DBUILD_SAMPLE_APP=$BUILD_SAMPLE_APP \\
        -DBUILD_WATCHDOG_APP=$BUILD_WATCHDOG_APP
    cmake --build $BUILD_DIR -j$JOBS

EOF
}

# ============================================================================
# 清理构建
# ============================================================================
if [ "$1" = "clean" ]; then
    echo -e "${YELLOW}清理构建目录: $BUILD_DIR${NC}"
    rm -rf "$BUILD_DIR"
    echo -e "${GREEN}清理完成${NC}"
    exit 0
fi

# ============================================================================
# 显示帮助
# ============================================================================
if [ "$1" = "-h" ] || [ "$1" = "--help" ]; then
    show_help
    exit 0
fi

# ============================================================================
# 检查参数
# ============================================================================
if [ $# -gt 0 ]; then
    echo -e "${RED}错误: 未知参数 '$1'${NC}"
    echo "使用 '$0 --help' 查看帮助"
    exit 1
fi

# ============================================================================
# 配置构建
# ============================================================================
echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}EMS 构建配置${NC}"
echo -e "${GREEN}========================================${NC}"
echo -e "构建类型:       ${YELLOW}$BUILD_TYPE${NC}"
echo -e "构建目录:       ${YELLOW}$BUILD_DIR${NC}"
echo -e "安装路径:       ${YELLOW}$INSTALL_PREFIX${NC}"
echo -e "编译测试:       ${YELLOW}$BUILD_TESTING${NC}"
echo -e "动态库:         ${YELLOW}$BUILD_SHARED_LIBS${NC}"
echo -e "安装头文件:     ${YELLOW}$INSTALL_HEADERS${NC}"
echo -e "Sample App:     ${YELLOW}$BUILD_SAMPLE_APP${NC}"
echo -e "Watchdog App:   ${YELLOW}$BUILD_WATCHDOG_APP${NC}"
if [ -n "$TOOLCHAIN_FILE" ]; then
    echo -e "工具链:         ${YELLOW}$TOOLCHAIN_FILE${NC}"
else
    echo -e "工具链:         ${YELLOW}(native)${NC}"
fi
echo -e "并行任务:       ${YELLOW}$JOBS${NC}"
echo -e "${GREEN}========================================${NC}"
echo ""

# 构建 CMake 配置命令
CMAKE_ARGS=(
    -B "$BUILD_DIR"
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
    -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX"
    -DBUILD_TESTING="$BUILD_TESTING"
    -DBUILD_SHARED_LIBS="$BUILD_SHARED_LIBS"
    -DINSTALL_HEADERS="$INSTALL_HEADERS"
    -DBUILD_SAMPLE_APP="$BUILD_SAMPLE_APP"
    -DBUILD_WATCHDOG_APP="$BUILD_WATCHDOG_APP"
)

if [ -n "$TOOLCHAIN_FILE" ]; then
    CMAKE_ARGS+=(-DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE")
fi

# 配置
echo -e "${GREEN}配置构建...${NC}"
cmake "${CMAKE_ARGS[@]}"
echo ""

# 编译
echo -e "${GREEN}开始编译...${NC}"
cmake --build "$BUILD_DIR" -j "$JOBS"
echo ""

# 显示结果
echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}编译完成！${NC}"
echo -e "${GREEN}========================================${NC}"
echo -e "输出目录: ${YELLOW}$BUILD_DIR${NC}"
echo ""

# 显示可执行文件
if [ -d "$BUILD_DIR/bin" ]; then
    echo -e "可执行文件:"
    ls -lh "$BUILD_DIR/bin" 2>/dev/null | tail -n +2 | awk '{printf "  \033[0;32m%s\033[0m (%s)\n", $9, $5}' || true
fi

# 显示库文件
if [ -d "$BUILD_DIR/lib" ]; then
    echo ""
    echo -e "库文件:"
    ls -lh "$BUILD_DIR/lib" 2>/dev/null | tail -n +2 | awk '{printf "  \033[0;32m%s\033[0m (%s)\n", $9, $5}' || true
fi

echo ""
echo -e "${GREEN}安装命令:${NC}"
echo -e "  sudo cmake --install $BUILD_DIR"
echo ""

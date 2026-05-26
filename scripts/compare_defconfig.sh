#!/bin/bash
# EMS Defconfig 配置对比工具
# 用于快速查看和对比不同配置文件的关键差异

set -e

CONFIGS_DIR="configs"
SCRIPT_NAME=$(basename "$0")

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

usage() {
    cat <<EOF
用法: $SCRIPT_NAME [选项] [配置文件...]

选项:
    -l, --list              列出所有可用的配置文件
    -s, --summary           显示所有配置的摘要信息
    -c, --compare FILE1 FILE2   对比两个配置文件的差异
    -d, --detail FILE       显示配置文件的详细信息
    -h, --help              显示此帮助信息

示例:
    $SCRIPT_NAME -l
    $SCRIPT_NAME -s
    $SCRIPT_NAME -c ccm_h200_am625_debug_defconfig ccm_h200_am625_release_defconfig
    $SCRIPT_NAME -d x86_64_full_defconfig

EOF
}

list_configs() {
    echo -e "${BLUE}可用的配置文件:${NC}"
    echo
    for config in "$CONFIGS_DIR"/*_defconfig; do
        if [ -f "$config" ]; then
            basename "$config"
        fi
    done | sort
}

extract_key_info() {
    local config_file="$1"
    local arch=$(grep "^CONFIG_ARCH_" "$config_file" | head -1 | sed 's/CONFIG_ARCH_//;s/=y//')
    local os=$(grep "^CONFIG_OS_" "$config_file" | head -1 | sed 's/CONFIG_OS_//;s/=y//')
    local opt=$(grep "^CONFIG_OPT_" "$config_file" | head -1 | sed 's/CONFIG_OPT_//;s/=y//')
    local debug=$(grep -q "^CONFIG_BUILD_TYPE_DEBUG=y" "$config_file" && echo "YES" || echo "NO")
    local static=$(grep -q "^CONFIG_LIBRARY_BUILD_TYPE_STATIC=y" "$config_file" && echo "S" || echo "-")
    local shared=$(grep -q "^CONFIG_LIBRARY_BUILD_TYPE_DYNAMIC=y" "$config_file" && echo "D" || echo "-")
    local testing=$(grep -q "^CONFIG_BUILD_TESTING=y" "$config_file" && echo "YES" || echo "NO")

    echo "$arch|$os|$opt|$debug|${static}${shared}|$testing"
}

show_summary() {
    echo -e "${BLUE}配置文件摘要:${NC}"
    echo
    printf "%-35s %-10s %-8s %-6s %-6s %-6s %-6s\n" "配置文件" "架构" "OS" "优化" "调试" "库" "测试"
    printf "%-35s %-10s %-8s %-6s %-6s %-6s %-6s\n" "$(printf '%.0s-' {1..35})" "$(printf '%.0s-' {1..10})" "$(printf '%.0s-' {1..8})" "$(printf '%.0s-' {1..6})" "$(printf '%.0s-' {1..6})" "$(printf '%.0s-' {1..6})" "$(printf '%.0s-' {1..6})"

    for config in "$CONFIGS_DIR"/*_defconfig; do
        if [ -f "$config" ]; then
            local name=$(basename "$config")
            local info=$(extract_key_info "$config")
            local arch=$(echo "$info" | cut -d'|' -f1)
            local os=$(echo "$info" | cut -d'|' -f2)
            local opt=$(echo "$info" | cut -d'|' -f3)
            local debug=$(echo "$info" | cut -d'|' -f4)
            local lib=$(echo "$info" | cut -d'|' -f5)
            local testing=$(echo "$info" | cut -d'|' -f6)

            printf "%-35s %-10s %-8s %-6s %-6s %-6s %-6s\n" \
                "$name" "$arch" "$os" "$opt" "$debug" "$lib" "$testing"
        fi
    done | sort
}

show_detail() {
    local config_file="$CONFIGS_DIR/$1"

    if [ ! -f "$config_file" ]; then
        echo -e "${RED}错误: 配置文件不存在: $config_file${NC}"
        exit 1
    fi

    echo -e "${BLUE}配置文件详细信息: $(basename $config_file)${NC}"
    echo

    # 基本信息
    echo -e "${GREEN}[基本配置]${NC}"
    grep "^CONFIG_ARCH_\|^CONFIG_OS_\|^CONFIG_OPT_\|^CONFIG_CROSS_COMPILE\|^CONFIG_BUILD_TYPE" "$config_file" | sed 's/^/  /'
    echo

    # OSAL 配置
    echo -e "${GREEN}[OSAL 配置]${NC}"
    grep "^CONFIG_OSAL" "$config_file" | sed 's/^/  /'
    echo

    # HAL 配置
    echo -e "${GREEN}[HAL 配置]${NC}"
    grep "^CONFIG_HAL\|^CONFIG_PLATFORM" "$config_file" | sed 's/^/  /'
    echo

    # 应用配置
    echo -e "${GREEN}[应用配置]${NC}"
    grep "^CONFIG_BUILD_CCM" "$config_file" | sed 's/^/  /'
    echo

    # 测试配置
    if grep -q "^CONFIG_BUILD_TESTING=y" "$config_file"; then
        echo -e "${GREEN}[测试配置]${NC}"
        grep "^CONFIG_BUILD_TESTING" "$config_file" | sed 's/^/  /'
        echo
    fi
}

compare_configs() {
    local file1="$CONFIGS_DIR/$1"
    local file2="$CONFIGS_DIR/$2"

    if [ ! -f "$file1" ]; then
        echo -e "${RED}错误: 配置文件不存在: $file1${NC}"
        exit 1
    fi

    if [ ! -f "$file2" ]; then
        echo -e "${RED}错误: 配置文件不存在: $file2${NC}"
        exit 1
    fi

    echo -e "${BLUE}配置对比:${NC}"
    echo -e "  文件1: ${YELLOW}$(basename $file1)${NC}"
    echo -e "  文件2: ${YELLOW}$(basename $file2)${NC}"
    echo

    echo -e "${GREEN}[仅在文件1中存在的配置]${NC}"
    diff -u "$file2" "$file1" | grep "^+CONFIG" | sed 's/^+/  /' || echo "  (无)"
    echo

    echo -e "${GREEN}[仅在文件2中存在的配置]${NC}"
    diff -u "$file2" "$file1" | grep "^-CONFIG" | sed 's/^-/  /' || echo "  (无)"
    echo

    echo -e "${GREEN}[配置值不同]${NC}"
    comm -3 <(grep "^CONFIG" "$file1" | sort) <(grep "^CONFIG" "$file2" | sort) | sed 's/^/  /'
}

# 主程序
if [ $# -eq 0 ]; then
    usage
    exit 0
fi

case "$1" in
    -l|--list)
        list_configs
        ;;
    -s|--summary)
        show_summary
        ;;
    -d|--detail)
        if [ -z "$2" ]; then
            echo -e "${RED}错误: 请指定配置文件${NC}"
            usage
            exit 1
        fi
        show_detail "$2"
        ;;
    -c|--compare)
        if [ -z "$2" ] || [ -z "$3" ]; then
            echo -e "${RED}错误: 请指定两个配置文件${NC}"
            usage
            exit 1
        fi
        compare_configs "$2" "$3"
        ;;
    -h|--help)
        usage
        ;;
    *)
        echo -e "${RED}错误: 未知选项: $1${NC}"
        usage
        exit 1
        ;;
esac

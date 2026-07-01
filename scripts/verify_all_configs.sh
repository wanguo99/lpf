#!/bin/bash
################################################################################
# PAF 配置验证脚本
#
# 功能：
# - 自动遍历所有 defconfig 配置
# - 逐个验证编译是否成功
# - 生成详细的测试报告
#
# 使用方法：
#   ./scripts/verify_all_configs.sh [选项]
#
# 选项：
#   -j N          并行编译线程数（默认：nproc）
#   -k            出错后继续测试其他配置
#   -v            详细输出模式
#   -o DIR        输出日志目录（默认：build_logs）
#   -h            显示帮助信息
################################################################################

set -e  # 遇到错误立即退出（除非使用 -k）

# 默认配置
PARALLEL_JOBS=$(nproc)
KEEP_GOING=0
VERBOSE=0
LOG_DIR="build_logs"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# 颜色定义
COLOR_RED='\033[0;31m'
COLOR_GREEN='\033[0;32m'
COLOR_YELLOW='\033[0;33m'
COLOR_BLUE='\033[0;34m'
COLOR_RESET='\033[0m'

# 统计变量
TOTAL_CONFIGS=0
SUCCESS_COUNT=0
FAILED_COUNT=0
SKIPPED_COUNT=0
START_TIME=$(date +%s)

# 失败的配置列表
declare -a FAILED_CONFIGS

################################################################################
# 辅助函数
################################################################################

print_help() {
    cat << EOF
用法: $0 [选项]

选项:
  -j N          并行编译线程数（默认：$(nproc)）
  -k            出错后继续测试其他配置
  -v            详细输出模式
  -o DIR        输出日志目录（默认：build_logs）
  -h            显示帮助信息

示例:
  $0                    # 使用默认设置验证所有配置
  $0 -j 8 -k            # 使用 8 线程，出错继续
  $0 -v -o logs         # 详细模式，日志输出到 logs 目录

EOF
}

log_info() {
    echo -e "${COLOR_BLUE}[INFO]${COLOR_RESET} $*"
}

log_success() {
    echo -e "${COLOR_GREEN}[PASS]${COLOR_RESET} $*"
}

log_error() {
    echo -e "${COLOR_RED}[FAIL]${COLOR_RESET} $*"
}

log_warning() {
    echo -e "${COLOR_YELLOW}[WARN]${COLOR_RESET} $*"
}

print_separator() {
    echo "========================================================================"
}

################################################################################
# 获取所有配置
################################################################################

get_all_configs() {
    log_info "获取所有可用配置..."

    cd "$PROJECT_ROOT"

    # 使用 make list 获取配置列表
    local configs=$(make list 2>/dev/null | grep -E "_defconfig$" | sed 's/^[[:space:]]*//' || true)

    if [ -z "$configs" ]; then
        log_error "无法获取配置列表，请确保在项目根目录下运行"
        exit 1
    fi

    echo "$configs"
}

################################################################################
# 验证单个配置
################################################################################

verify_config() {
    local config="$1"
    local log_file="$LOG_DIR/${config}.log"

    print_separator
    log_info "正在测试配置: $config"

    # 清理之前的构建
    if [ $VERBOSE -eq 1 ]; then
        log_info "  执行 make distclean..."
    fi

    if ! make distclean >> "$log_file" 2>&1; then
        log_warning "  清理失败（可能是首次构建），继续..."
    fi

    # 加载配置
    if [ $VERBOSE -eq 1 ]; then
        log_info "  加载配置 $config..."
    fi

    if ! make "$config" >> "$log_file" 2>&1; then
        log_error "  配置加载失败: $config"
        FAILED_CONFIGS+=("$config")
        ((FAILED_COUNT++))
        return 1
    fi

    # 编译
    if [ $VERBOSE -eq 1 ]; then
        log_info "  开始编译（-j$PARALLEL_JOBS）..."
    fi

    local build_start=$(date +%s)

    if make -j"$PARALLEL_JOBS" >> "$log_file" 2>&1; then
        local build_end=$(date +%s)
        local build_time=$((build_end - build_start))

        log_success "$config - 编译成功 (耗时: ${build_time}s)"
        ((SUCCESS_COUNT++))
        return 0
    else
        local build_end=$(date +%s)
        local build_time=$((build_end - build_start))

        log_error "$config - 编译失败 (耗时: ${build_time}s)"
        log_error "  详细日志: $log_file"

        # 显示最后 20 行错误日志
        if [ $VERBOSE -eq 1 ]; then
            echo "  --- 错误摘要 ---"
            tail -n 20 "$log_file" | sed 's/^/  /'
            echo "  ----------------"
        fi

        FAILED_CONFIGS+=("$config")
        ((FAILED_COUNT++))
        return 1
    fi
}

################################################################################
# 生成测试报告
################################################################################

generate_report() {
    local end_time=$(date +%s)
    local total_time=$((end_time - START_TIME))
    local report_file="$LOG_DIR/test_report.txt"

    print_separator
    log_info "生成测试报告..."

    cat > "$report_file" << EOF
========================================================================
PAF 配置验证报告
========================================================================

测试时间: $(date '+%Y-%m-%d %H:%M:%S')
总耗时: ${total_time}s ($(date -u -d @${total_time} +'%H:%M:%S'))

------------------------------------------------------------------------
测试统计
------------------------------------------------------------------------
总配置数:   $TOTAL_CONFIGS
成功数:     $SUCCESS_COUNT
失败数:     $FAILED_COUNT
跳过数:     $SKIPPED_COUNT

成功率:     $(awk "BEGIN {printf \"%.1f%%\", ($SUCCESS_COUNT/$TOTAL_CONFIGS)*100}")

------------------------------------------------------------------------
配置详情
------------------------------------------------------------------------

EOF

    # 列出所有配置的状态
    cd "$PROJECT_ROOT"
    local configs=$(get_all_configs)

    while IFS= read -r config; do
        if [[ " ${FAILED_CONFIGS[@]} " =~ " ${config} " ]]; then
            echo "[FAIL] $config" >> "$report_file"
        else
            echo "[PASS] $config" >> "$report_file"
        fi
    done <<< "$configs"

    # 如果有失败的配置，列出详细信息
    if [ ${#FAILED_CONFIGS[@]} -gt 0 ]; then
        cat >> "$report_file" << EOF

------------------------------------------------------------------------
失败配置详情
------------------------------------------------------------------------

EOF
        for config in "${FAILED_CONFIGS[@]}"; do
            echo "配置: $config" >> "$report_file"
            echo "日志: $LOG_DIR/${config}.log" >> "$report_file"
            echo "" >> "$report_file"
        done
    fi

    cat >> "$report_file" << EOF
------------------------------------------------------------------------
日志文件位置: $LOG_DIR
报告文件位置: $report_file
------------------------------------------------------------------------
EOF

    # 显示报告摘要
    print_separator
    cat "$report_file"
    print_separator

    log_info "完整报告已保存到: $report_file"
}

################################################################################
# 主函数
################################################################################

main() {
    # 解析命令行参数
    while getopts "j:kvo:h" opt; do
        case $opt in
            j)
                PARALLEL_JOBS="$OPTARG"
                ;;
            k)
                KEEP_GOING=1
                set +e  # 出错不退出
                ;;
            v)
                VERBOSE=1
                ;;
            o)
                LOG_DIR="$OPTARG"
                ;;
            h)
                print_help
                exit 0
                ;;
            *)
                print_help
                exit 1
                ;;
        esac
    done

    # 创建日志目录
    mkdir -p "$LOG_DIR"

    # 切换到项目根目录
    cd "$PROJECT_ROOT"

    print_separator
    log_info "PAF 配置验证脚本"
    print_separator
    log_info "项目目录: $PROJECT_ROOT"
    log_info "日志目录: $LOG_DIR"
    log_info "并行线程: $PARALLEL_JOBS"
    log_info "出错继续: $([ $KEEP_GOING -eq 1 ] && echo '是' || echo '否')"
    log_info "详细模式: $([ $VERBOSE -eq 1 ] && echo '是' || echo '否')"
    print_separator

    # 获取所有配置
    local configs=$(get_all_configs)
    TOTAL_CONFIGS=$(echo "$configs" | wc -l)

    log_info "找到 $TOTAL_CONFIGS 个配置"
    echo ""

    # 遍历所有配置
    local current=0
    while IFS= read -r config; do
        ((current++))
        log_info "进度: $current/$TOTAL_CONFIGS"

        if ! verify_config "$config"; then
            if [ $KEEP_GOING -eq 0 ]; then
                log_error "测试失败，退出"
                generate_report
                exit 1
            fi
        fi

        echo ""
    done <<< "$configs"

    # 生成报告
    generate_report

    # 返回状态
    if [ $FAILED_COUNT -eq 0 ]; then
        log_success "所有配置验证通过！"
        exit 0
    else
        log_error "$FAILED_COUNT 个配置验证失败"
        exit 1
    fi
}

# 运行主函数
main "$@"

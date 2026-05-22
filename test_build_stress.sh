#!/bin/bash
# =============================================================================
# EMS 构建系统压力测试脚本
# =============================================================================
# 测试所有构建操作的稳定性和正确性
# 运行次数: 200 次
# =============================================================================

# 不使用 set -e，手动处理错误

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 统计变量
TOTAL_TESTS=200
PASSED=0
FAILED=0
ERRORS_LOG="/tmp/ems_stress_test_errors.log"

# 清空错误日志
> "$ERRORS_LOG"

# 打印带颜色的消息
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[PASS]${NC} $1"
}

print_error() {
    echo -e "${RED}[FAIL]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

# 记录错误
log_error() {
    local iteration=$1
    local operation=$2
    local error_msg=$3
    echo "========================================" >> "$ERRORS_LOG"
    echo "Iteration: $iteration" >> "$ERRORS_LOG"
    echo "Operation: $operation" >> "$ERRORS_LOG"
    echo "Error: $error_msg" >> "$ERRORS_LOG"
    echo "Time: $(date)" >> "$ERRORS_LOG"
    echo "" >> "$ERRORS_LOG"
}

# 检查命令是否成功
check_command() {
    local iteration=$1
    local operation=$2
    local cmd=$3

    if eval "$cmd" > /dev/null 2>&1; then
        return 0
    else
        log_error "$iteration" "$operation" "Command failed: $cmd"
        return 1
    fi
}

# 检查文件是否存在
check_file_exists() {
    local file=$1
    if [ -f "$file" ]; then
        return 0
    else
        return 1
    fi
}

# 检查目录是否存在
check_dir_exists() {
    local dir=$1
    if [ -d "$dir" ]; then
        return 0
    else
        return 1
    fi
}

# 单次测试
run_single_test() {
    local iteration=$1
    local test_failed=0

    print_info "=========================================="
    print_info "Iteration $iteration/$TOTAL_TESTS"
    print_info "=========================================="

    # 1. 清理环境
    print_info "[$iteration] Step 1: Clean environment (make mrproper)"
    if ! check_command "$iteration" "mrproper" "make mrproper"; then
        print_error "[$iteration] mrproper failed"
        test_failed=1
        return 1
    fi

    # 验证清理结果
    if check_file_exists ".config" || check_dir_exists "staging"; then
        print_error "[$iteration] mrproper did not clean properly"
        log_error "$iteration" "mrproper" "Files still exist after mrproper"
        test_failed=1
        return 1
    fi
    print_success "[$iteration] mrproper OK"

    # 2. 配置（使用 defconfig 如果存在，否则创建最小配置）
    print_info "[$iteration] Step 2: Configure"

    # 查找第一个可用的 defconfig
    defconfig=$(find configs -name "*.defconfig" -type f 2>/dev/null | head -1)

    if [ -n "$defconfig" ]; then
        defconfig_name=$(basename "$defconfig")
        print_info "[$iteration] Using defconfig: $defconfig_name"
        if ! check_command "$iteration" "defconfig" "make $defconfig_name"; then
            print_error "[$iteration] defconfig failed"
            test_failed=1
            return 1
        fi
    else
        # 创建最小配置
        print_info "[$iteration] Creating minimal config"
        cat > .config << 'EOF'
CONFIG_CROSS_COMPILE=""
CONFIG_CC_OPTIMIZE_FOR_SIZE=y
EOF
        if ! check_command "$iteration" "olddefconfig" "make olddefconfig"; then
            print_error "[$iteration] olddefconfig failed"
            test_failed=1
            return 1
        fi
    fi

    # 验证配置文件
    if ! check_file_exists ".config"; then
        print_error "[$iteration] .config not created"
        log_error "$iteration" "config" ".config file missing"
        test_failed=1
        return 1
    fi
    print_success "[$iteration] Configure OK"

    # 3. 单线程编译
    print_info "[$iteration] Step 3: Single-threaded build"
    if ! check_command "$iteration" "build-single" "make"; then
        print_error "[$iteration] Single-threaded build failed"
        test_failed=1
        return 1
    fi

    # 验证 staging 目录
    if ! check_dir_exists "staging"; then
        print_error "[$iteration] staging directory not created"
        log_error "$iteration" "build" "staging directory missing"
        test_failed=1
        return 1
    fi
    print_success "[$iteration] Single-threaded build OK"

    # 4. 清理编译产物
    print_info "[$iteration] Step 4: Clean build artifacts (make clean)"
    if ! check_command "$iteration" "clean" "make clean"; then
        print_error "[$iteration] clean failed"
        test_failed=1
        return 1
    fi

    # 验证 staging 被清理
    if check_dir_exists "staging"; then
        print_error "[$iteration] staging directory not cleaned"
        log_error "$iteration" "clean" "staging directory still exists"
        test_failed=1
        return 1
    fi

    # 验证 .config 仍然存在
    if ! check_file_exists ".config"; then
        print_error "[$iteration] .config was removed by clean"
        log_error "$iteration" "clean" ".config should not be removed"
        test_failed=1
        return 1
    fi
    print_success "[$iteration] Clean OK"

    # 5. 多线程编译
    local nproc=$(nproc)
    print_info "[$iteration] Step 5: Multi-threaded build (make -j$nproc)"
    if ! check_command "$iteration" "build-multi" "make -j$nproc"; then
        print_error "[$iteration] Multi-threaded build failed"
        test_failed=1
        return 1
    fi

    # 验证 staging 目录
    if ! check_dir_exists "staging"; then
        print_error "[$iteration] staging directory not created"
        log_error "$iteration" "build-multi" "staging directory missing"
        test_failed=1
        return 1
    fi
    print_success "[$iteration] Multi-threaded build OK"

    # 6. 测试 make install
    local install_dir="/tmp/ems-stress-test-install-$iteration"
    rm -rf "$install_dir"

    print_info "[$iteration] Step 6: Install (make install)"
    if ! check_command "$iteration" "install" "make install DESTDIR=$install_dir prefix=/usr"; then
        print_error "[$iteration] install failed"
        test_failed=1
        return 1
    fi

    # 验证安装目录
    if ! check_dir_exists "$install_dir/usr"; then
        print_error "[$iteration] install directory not created"
        log_error "$iteration" "install" "install directory missing"
        test_failed=1
        return 1
    fi
    print_success "[$iteration] Install OK"

    # 清理安装目录
    rm -rf "$install_dir"

    # 7. 测试部分安装
    print_info "[$iteration] Step 7: Partial install (install_bin, install_lib, install_headers)"

    install_dir="/tmp/ems-stress-test-partial-$iteration"
    rm -rf "$install_dir"

    if ! check_command "$iteration" "install_bin" "make install_bin DESTDIR=$install_dir"; then
        print_error "[$iteration] install_bin failed"
        test_failed=1
        return 1
    fi

    if ! check_command "$iteration" "install_lib" "make install_lib DESTDIR=$install_dir"; then
        print_error "[$iteration] install_lib failed"
        test_failed=1
        return 1
    fi

    if ! check_command "$iteration" "install_headers" "make install_headers DESTDIR=$install_dir"; then
        print_error "[$iteration] install_headers failed"
        test_failed=1
        return 1
    fi

    print_success "[$iteration] Partial install OK"

    # 清理安装目录
    rm -rf "$install_dir"

    # 8. 测试 distclean
    print_info "[$iteration] Step 8: Deep clean (make distclean)"
    if ! check_command "$iteration" "distclean" "make distclean"; then
        print_error "[$iteration] distclean failed"
        test_failed=1
        return 1
    fi

    # 验证所有文件都被清理
    if check_file_exists ".config" || check_dir_exists "staging" || check_dir_exists "include/config"; then
        print_error "[$iteration] distclean did not clean properly"
        log_error "$iteration" "distclean" "Files still exist after distclean"
        test_failed=1
        return 1
    fi
    print_success "[$iteration] Distclean OK"

    # 测试成功
    if [ $test_failed -eq 0 ]; then
        print_success "[$iteration] All tests passed"
        return 0
    else
        return 1
    fi
}

# 主测试循环
main() {
    print_info "=========================================="
    print_info "EMS Build System Stress Test"
    print_info "Total iterations: $TOTAL_TESTS"
    print_info "=========================================="
    echo ""

    local start_time=$(date +%s)

    for i in $(seq 1 $TOTAL_TESTS); do
        if run_single_test "$i"; then
            ((PASSED++))
        else
            ((FAILED++))
            print_error "Test iteration $i failed"
        fi

        # 显示进度
        local progress=$((i * 100 / TOTAL_TESTS))
        echo ""
        print_info "Progress: $i/$TOTAL_TESTS ($progress%) - Passed: $PASSED, Failed: $FAILED"
        echo ""

        # 如果失败太多，提前退出
        if [ $FAILED -gt 10 ]; then
            print_error "Too many failures ($FAILED), stopping test"
            break
        fi
    done

    local end_time=$(date +%s)
    local duration=$((end_time - start_time))

    # 最终清理
    print_info "Final cleanup..."
    make mrproper > /dev/null 2>&1 || true

    # 打印测试结果
    echo ""
    print_info "=========================================="
    print_info "Test Summary"
    print_info "=========================================="
    print_info "Total tests: $TOTAL_TESTS"
    print_success "Passed: $PASSED"
    if [ $FAILED -gt 0 ]; then
        print_error "Failed: $FAILED"
    else
        print_info "Failed: $FAILED"
    fi
    print_info "Duration: ${duration}s"
    print_info "Error log: $ERRORS_LOG"
    echo ""

    if [ $FAILED -eq 0 ]; then
        print_success "=========================================="
        print_success "ALL TESTS PASSED!"
        print_success "=========================================="
        return 0
    else
        print_error "=========================================="
        print_error "SOME TESTS FAILED!"
        print_error "Check error log: $ERRORS_LOG"
        print_error "=========================================="
        return 1
    fi
}

# 运行主函数
main
exit $?

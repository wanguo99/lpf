#!/bin/bash
# =============================================================================
# EMS 构建系统压力测试脚本
# =============================================================================
# 功能：对所有 defconfig 进行多次配置、构建、清理循环测试
# 用法：./stress_test.sh [iterations] [configs...]
# =============================================================================

set -e  # 遇到错误立即退出

# 配置
ITERATIONS=${1:-300}
CONFIGS=${@:2}
if [ -z "$CONFIGS" ]; then
    CONFIGS="x86_64_full x86_64_minimal x86_64_test"
fi

LOG_DIR="stress_test_logs"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
SUMMARY_LOG="${LOG_DIR}/summary_${TIMESTAMP}.log"

# 创建日志目录
mkdir -p "${LOG_DIR}"

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 统计变量
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# 记录开始时间
START_TIME=$(date +%s)

echo "=========================================="
echo "EMS 构建系统压力测试"
echo "=========================================="
echo "迭代次数: ${ITERATIONS}"
echo "测试配置: ${CONFIGS}"
echo "日志目录: ${LOG_DIR}"
echo "开始时间: $(date)"
echo "=========================================="
echo ""

# 初始化摘要日志
cat > "${SUMMARY_LOG}" << EOF
EMS 构建系统压力测试报告
========================================
开始时间: $(date)
迭代次数: ${ITERATIONS}
测试配置: ${CONFIGS}
========================================

EOF

# 测试函数
test_config() {
    local config=$1
    local iteration=$2
    local build_dir="build-stress-${config}"
    local log_file="${LOG_DIR}/${config}_iter${iteration}_${TIMESTAMP}.log"

    echo -e "${BLUE}[${iteration}/${ITERATIONS}]${NC} 测试配置: ${config}"

    # 1. 清理
    echo -n "  清理... "
    if rm -rf "${build_dir}" >> "${log_file}" 2>&1; then
        echo -e "${GREEN}✓${NC}"
    else
        echo -e "${RED}✗${NC}"
        echo "ERROR: 清理失败" >> "${log_file}"
        return 1
    fi

    # 2. 配置
    echo -n "  配置... "
    if cmake -B "${build_dir}" -DDEFCONFIG="${config}" >> "${log_file}" 2>&1; then
        echo -e "${GREEN}✓${NC}"
    else
        echo -e "${RED}✗${NC}"
        echo "ERROR: 配置失败" >> "${log_file}"
        return 1
    fi

    # 3. 构建
    echo -n "  构建... "
    if cmake --build "${build_dir}" -j$(nproc) >> "${log_file}" 2>&1; then
        echo -e "${GREEN}✓${NC}"
    else
        echo -e "${RED}✗${NC}"
        echo "ERROR: 构建失败" >> "${log_file}"
        return 1
    fi

    # 4. 验证产物
    echo -n "  验证... "
    local missing_files=0
    local found_bins=0
    local found_libs=0

    # 检查二进制文件（至少需要有一些）
    for bin in ccm_collector ccm_logger ccm_health ccm_supervisor ccm_comm; do
        if [ -f "${build_dir}/bin/${bin}" ]; then
            found_bins=$((found_bins + 1))
        fi
    done

    # 检查关键库文件（必须存在）
    for lib in libosal.so libhal.so libpcl.so libacl.so libccm.so libh200_am625.so; do
        if [ ! -f "${build_dir}/lib/${lib}" ]; then
            echo "ERROR: 缺少库文件: ${lib}" >> "${log_file}"
            missing_files=$((missing_files + 1))
        else
            found_libs=$((found_libs + 1))
        fi
    done

    # 验证：至少有1个二进制文件，所有库文件都存在
    if [ ${found_bins} -eq 0 ]; then
        echo "ERROR: 没有找到任何二进制文件" >> "${log_file}"
        echo -e "${RED}✗ (没有二进制文件)${NC}"
        return 1
    elif [ ${missing_files} -gt 0 ]; then
        echo -e "${RED}✗ (缺少 ${missing_files} 个库文件)${NC}"
        return 1
    else
        echo -e "${GREEN}✓ (${found_bins} 个二进制, ${found_libs} 个库)${NC}"
    fi

    # 5. 生成校验和
    echo -n "  校验和... "
    if cmake --build "${build_dir}" --target checksums >> "${log_file}" 2>&1; then
        echo -e "${GREEN}✓${NC}"
    else
        echo -e "${RED}✗${NC}"
        echo "ERROR: 校验和生成失败" >> "${log_file}"
        return 1
    fi

    # 6. 验证校验和
    echo -n "  验证校验和... "
    if cmake --build "${build_dir}" --target verify-checksums >> "${log_file}" 2>&1; then
        echo -e "${GREEN}✓${NC}"
    else
        echo -e "${RED}✗${NC}"
        echo "ERROR: 校验和验证失败" >> "${log_file}"
        return 1
    fi

    return 0
}

# 主测试循环
for config in ${CONFIGS}; do
    echo ""
    echo "=========================================="
    echo "测试配置: ${config}"
    echo "=========================================="

    CONFIG_PASSED=0
    CONFIG_FAILED=0

    for i in $(seq 1 ${ITERATIONS}); do
        TOTAL_TESTS=$((TOTAL_TESTS + 1))

        if test_config "${config}" "${i}"; then
            PASSED_TESTS=$((PASSED_TESTS + 1))
            CONFIG_PASSED=$((CONFIG_PASSED + 1))
        else
            FAILED_TESTS=$((FAILED_TESTS + 1))
            CONFIG_FAILED=$((CONFIG_FAILED + 1))
            echo -e "${RED}✗ 测试失败: ${config} 迭代 ${i}${NC}"
            echo "FAILED: ${config} iteration ${i}" >> "${SUMMARY_LOG}"

            # 如果连续失败 3 次，停止该配置的测试
            if [ ${CONFIG_FAILED} -ge 3 ]; then
                echo -e "${RED}连续失败 3 次，跳过 ${config} 的剩余测试${NC}"
                echo "SKIPPED: ${config} after 3 consecutive failures" >> "${SUMMARY_LOG}"
                break
            fi
        fi

        # 每 10 次迭代显示进度
        if [ $((i % 10)) -eq 0 ]; then
            echo -e "${YELLOW}进度: ${i}/${ITERATIONS} (通过: ${CONFIG_PASSED}, 失败: ${CONFIG_FAILED})${NC}"
        fi
    done

    echo ""
    echo "配置 ${config} 测试完成:"
    echo "  通过: ${CONFIG_PASSED}"
    echo "  失败: ${CONFIG_FAILED}"
    echo ""

    # 记录到摘要
    cat >> "${SUMMARY_LOG}" << EOF

配置: ${config}
  通过: ${CONFIG_PASSED}
  失败: ${CONFIG_FAILED}
  成功率: $(awk "BEGIN {printf \"%.2f\", ${CONFIG_PASSED}*100/(${CONFIG_PASSED}+${CONFIG_FAILED})}")%

EOF
done

# 计算总耗时
END_TIME=$(date +%s)
DURATION=$((END_TIME - START_TIME))
HOURS=$((DURATION / 3600))
MINUTES=$(((DURATION % 3600) / 60))
SECONDS=$((DURATION % 60))

# 最终报告
echo ""
echo "=========================================="
echo "压力测试完成"
echo "=========================================="
echo "总测试数: ${TOTAL_TESTS}"
echo "通过: ${PASSED_TESTS}"
echo "失败: ${FAILED_TESTS}"
echo "成功率: $(awk "BEGIN {printf \"%.2f\", ${PASSED_TESTS}*100/${TOTAL_TESTS}}")%"
echo "总耗时: ${HOURS}h ${MINUTES}m ${SECONDS}s"
echo "日志目录: ${LOG_DIR}"
echo "摘要日志: ${SUMMARY_LOG}"
echo "=========================================="

# 写入最终摘要
cat >> "${SUMMARY_LOG}" << EOF

========================================
总体统计
========================================
总测试数: ${TOTAL_TESTS}
通过: ${PASSED_TESTS}
失败: ${FAILED_TESTS}
成功率: $(awk "BEGIN {printf \"%.2f\", ${PASSED_TESTS}*100/${TOTAL_TESTS}}")%
总耗时: ${HOURS}h ${MINUTES}m ${SECONDS}s
结束时间: $(date)
========================================
EOF

# 如果有失败，返回非零退出码
if [ ${FAILED_TESTS} -gt 0 ]; then
    echo -e "${RED}警告: 发现 ${FAILED_TESTS} 个失败的测试${NC}"
    exit 1
else
    echo -e "${GREEN}所有测试通过！${NC}"
    exit 0
fi

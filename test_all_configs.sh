#!/bin/bash
# 测试所有 defconfig 配置的编译

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# 日志目录
LOG_DIR="test_logs"
mkdir -p "$LOG_DIR"

# 配置列表
CONFIGS=(
    "osal_test_x86"
    "osal_test_arm64"
    "hal_test_linux"
    "pcl_test"
    "pdl_test"
    "acl_test"
    "system_test"
    "stress_test"
    "full_test"
)

# 测试结果
PASSED=0
FAILED=0
FAILED_CONFIGS=()

echo "========================================"
echo "EMS SDK Configuration Build Test"
echo "========================================"
echo ""

# 测试每个配置
for config in "${CONFIGS[@]}"; do
    echo -e "${YELLOW}Testing: ${config}${NC}"
    echo "----------------------------------------"

    # 清理
    echo "  [1/3] Cleaning..."
    python3 build.py distclean > "$LOG_DIR/${config}_clean.log" 2>&1

    # 配置
    echo "  [2/3] Configuring..."
    if python3 build.py config "$config" > "$LOG_DIR/${config}_config.log" 2>&1; then
        echo -e "  ${GREEN}✓${NC} Configuration successful"
    else
        echo -e "  ${RED}✗${NC} Configuration failed"
        FAILED=$((FAILED + 1))
        FAILED_CONFIGS+=("$config (config)")
        continue
    fi

    # 编译
    echo "  [3/3] Building..."
    if python3 build.py build -v > "$LOG_DIR/${config}_build.log" 2>&1; then
        # 检查警告
        WARNINGS=$(grep -i "warning:" "$LOG_DIR/${config}_build.log" | wc -l)
        if [ "$WARNINGS" -gt 0 ]; then
            echo -e "  ${YELLOW}⚠${NC} Build successful with $WARNINGS warnings"
        else
            echo -e "  ${GREEN}✓${NC} Build successful (no warnings)"
        fi
        PASSED=$((PASSED + 1))
    else
        echo -e "  ${RED}✗${NC} Build failed"
        FAILED=$((FAILED + 1))
        FAILED_CONFIGS+=("$config (build)")
    fi

    echo ""
done

# 总结
echo "========================================"
echo "Test Summary"
echo "========================================"
echo -e "Total configs: ${#CONFIGS[@]}"
echo -e "${GREEN}Passed: $PASSED${NC}"
echo -e "${RED}Failed: $FAILED${NC}"

if [ $FAILED -gt 0 ]; then
    echo ""
    echo "Failed configurations:"
    for failed in "${FAILED_CONFIGS[@]}"; do
        echo -e "  ${RED}✗${NC} $failed"
    done
    echo ""
    echo "Check logs in $LOG_DIR/ for details"
    exit 1
else
    echo ""
    echo -e "${GREEN}All configurations passed!${NC}"
    exit 0
fi

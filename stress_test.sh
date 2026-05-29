#!/bin/bash
# 压力测试：多次完整的 distclean -> config -> build 循环

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 测试参数
STRESS_ROUNDS=3  # 压力测试轮数
TEST_CONFIG="full_test"  # 使用最复杂的配置

echo "========================================"
echo "EMS SDK Stress Test"
echo "========================================"
echo "Configuration: $TEST_CONFIG"
echo "Test rounds: $STRESS_ROUNDS"
echo ""

# 记录开始时间
START_TIME=$(date +%s)

# 压力测试循环
for round in $(seq 1 $STRESS_ROUNDS); do
    echo -e "${BLUE}========== Round $round/$STRESS_ROUNDS ==========${NC}"

    # 1. Distclean
    echo -e "${YELLOW}[1/3] Distclean...${NC}"
    if python3 build.py distclean > /dev/null 2>&1; then
        echo -e "  ${GREEN}✓${NC} Clean successful"
    else
        echo -e "  ${RED}✗${NC} Clean failed"
        exit 1
    fi

    # 验证清理完成
    if [ -d "_build" ]; then
        echo -e "  ${RED}✗${NC} Build directory still exists after distclean"
        exit 1
    fi

    # 2. Config
    echo -e "${YELLOW}[2/3] Configure...${NC}"
    CONFIG_START=$(date +%s)
    if python3 build.py config "$TEST_CONFIG" > /dev/null 2>&1; then
        CONFIG_END=$(date +%s)
        CONFIG_TIME=$((CONFIG_END - CONFIG_START))
        echo -e "  ${GREEN}✓${NC} Configuration successful (${CONFIG_TIME}s)"
    else
        echo -e "  ${RED}✗${NC} Configuration failed"
        exit 1
    fi

    # 验证配置文件生成
    if [ ! -f "_build/config/global_config.cmake" ]; then
        echo -e "  ${RED}✗${NC} Configuration files not generated"
        exit 1
    fi

    # 3. Build
    echo -e "${YELLOW}[3/3] Build...${NC}"
    BUILD_START=$(date +%s)
    if python3 build.py build > /tmp/stress_build_round${round}.log 2>&1; then
        BUILD_END=$(date +%s)
        BUILD_TIME=$((BUILD_END - BUILD_START))

        # 检查警告
        WARNINGS=$(grep -i "warning:" /tmp/stress_build_round${round}.log | wc -l)

        if [ "$WARNINGS" -gt 0 ]; then
            echo -e "  ${YELLOW}⚠${NC} Build successful with $WARNINGS warnings (${BUILD_TIME}s)"
        else
            echo -e "  ${GREEN}✓${NC} Build successful (${BUILD_TIME}s)"
        fi
    else
        echo -e "  ${RED}✗${NC} Build failed"
        echo "Check log: /tmp/stress_build_round${round}.log"
        exit 1
    fi

    # 验证构建产物
    if [ ! -d "_build/products" ]; then
        echo -e "  ${RED}✗${NC} Build products directory not created"
        exit 1
    fi

    # 统计库文件
    LIB_COUNT=$(find _build/products -name "*.so" -o -name "*.a" | wc -l)
    echo -e "  ${GREEN}✓${NC} Generated $LIB_COUNT library files"

    # 统计可执行文件（排除 CMake 生成的测试文件）
    BIN_COUNT=$(find _build/products -type f -executable -name "collector" -o -name "comm" -o -name "logger" -o -name "health" -o -name "supervisor" | wc -l)
    if [ "$BIN_COUNT" -gt 0 ]; then
        echo -e "  ${GREEN}✓${NC} Generated $BIN_COUNT executable files"
    fi

    echo ""
done

# 记录结束时间
END_TIME=$(date +%s)
TOTAL_TIME=$((END_TIME - START_TIME))

echo "========================================"
echo "Stress Test Summary"
echo "========================================"
echo -e "Total rounds: $STRESS_ROUNDS"
echo -e "Total time: ${TOTAL_TIME}s (avg: $((TOTAL_TIME / STRESS_ROUNDS))s per round)"
echo -e "${GREEN}All stress test rounds passed!${NC}"
echo ""

# 最终验证：检查当前构建状态
echo "Final build verification:"
echo "----------------------------------------"

# 检查库文件
echo "Libraries:"
find _build/products -name "*.so" -o -name "*.a" | while read lib; do
    SIZE=$(du -h "$lib" | cut -f1)
    echo "  - $(basename $lib): $SIZE"
done

# 检查可执行文件
echo ""
echo "Executables:"
find _build/products -type f -executable \( -name "collector" -o -name "comm" -o -name "logger" -o -name "health" -o -name "supervisor" \) | while read exe; do
    SIZE=$(du -h "$exe" | cut -f1)
    echo "  - $(basename $exe): $SIZE"
done

echo ""
echo -e "${GREEN}✓ Stress test completed successfully!${NC}"

#!/bin/bash
# 测试运行脚本

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 默认配置
BUILD_DIR="build"
TEST_BINARY="$BUILD_DIR/bin/ems-test"
TEST_TYPE="all"
VERBOSE=0
REPORT_DIR="test_reports"

# 打印帮助信息
print_help() {
    cat << EOF
Usage: $0 [OPTIONS]

Options:
    -t, --type TYPE         Test type: unit, stress, performance, system, all (default: all)
    -l, --layer LAYER       Test specific layer: OSAL, HAL, PDL, PCL, ACL
    -m, --module MODULE     Test specific module
    -v, --verbose           Verbose output
    -r, --report            Generate test report
    -h, --help              Show this help message

Examples:
    $0                              # Run all tests
    $0 -t unit                      # Run unit tests only
    $0 -t performance -l OSAL       # Run OSAL performance tests
    $0 -t stress -v                 # Run stress tests with verbose output
    $0 -t system -r                 # Run system tests and generate report

EOF
}

# 解析命令行参数
while [[ $# -gt 0 ]]; do
    case $1 in
        -t|--type)
            TEST_TYPE="$2"
            shift 2
            ;;
        -l|--layer)
            LAYER="$2"
            shift 2
            ;;
        -m|--module)
            MODULE="$2"
            shift 2
            ;;
        -v|--verbose)
            VERBOSE=1
            shift
            ;;
        -r|--report)
            GENERATE_REPORT=1
            shift
            ;;
        -h|--help)
            print_help
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            print_help
            exit 1
            ;;
    esac
done

# 检查测试二进制文件
if [ ! -f "$TEST_BINARY" ]; then
    echo -e "${RED}Error: Test binary not found: $TEST_BINARY${NC}"
    echo "Please build the project first: ./build.sh -d"
    exit 1
fi

# 创建报告目录
if [ "$GENERATE_REPORT" = "1" ]; then
    mkdir -p "$REPORT_DIR"
    TIMESTAMP=$(date +%Y%m%d_%H%M%S)
    REPORT_FILE="$REPORT_DIR/test_report_${TIMESTAMP}.txt"
fi

# 运行测试
run_tests() {
    local test_args=""

    # 构建测试参数
    if [ -n "$LAYER" ]; then
        test_args="--layer $LAYER"
    elif [ -n "$MODULE" ]; then
        test_args="--module $MODULE"
    else
        case $TEST_TYPE in
            unit)
                test_args="--layer UNIT"
                ;;
            stress)
                test_args="--layer STRESS"
                ;;
            performance)
                test_args="--layer PERFORMANCE"
                ;;
            system)
                test_args="--layer SYSTEM"
                ;;
            all)
                test_args="--all"
                ;;
            *)
                echo -e "${RED}Error: Unknown test type: $TEST_TYPE${NC}"
                exit 1
                ;;
        esac
    fi

    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}Running EMS Tests${NC}"
    echo -e "${BLUE}========================================${NC}"
    echo -e "Test Type: ${GREEN}$TEST_TYPE${NC}"
    [ -n "$LAYER" ] && echo -e "Layer: ${GREEN}$LAYER${NC}"
    [ -n "$MODULE" ] && echo -e "Module: ${GREEN}$MODULE${NC}"
    echo -e "${BLUE}========================================${NC}"
    echo ""

    # 运行测试
    if [ "$GENERATE_REPORT" = "1" ]; then
        if [ "$VERBOSE" = "1" ]; then
            $TEST_BINARY $test_args 2>&1 | tee "$REPORT_FILE"
        else
            $TEST_BINARY $test_args > "$REPORT_FILE" 2>&1
            cat "$REPORT_FILE"
        fi
        echo ""
        echo -e "${GREEN}Test report saved to: $REPORT_FILE${NC}"
    else
        $TEST_BINARY $test_args
    fi

    # 检查测试结果
    local exit_code=$?
    echo ""
    if [ $exit_code -eq 0 ]; then
        echo -e "${GREEN}========================================${NC}"
        echo -e "${GREEN}All tests passed!${NC}"
        echo -e "${GREEN}========================================${NC}"
    else
        echo -e "${RED}========================================${NC}"
        echo -e "${RED}Some tests failed!${NC}"
        echo -e "${RED}========================================${NC}"
        exit $exit_code
    fi
}

# 运行测试
run_tests

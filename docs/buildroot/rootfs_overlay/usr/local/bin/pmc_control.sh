#!/bin/bash
# PMC进程启动脚本
# 按正确顺序启动5个进程

set -e

# 配置
BIN_DIR="./bin"
LOG_DIR="/var/log/pmc"
PID_DIR="/var/run/pmc"

# 创建必要的目录
mkdir -p "$LOG_DIR"
mkdir -p "$PID_DIR"

# 颜色输出
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# 日志函数
log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# 检查进程是否运行
is_running() {
    local name=$1
    local pid_file="$PID_DIR/$name.pid"

    if [ -f "$pid_file" ]; then
        local pid=$(cat "$pid_file")
        if kill -0 "$pid" 2>/dev/null; then
            return 0
        fi
    fi
    return 1
}

# 启动进程
start_process() {
    local name=$1
    local bin="$BIN_DIR/$name"

    if [ ! -x "$bin" ]; then
        log_error "可执行文件不存在: $bin"
        return 1
    fi

    if is_running "$name"; then
        log_warn "$name 已经在运行"
        return 0
    fi

    log_info "启动 $name..."
    "$bin" &
    local pid=$!
    echo "$pid" > "$PID_DIR/$name.pid"

    # 等待进程启动
    sleep 0.5

    if kill -0 "$pid" 2>/dev/null; then
        log_info "$name 启动成功 (PID: $pid)"
        return 0
    else
        log_error "$name 启动失败"
        rm -f "$PID_DIR/$name.pid"
        return 1
    fi
}

# 停止进程
stop_process() {
    local name=$1
    local pid_file="$PID_DIR/$name.pid"

    if [ ! -f "$pid_file" ]; then
        log_warn "$name 未运行"
        return 0
    fi

    local pid=$(cat "$pid_file")

    if ! kill -0 "$pid" 2>/dev/null; then
        log_warn "$name 进程不存在 (PID: $pid)"
        rm -f "$pid_file"
        return 0
    fi

    log_info "停止 $name (PID: $pid)..."
    kill -TERM "$pid"

    # 等待进程退出（最多5秒）
    for i in {1..50}; do
        if ! kill -0 "$pid" 2>/dev/null; then
            log_info "$name 已停止"
            rm -f "$pid_file"
            return 0
        fi
        sleep 0.1
    done

    # 强制杀死
    log_warn "强制杀死 $name"
    kill -KILL "$pid" 2>/dev/null || true
    rm -f "$pid_file"
    return 0
}

# 启动所有进程
start_all() {
    log_info "启动PMC系统..."

    # 按顺序启动进程
    # 1. Logger进程（最先启动，收集其他进程的日志）
    start_process "pmc_logger" || exit 1
    sleep 0.5

    # 2. Communication进程（通信进程）
    start_process "pmc_comm" || exit 1
    sleep 0.5

    # 3. Collector进程（数据采集）
    start_process "pmc_collector" || exit 1
    sleep 0.5

    # 4. Health进程（健康监测）
    start_process "pmc_health" || exit 1
    sleep 0.5

    # 5. Supervisor进程（最后启动，监督其他进程）
    start_process "pmc_supervisor" || exit 1

    log_info "PMC系统启动完成"
}

# 停止所有进程
stop_all() {
    log_info "停止PMC系统..."

    # 按相反顺序停止进程
    stop_process "pmc_supervisor"
    stop_process "pmc_health"
    stop_process "pmc_collector"
    stop_process "pmc_comm"
    stop_process "pmc_logger"

    log_info "PMC系统已停止"
}

# 重启所有进程
restart_all() {
    stop_all
    sleep 1
    start_all
}

# 显示状态
status_all() {
    log_info "PMC系统状态:"

    local processes=("pmc_logger" "pmc_comm" "pmc_collector" "pmc_health" "pmc_supervisor")

    for name in "${processes[@]}"; do
        if is_running "$name"; then
            local pid=$(cat "$PID_DIR/$name.pid")
            echo -e "  ${GREEN}●${NC} $name (PID: $pid)"
        else
            echo -e "  ${RED}○${NC} $name (未运行)"
        fi
    done
}

# 主函数
main() {
    case "${1:-}" in
        start)
            start_all
            ;;
        stop)
            stop_all
            ;;
        restart)
            restart_all
            ;;
        status)
            status_all
            ;;
        *)
            echo "用法: $0 {start|stop|restart|status}"
            echo ""
            echo "命令:"
            echo "  start   - 启动所有PMC进程"
            echo "  stop    - 停止所有PMC进程"
            echo "  restart - 重启所有PMC进程"
            echo "  status  - 显示进程状态"
            exit 1
            ;;
    esac
}

main "$@"

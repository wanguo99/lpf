#!/bin/bash
#
# OSAL 精简优化计划进度管理脚本
# 用于更新任务状态、记录 Git 提交、生成进度报告
#

set -e

PLAN_FILE="$(dirname "$0")/OPTIMIZATION_PLAN_SIMPLIFIED.md"
BACKUP_DIR="$(dirname "$0")/.progress_backup"

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 创建备份目录
mkdir -p "$BACKUP_DIR"

# 显示帮助信息
show_help() {
    cat << EOF
OSAL 精简优化计划进度管理工具

用法:
    $0 <命令> [参数]

命令:
    start <任务ID>              开始一项任务（状态改为 IN PROGRESS）
    done <任务ID> [commit]      完成一项任务（状态改为 DONE，记录提交哈希）
    status                      显示当前进度统计
    list                        列出所有任务
    commit <任务ID> <消息>      提交代码并自动更新计划

示例:
    # 开始任务
    $0 start T0.1

    # 完成任务（自动获取最新提交哈希）
    $0 done T0.1

    # 完成任务（手动指定提交哈希）
    $0 done T0.1 a1b2c3d

    # 一键提交代码并更新计划
    $0 commit T0.1 "重构：统一使用固定长度类型"

    # 查看进度
    $0 status

    # 列出所有任务
    $0 list

EOF
}

# 备份计划文件
backup_plan() {
    local timestamp=$(date +%Y%m%d_%H%M%S)
    cp "$PLAN_FILE" "$BACKUP_DIR/OPTIMIZATION_PLAN_SIMPLIFIED_${timestamp}.md"
    echo -e "${GREEN}✓${NC} 已备份计划文件到: $BACKUP_DIR/OPTIMIZATION_PLAN_SIMPLIFIED_${timestamp}.md"
}

# 更新任务状态
update_task_status() {
    local task_id="$1"
    local new_status="$2"
    local commit_hash="$3"

    backup_plan

    # 状态符号映射
    local status_symbol
    case "$new_status" in
        "TODO") status_symbol="⬜ TODO" ;;
        "IN_PROGRESS") status_symbol="🔄 IN PROGRESS" ;;
        "DONE") status_symbol="✅ DONE" ;;
        *) echo -e "${RED}✗${NC} 未知状态: $new_status"; exit 1 ;;
    esac

    # 转义任务 ID 中的特殊字符（如 .）
    local escaped_task_id=$(echo "$task_id" | sed 's/\./\\./g')

    # 使用 sed 更新任务状态（保留原有的优先级和工时）
    if [ -n "$commit_hash" ]; then
        # 更新状态和提交哈希
        sed -i "s/| $escaped_task_id | \([^|]*\) | \([^|]*\) | ⬜ TODO |  |/| $task_id | \1 | \2 | $status_symbol | $commit_hash |/g" "$PLAN_FILE"
        sed -i "s/| $escaped_task_id | \([^|]*\) | \([^|]*\) | 🔄 IN PROGRESS | \([^|]*\) |/| $task_id | \1 | \2 | $status_symbol | $commit_hash |/g" "$PLAN_FILE"
    else
        # 只更新状态
        sed -i "s/| $escaped_task_id | \([^|]*\) | \([^|]*\) | ⬜ TODO |/| $task_id | \1 | \2 | $status_symbol |/g" "$PLAN_FILE"
        sed -i "s/| $escaped_task_id | \([^|]*\) | \([^|]*\) | ✅ DONE |/| $task_id | \1 | \2 | $status_symbol |/g" "$PLAN_FILE"
    fi

    # 更新最后更新日期
    local today=$(date +%Y-%m-%d)
    sed -i "s/\*\*最后更新\*\*:.*/\*\*最后更新\*\*: $today/g" "$PLAN_FILE"

    echo -e "${GREEN}✓${NC} 任务 $task_id 状态已更新为: $status_symbol"
}

# 开始任务
start_task() {
    local task_id="$1"

    if [ -z "$task_id" ]; then
        echo -e "${RED}✗${NC} 错误: 请指定任务 ID"
        echo "用法: $0 start <任务ID>"
        exit 1
    fi

    update_task_status "$task_id" "IN_PROGRESS"

    # 提交计划文档更新
    git add "$PLAN_FILE"
    git commit -m "文档：开始任务 $task_id" || true

    echo -e "${BLUE}→${NC} 任务 $task_id 已开始"
}

# 完成任务
done_task() {
    local task_id="$1"
    local commit_hash="$2"

    if [ -z "$task_id" ]; then
        echo -e "${RED}✗${NC} 错误: 请指定任务 ID"
        echo "用法: $0 done <任务ID> [commit_hash]"
        exit 1
    fi

    # 如果未指定提交哈希，自动获取最新提交
    if [ -z "$commit_hash" ]; then
        commit_hash=$(git rev-parse --short HEAD)
        echo -e "${BLUE}→${NC} 使用最新提交: $commit_hash"
    fi

    update_task_status "$task_id" "DONE" "$commit_hash"

    # 提交计划文档更新
    git add "$PLAN_FILE"
    git commit -m "文档：完成任务 $task_id ($commit_hash)" || true

    echo -e "${GREEN}✓${NC} 任务 $task_id 已完成"
}

# 一键提交代码并更新计划
commit_and_update() {
    local task_id="$1"
    shift
    local commit_message="$*"

    if [ -z "$task_id" ] || [ -z "$commit_message" ]; then
        echo -e "${RED}✗${NC} 错误: 请指定任务 ID 和提交消息"
        echo "用法: $0 commit <任务ID> <提交消息>"
        exit 1
    fi

    # 提交代码
    echo -e "${BLUE}→${NC} 提交代码..."
    git add -A
    git commit -m "$commit_message"
    local commit_hash=$(git rev-parse --short HEAD)
    echo -e "${GREEN}✓${NC} 代码已提交: $commit_hash"

    # 更新计划
    echo -e "${BLUE}→${NC} 更新优化计划..."
    update_task_status "$task_id" "DONE" "$commit_hash"

    # 提交计划文档更新
    git add "$PLAN_FILE"
    git commit -m "文档：完成任务 $task_id ($commit_hash)" || true

    echo -e "${GREEN}✓${NC} 任务 $task_id 已完成并记录"
}

# 显示进度统计
show_status() {
    echo -e "${BLUE}=== OSAL 精简优化计划进度统计 ===${NC}"
    echo ""

    local total=$(grep -c "^| T[0-9]" "$PLAN_FILE" 2>/dev/null | tr -d '\n' || echo "0")
    local done=$(grep -c "✅ DONE" "$PLAN_FILE" 2>/dev/null | tr -d '\n' || echo "0")
    local in_progress=$(grep -c "🔄 IN PROGRESS" "$PLAN_FILE" 2>/dev/null | tr -d '\n' || echo "0")
    local todo=$(grep -c "⬜ TODO" "$PLAN_FILE" 2>/dev/null | tr -d '\n' || echo "0")

    local percent=0
    if [ "$total" -gt 0 ]; then
        percent=$((done * 100 / total))
    fi

    local bar_length=50
    local filled=0
    local empty=$bar_length
    if [ "$total" -gt 0 ]; then
        filled=$((percent * bar_length / 100))
        empty=$((bar_length - filled))
    fi

    echo -e "总任务数:   ${BLUE}$total${NC}"
    echo -e "已完成:     ${GREEN}$done${NC} (${percent}%)"
    echo -e "进行中:     ${YELLOW}$in_progress${NC}"
    echo -e "待开始:     $todo"
    echo ""

    # 显示进度条
    echo -n "进度: ["
    if [ "$filled" -gt 0 ]; then
        printf "%${filled}s" | tr ' ' '='
    fi
    if [ "$empty" -gt 0 ]; then
        printf "%${empty}s" | tr ' ' '-'
    fi
    echo "] ${percent}%"
    echo ""

    # 显示当前进行中的任务
    if [ "$in_progress" -gt 0 ]; then
        echo -e "${YELLOW}当前进行中的任务:${NC}"
        grep "🔄 IN PROGRESS" "$PLAN_FILE" | sed 's/^| \(T[^ ]*\) |.*/  - \1/'
        echo ""
    fi
}

# 列出所有任务
list_tasks() {
    echo -e "${BLUE}=== OSAL 精简优化计划任务列表 ===${NC}\n"

    # 提取所有任务行
    grep "^| T[0-9]" "$PLAN_FILE" | while IFS='|' read -r _ id _ priority _ time _ status _; do
        # 去除空格
        id=$(echo "$id" | xargs)
        priority=$(echo "$priority" | xargs)
        time=$(echo "$time" | xargs)
        status=$(echo "$status" | xargs)

        # 根据状态设置颜色
        case "$status" in
            *"DONE"*) color=$GREEN ;;
            *"IN PROGRESS"*) color=$YELLOW ;;
            *"TODO"*) color=$NC ;;
            *) color=$NC ;;
        esac

        echo -e "${color}[$id]${NC} $priority | $time | $status"
    done

    echo ""
}

# 主函数
main() {
    if [ $# -eq 0 ]; then
        show_help
        exit 0
    fi

    local command="$1"
    shift

    case "$command" in
        start)
            start_task "$@"
            ;;
        done)
            done_task "$@"
            ;;
        commit)
            commit_and_update "$@"
            ;;
        status)
            show_status
            ;;
        list)
            list_tasks
            ;;
        help|--help|-h)
            show_help
            ;;
        *)
            echo -e "${RED}✗${NC} 未知命令: $command"
            show_help
            exit 1
            ;;
    esac
}

main "$@"

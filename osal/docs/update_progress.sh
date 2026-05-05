#!/bin/bash
#
# OSAL 优化计划进度管理脚本
# 用于更新任务状态、记录 Git 提交、生成进度报告
#

set -e

PLAN_FILE="$(dirname "$0")/OPTIMIZATION_PLAN.md"
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
OSAL 优化计划进度管理工具

用法:
    $0 <命令> [参数]

命令:
    start <任务ID>              开始一项任务（状态改为 IN PROGRESS）
    done <任务ID> [commit]      完成一项任务（状态改为 DONE，记录提交哈希）
    block <任务ID> <原因>       阻塞一项任务（状态改为 BLOCKED）
    cancel <任务ID> <原因>      取消一项任务（状态改为 CANCELLED）
    status                      显示当前进度统计
    list [阶段]                 列出所有任务（可选：按阶段过滤）
    commit <任务ID> <消息>      提交代码并自动更新计划
    report                      生成详细进度报告

示例:
    # 开始任务
    $0 start T1.1.1

    # 完成任务（自动获取最新提交哈希）
    $0 done T1.1.1

    # 完成任务（手动指定提交哈希）
    $0 done T1.1.1 a1b2c3d

    # 一键提交代码并更新计划
    $0 commit T1.1.1 "新增：补充硬件寄存器类型"

    # 查看进度
    $0 status

    # 列出阶段 1 的所有任务
    $0 list 1

    # 生成详细报告
    $0 report

EOF
}

# 备份计划文件
backup_plan() {
    local timestamp=$(date +%Y%m%d_%H%M%S)
    cp "$PLAN_FILE" "$BACKUP_DIR/OPTIMIZATION_PLAN_${timestamp}.md"
    echo -e "${GREEN}✓${NC} 已备份计划文件到: $BACKUP_DIR/OPTIMIZATION_PLAN_${timestamp}.md"
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
        "BLOCKED") status_symbol="⏸️ BLOCKED" ;;
        "CANCELLED") status_symbol="❌ CANCELLED" ;;
        *) echo -e "${RED}✗${NC} 未知状态: $new_status"; exit 1 ;;
    esac

    # 使用 sed 更新任务状态
    if [ -n "$commit_hash" ]; then
        # 更新状态和提交哈希
        sed -i "s/| $task_id |.*| ⬜ TODO |  |/| $task_id | P0 | 2h | $status_symbol | $commit_hash |/g" "$PLAN_FILE"
        sed -i "s/| $task_id |.*| 🔄 IN PROGRESS |.*|/| $task_id | P0 | 2h | $status_symbol | $commit_hash |/g" "$PLAN_FILE"
    else
        # 只更新状态
        sed -i "s/| $task_id |.*| ⬜ TODO |/| $task_id | P0 | 2h | $status_symbol |/g" "$PLAN_FILE"
        sed -i "s/| $task_id |.*| 🔄 IN PROGRESS |/| $task_id | P0 | 2h | $status_symbol |/g" "$PLAN_FILE"
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
        commit_hash=$(git log -1 --format=%h)
        echo -e "${BLUE}→${NC} 使用最新提交: $commit_hash"
    fi

    update_task_status "$task_id" "DONE" "$commit_hash"

    # 提交计划文档更新
    git add "$PLAN_FILE"
    git commit -m "文档：完成任务 $task_id (提交: $commit_hash)" || true

    echo -e "${GREEN}✓${NC} 任务 $task_id 已完成"
}

# 阻塞任务
block_task() {
    local task_id="$1"
    local reason="$2"

    if [ -z "$task_id" ] || [ -z "$reason" ]; then
        echo -e "${RED}✗${NC} 错误: 请指定任务 ID 和阻塞原因"
        echo "用法: $0 block <任务ID> <原因>"
        exit 1
    fi

    update_task_status "$task_id" "BLOCKED"

    # 提交计划文档更新
    git add "$PLAN_FILE"
    git commit -m "文档：阻塞任务 $task_id - $reason" || true

    echo -e "${YELLOW}⏸${NC} 任务 $task_id 已阻塞: $reason"
}

# 取消任务
cancel_task() {
    local task_id="$1"
    local reason="$2"

    if [ -z "$task_id" ] || [ -z "$reason" ]; then
        echo -e "${RED}✗${NC} 错误: 请指定任务 ID 和取消原因"
        echo "用法: $0 cancel <任务ID> <原因>"
        exit 1
    fi

    update_task_status "$task_id" "CANCELLED"

    # 提交计划文档更新
    git add "$PLAN_FILE"
    git commit -m "文档：取消任务 $task_id - $reason" || true

    echo -e "${RED}✗${NC} 任务 $task_id 已取消: $reason"
}

# 显示进度统计
show_status() {
    echo -e "${BLUE}=== OSAL 优化计划进度统计 ===${NC}\n"

    local total=$(grep -c "| T[0-9]" "$PLAN_FILE" || echo 0)
    local done=$(grep -c "✅ DONE" "$PLAN_FILE" || echo 0)
    local in_progress=$(grep -c "🔄 IN PROGRESS" "$PLAN_FILE" || echo 0)
    local todo=$(grep -c "⬜ TODO" "$PLAN_FILE" || echo 0)
    local blocked=$(grep -c "⏸️ BLOCKED" "$PLAN_FILE" || echo 0)
    local cancelled=$(grep -c "❌ CANCELLED" "$PLAN_FILE" || echo 0)

    local percent=0
    if [ $total -gt 0 ]; then
        percent=$((done * 100 / total))
    fi

    echo -e "${GREEN}总任务数:${NC} $total"
    echo -e "${GREEN}已完成:${NC}   $done  (${percent}%)"
    echo -e "${YELLOW}进行中:${NC}   $in_progress"
    echo -e "${BLUE}待开始:${NC}   $todo"
    echo -e "${YELLOW}已阻塞:${NC}   $blocked"
    echo -e "${RED}已取消:${NC}   $cancelled"

    echo -e "\n${BLUE}=== 按优先级统计 ===${NC}\n"

    local p0_total=$(grep "| P0 |" "$PLAN_FILE" | wc -l)
    local p0_done=$(grep "| P0 |.*✅ DONE" "$PLAN_FILE" | wc -l)
    local p1_total=$(grep "| P1 |" "$PLAN_FILE" | wc -l)
    local p1_done=$(grep "| P1 |.*✅ DONE" "$PLAN_FILE" | wc -l)
    local p2_total=$(grep "| P2 |" "$PLAN_FILE" | wc -l)
    local p2_done=$(grep "| P2 |.*✅ DONE" "$PLAN_FILE" | wc -l)

    echo -e "${RED}P0 (核心基础):${NC}  $p0_done / $p0_total"
    echo -e "${YELLOW}P1 (重要功能):${NC}  $p1_done / $p1_total"
    echo -e "${BLUE}P2 (增强功能):${NC}  $p2_done / $p2_total"

    echo -e "\n${BLUE}=== 进度条 ===${NC}\n"

    local bar_length=50
    local filled=$((percent * bar_length / 100))
    local empty=$((bar_length - filled))

    printf "["
    printf "%${filled}s" | tr ' ' '='
    printf "%${empty}s" | tr ' ' '-'
    printf "] ${percent}%%\n"
}

# 列出任务
list_tasks() {
    local stage="$1"

    echo -e "${BLUE}=== OSAL 优化任务列表 ===${NC}\n"

    if [ -n "$stage" ]; then
        echo -e "${YELLOW}阶段 $stage:${NC}\n"
        grep "| T${stage}\." "$PLAN_FILE" | head -20
    else
        echo -e "${YELLOW}所有任务:${NC}\n"
        grep "| T[0-9]" "$PLAN_FILE" | head -50
    fi
}

# 一键提交代码并更新计划
commit_and_update() {
    local task_id="$1"
    shift
    local message="$*"

    if [ -z "$task_id" ] || [ -z "$message" ]; then
        echo -e "${RED}✗${NC} 错误: 请指定任务 ID 和提交消息"
        echo "用法: $0 commit <任务ID> <提交消息>"
        exit 1
    fi

    # 提交代码
    echo -e "${BLUE}→${NC} 提交代码变更..."
    git add -A
    git commit -m "$message

任务: $task_id"

    local commit_hash=$(git log -1 --format=%h)
    echo -e "${GREEN}✓${NC} 代码已提交: $commit_hash"

    # 更新计划
    done_task "$task_id" "$commit_hash"
}

# 生成详细报告
generate_report() {
    local report_file="$(dirname "$0")/PROGRESS_REPORT_$(date +%Y%m%d).md"

    echo -e "${BLUE}→${NC} 生成进度报告..."

    cat > "$report_file" << EOF
# OSAL 优化计划进度报告

**生成日期**: $(date +%Y-%m-%d\ %H:%M:%S)

## 总体进度

EOF

    # 统计数据
    local total=$(grep -c "| T[0-9]" "$PLAN_FILE" || echo 0)
    local done=$(grep -c "✅ DONE" "$PLAN_FILE" || echo 0)
    local in_progress=$(grep -c "🔄 IN PROGRESS" "$PLAN_FILE" || echo 0)
    local todo=$(grep -c "⬜ TODO" "$PLAN_FILE" || echo 0)
    local percent=$((done * 100 / total))

    cat >> "$report_file" << EOF
- 总任务数: $total
- 已完成: $done ($percent%)
- 进行中: $in_progress
- 待开始: $todo

## 已完成任务

EOF

    # 列出已完成任务
    grep "✅ DONE" "$PLAN_FILE" >> "$report_file" || echo "暂无已完成任务" >> "$report_file"

    cat >> "$report_file" << EOF

## 进行中任务

EOF

    # 列出进行中任务
    grep "🔄 IN PROGRESS" "$PLAN_FILE" >> "$report_file" || echo "暂无进行中任务" >> "$report_file"

    cat >> "$report_file" << EOF

## 最近提交

EOF

    # 列出最近 10 次相关提交
    git log --grep="任务: T" --oneline -10 >> "$report_file" || echo "暂无相关提交" >> "$report_file"

    echo -e "${GREEN}✓${NC} 报告已生成: $report_file"
}

# 主函数
main() {
    if [ ! -f "$PLAN_FILE" ]; then
        echo -e "${RED}✗${NC} 错误: 找不到计划文件: $PLAN_FILE"
        exit 1
    fi

    case "$1" in
        start)
            start_task "$2"
            ;;
        done)
            done_task "$2" "$3"
            ;;
        block)
            block_task "$2" "$3"
            ;;
        cancel)
            cancel_task "$2" "$3"
            ;;
        status)
            show_status
            ;;
        list)
            list_tasks "$2"
            ;;
        commit)
            shift
            commit_and_update "$@"
            ;;
        report)
            generate_report
            ;;
        help|--help|-h)
            show_help
            ;;
        *)
            echo -e "${RED}✗${NC} 未知命令: $1"
            echo ""
            show_help
            exit 1
            ;;
    esac
}

main "$@"

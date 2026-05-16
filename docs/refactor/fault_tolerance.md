# 容错与恢复机制

## 15. 调试与故障排查

### 15.1 调试工具

实时性分析：
```shell
# 1. 查看进程调度策略

chrt -p <pid>

# 2. 查看CPU亲和性

taskset -p <pid>

# 3. 测量延迟（cyclictest）

cyclictest -p 99 -t 1 -n -i 1000 -l 100000

# 4. 查看中断延迟

cat /proc/interrupts

# 5. 跟踪系统调用（strace）

strace -p <pid> -T -tt
```

性能分析：
``` shell
# 1. CPU占用（top）

top -H -p <pid>

# 2. 内存占用（pmap）

pmap -x <pid>

# 3. 系统调用统计（strace）

strace -c -p <pid>

# 4. 性能分析（perf）

perf record -p <pid> -g
perf report

# 5. 火焰图

perf script | stackcollapse-perf.pl | flamegraph.pl > flame.svg

日志分析：

# 1. 查看实时日志

tail -f /var/log/pmc/telecommand.log

# 2. 搜索错误日志

grep ERROR /var/log/pmc/*.log

# 3. 统计错误次数

grep ERROR /var/log/pmc/*.log | wc -l

# 4. 查看崩溃日志

ls -lh /var/log/pmc/crash/

# 5. 分析状态日志（JSON）

jq '.server.power_state' /var/log/pmc/status.log
```

### 15.2 常见问题排查

问题1：2ms应答超时
症状：遥控遥测命令应答时间>2ms

排查步骤：

1. 检查Telecommand进程调度策略
``` shell
 chrt -p $(pgrep pmc_telecommand)
 # 预期：SCHED_FIFO, priority=99
```

2. 检查CPU亲和性
``` shell
 taskset -p $(pgrep pmc_telecommand)
 # 预期：affinity mask: 1（绑定CPU0）
```

3. 检查内存锁定
``` shell
 cat /proc/$(pgrep pmc_telecommand)/status | grep VmLck
 # 预期：VmLck > 0
```

4. 检查中断延迟
``` shell
 cyclictest -p 99 -t 1 -n -i 1000 -l 10000
 # 预期：Max latency < 100μs
```

5. 检查CAN总线状态
``` shell
 ip -s link show can0
 # 检查错误计数
```

解决方案：

- 使用PREEMPT_RT内核

- 禁用不必要的中断

- 优化PDL层代码

问题2：进程频繁崩溃
症状：Telemetry进程每隔几分钟崩溃一次

排查步骤：

1. 查看崩溃日志
``` shell
 cat /var/log/pmc/crash/crash_*.log
 # 查看崩溃原因（SIGSEGV/SIGABRT）
```

2. 分析coredump
``` shell
 gdb /usr/bin/pmc_telemetry /var/log/pmc/crash/core.1234
    (gdb) bt
 # 查看调用栈
```

3. 检查内存泄漏
``` shell
 valgrind --leak-check=full /usr/bin/pmc_telemetry
```

4. 检查共享内存
``` shell
 ls -lh /dev/shm/pmc_*
 # 检查共享内存是否损坏
```

解决方案：

- 修复段错误（NULL指针、数组越界）

- 修复内存泄漏

- 增加错误处理

问题3：日志占满Flash
症状：Flash空间不足，系统无法写入日志

排查步骤：

1. 检查Flash使用情况
``` shell
 df -h /var/log/pmc
 # 查看剩余空间
```

2. 检查日志文件大小
``` shell
 du -sh /var/log/pmc/*
 # 查找大文件
```

3. 检查日志轮转
``` shell
 ls -lh /var/log/pmc/archive/
 # 检查是否正常轮转
```

解决方案：

- 手动清理旧日志
``` shell
rm -f /var/log/pmc/archive/*.log.gz
```

- 调整日志轮转策略
``` shell

# 修改/etc/pmc/pmc.conf

max_log_size_mb = 5
max_archive_days = 3

- 重启Logger进程
kill -HUP $(pgrep pmc_logger)
```

---



---

## 16. 附录

### 16.1 术语表

| 术语 | 全称 | 说明 |
|------|------|------|
| PMC | Payload Management Controller | 载荷管理控制器 |
| BMC | Baseboard Management Controller | 基板管理控制器 |
| MCU | Microcontroller Unit | 微控制器 |
| FPGA | Field-Programmable Gate Array | 现场可编程门阵列 |
| Telecommand | Telecommand | 遥控，卫星平台向载荷发送的控制命令 |
| Telemetry | Telemetry | 遥测，载荷向卫星平台上报的状态数据 |
| ACL | Application Configuration Layer | 应用配置层 |
| PDL | Peripheral Driver Layer | 外设驱动层 |
| PCL | Peripheral Configuration Layer | 外设配置层 |
| HAL | Hardware Abstraction Layer | 硬件抽象层 |
| OSAL | Operating System Abstraction Layer | 操作系统抽象层 |
| SEU | Single Event Upset | 单粒子翻转 |
| SEL | Single Event Latchup | 单粒子闩锁 |
| IPC | Inter-Process Communication | 进程间通信 |
| SCHED_FIFO | First-In-First-Out Scheduling | 先进先出调度 |
| PREEMPT_RT | Real-Time Preemption | 实时抢占 |

### 16.2 参考文档

1. EMS架构文档

- docs/ARCHITECTURE.md

- docs/CODING_STANDARDS.md


2. Refactor方案

- docs/refactor/EMS_Architecture_Refactor_v3.0_SingleProcess.md

- docs/refactor/EMS_Architecture_Refactor_v3.1_MultiProcess_Final.md

3. Linux实时编程

- Linux PREEMPT_RT Documentation

- POSIX Real-Time Extensions

- Shared Memory Programming Guide

4. 航天软件标准

- NASA Software Safety Guidebook

- ESA Software Engineering Standards

- DO-178C (航空软件标准)

### 16.3 联系方式

技术支持：

- 邮箱：guohaoprc@163.com

- 文档：https://github.com/wanguo99/EMS/tree/master/docs

- 问题跟踪：https://github.com/wanguo99/EMS/issues

---
验证清单

在实施完成后，请确认以下检查项：

功能验证

- 遥控命令2ms内应答（100%成功率）

- 快遥1秒周期正常工作

- 慢遥2秒周期正常工作

- 快遥慢遥可同时进行

- 缓存型遥测返回正确数据

- 实时型遥测返回最新数据

- 固件升级成功（主备分区切换）

- 日志正常收集和轮转

性能验证

- 遥控命令延迟<500μs（平均）

- 缓存型遥测延迟<200μs（平均）

- 实时型遥测延迟<800μs（平均）

- CPU占用<60%（正常负载）

- 内存占用<50MB

- Flash占用<200MB

可靠性验证

- Telemetry进程崩溃后1秒内恢复

- Telecommand进程崩溃后1秒内恢复

- 连续崩溃5次触发系统复位

- 硬件看门狗10秒超时触发复位

- SEU模拟测试通过

- 72小时稳定性测试通过

文档验证

- 代码注释完整

- API文档完整

- 用户手册完整

- 测试报告完整

- 部署文档完整

---
方案版本：v1.0
最后更新：2026-05-17
作者：wanguo

# EMS OSAL和HAL层测试计划

## 文档信息

- **版本**: 1.0
- **日期**: 2026-05-06
- **状态**: 草案
- **目标**: 建立完整的OSAL和HAL层测试覆盖体系

## 1. 测试目标

### 1.1 总体目标

- **功能完整性**: 验证所有公共API的功能正确性
- **边界条件**: 测试参数边界、异常输入、资源限制
- **并发安全**: 验证多线程环境下的线程安全性
- **性能基准**: 建立性能基线，检测性能退化
- **平台兼容**: 验证跨平台（x86_64/ARM32/ARM64/RISC-V）兼容性
- **错误处理**: 验证错误码返回和异常恢复机制

### 1.2 测试覆盖率目标

- **代码覆盖率**: ≥85% (行覆盖)
- **分支覆盖率**: ≥80%
- **API覆盖率**: 100% (所有公共API)
- **错误路径覆盖**: ≥90%

## 2. OSAL层测试计划

### 2.1 IPC模块 (进程间通信)

#### 2.1.1 互斥锁 (osal_mutex)

**已有测试**: `tests/unit/osal/test_osal_mutex.c`

**测试用例**:
- ✅ 基础功能
  - 创建/删除互斥锁
  - 加锁/解锁操作
  - 空指针参数验证
- ✅ 并发测试
  - 多线程竞争保护共享资源
  - 递归加锁检测
- 🔲 待补充
  - 死锁检测机制验证
  - 优先级继承测试（如果支持）
  - 超时加锁测试
  - 性能基准测试（加锁/解锁延迟）

#### 2.1.2 信号量 (osal_semaphore)

**已有测试**: `tests/unit/osal/test_osal_semaphore.c`

**测试用例**:
- ✅ 基础功能
  - 创建/删除信号量
  - P/V操作（等待/释放）
  - 初始计数验证
- 🔲 待补充
  - 超时等待测试
  - 多生产者-多消费者场景
  - 信号量计数边界测试（0, MAX）
  - 性能测试（唤醒延迟）

#### 2.1.3 条件变量 (osal_cond)

**已有测试**: `tests/unit/osal/test_osal_cond.c`

**测试用例**:
- ✅ 基础功能
  - 创建/删除条件变量
  - 等待/通知操作
- 🔲 待补充
  - 超时等待测试
  - 广播通知测试
  - 虚假唤醒处理
  - 与互斥锁配合使用场景
  - 多等待者场景

#### 2.1.4 原子操作 (osal_atomic)

**已有测试**: `tests/unit/osal/test_osal_atomic.c`

**测试用例**:
- ✅ 基础功能
  - 原子加载/存储
  - 原子加/减操作
  - CAS操作
- 🔲 待补充
  - 多线程并发原子操作验证
  - 内存序测试（acquire/release/seq_cst）
  - 性能对比测试（原子 vs 互斥锁）
  - 跨架构验证（ARM/RISC-V弱内存序）

### 2.2 SYS模块 (系统调用封装)

#### 2.2.1 时钟 (osal_clock)

**已有测试**: `tests/unit/osal/test_osal_clock.c`

**测试用例**:
- ✅ 基础功能
  - 获取单调时钟
  - 获取实时时钟
- 🔲 待补充
  - 时钟精度验证
  - 时钟回退检测（单调性）
  - 不同时钟源对比
  - 性能测试（获取时钟开销）

#### 2.2.2 时间 (osal_time)

**已有测试**: `tests/unit/osal/test_osal_time.c`

**测试用例**:
- ✅ 基础功能
  - 延迟函数
  - 时间转换
- 🔲 待补充
  - 延迟精度测试
  - 长时间延迟测试
  - 时区转换测试
  - 闰秒处理

#### 2.2.3 文件操作 (osal_file)

**已有测试**: `tests/unit/osal/test_osal_file.c`

**测试用例**:
- ✅ 基础功能
  - 打开/关闭文件
  - 读写操作
- 🔲 待补充
  - 大文件操作（>2GB）
  - 并发读写测试
  - 文件锁测试
  - 错误恢复（磁盘满、权限不足）
  - 符号链接处理
  - 目录操作测试

#### 2.2.4 信号处理 (osal_signal)

**已有测试**: `tests/unit/osal/test_osal_signal.c`

**测试用例**:
- ✅ 基础功能
  - 信号注册/注销
  - 信号发送/接收
- 🔲 待补充
  - 信号屏蔽测试
  - 嵌套信号处理
  - 实时信号测试
  - 信号与线程交互

#### 2.2.5 进程管理 (osal_process)

**测试文件**: 🔲 待创建 `tests/unit/osal/test_osal_process.c`

**测试用例**:
- 🔲 基础功能
  - 进程创建/销毁
  - 进程等待
  - 获取进程ID
- 🔲 高级功能
  - 进程退出码获取
  - 子进程资源清理
  - 僵尸进程处理

#### 2.2.6 线程管理 (osal_thread)

**测试文件**: 🔲 待创建 `tests/unit/osal/test_osal_thread.c`

**测试用例**:
- 🔲 基础功能
  - 线程创建/销毁
  - 线程join/detach
  - 线程ID获取
- 🔲 高级功能
  - 线程属性设置（栈大小、优先级）
  - 线程局部存储（TLS）
  - 线程取消机制
  - 线程资源清理

#### 2.2.7 环境变量 (osal_env)

**已有测试**: `tests/unit/osal/test_osal_env.c`

**测试用例**:
- ✅ 基础功能
  - 获取/设置环境变量
- 🔲 待补充
  - 环境变量删除
  - 特殊字符处理
  - 大值环境变量
  - 线程安全性验证

#### 2.2.8 IO多路复用 (osal_select)

**测试文件**: 🔲 待创建 `tests/unit/osal/test_osal_select.c`

**测试用例**:
- 🔲 基础功能
  - select/poll封装
  - 超时处理
  - 多文件描述符监控
- 🔲 高级功能
  - 边缘触发 vs 水平触发
  - 大量fd性能测试
  - 信号中断处理

### 2.3 NET模块 (网络相关)

#### 2.3.1 套接字 (osal_socket)

**测试文件**: 🔲 待创建 `tests/unit/osal/test_osal_socket.c`

**测试用例**:
- 🔲 基础功能
  - socket创建/关闭
  - bind/listen/accept
  - connect/send/recv
- 🔲 TCP测试
  - 客户端-服务器通信
  - 大数据传输
  - 连接超时
  - 半关闭状态
- 🔲 UDP测试
  - 数据报发送/接收
  - 广播/组播
- 🔲 错误处理
  - 连接拒绝
  - 网络不可达
  - 缓冲区满

#### 2.3.2 串口配置 (osal_termios)

**测试文件**: 🔲 待创建 `tests/unit/osal/test_osal_termios.c`

**测试用例**:
- 🔲 基础功能
  - 波特率设置
  - 数据位/停止位/校验位配置
  - 流控制设置
- 🔲 高级功能
  - 非阻塞模式
  - 原始模式 vs 规范模式
  - 特殊字符处理

### 2.4 LIB模块 (标准库封装)

#### 2.4.1 字符串操作 (osal_string)

**已有测试**: `tests/unit/osal/test_osal_string.c`

**测试用例**:
- ✅ 基础功能
  - 字符串复制/连接
  - 字符串比较
  - 字符串查找
- 🔲 待补充
  - 边界测试（空字符串、超长字符串）
  - UTF-8字符串处理
  - 性能对比测试

#### 2.4.2 标准输入输出 (osal_stdio)

**测试文件**: 🔲 待创建 `tests/unit/osal/test_osal_stdio.c`

**测试用例**:
- 🔲 基础功能
  - printf/scanf封装
  - 格式化输出
  - 缓冲区管理
- 🔲 高级功能
  - 重定向测试
  - 大数据输出
  - 线程安全性

#### 2.4.3 堆内存管理 (osal_heap)

**已有测试**: `tests/unit/osal/test_osal_heap.c`

**测试用例**:
- ✅ 基础功能
  - 内存分配/释放
  - 内存对齐
- 🔲 待补充
  - 内存泄漏检测
  - 大块内存分配
  - 内存碎片测试
  - OOM处理
  - 性能测试（分配/释放速度）

#### 2.4.4 错误码 (osal_errno)

**已有测试**: `tests/unit/osal/test_osal_errno.c`

**测试用例**:
- ✅ 基础功能
  - 错误码获取/设置
  - 错误信息转换
- 🔲 待补充
  - 线程局部错误码
  - 错误码映射完整性

### 2.5 UTIL模块 (工具类)

#### 2.5.1 日志系统 (osal_log)

**已有测试**: `tests/unit/osal/test_osal_log.c`

**测试用例**:
- ✅ 基础功能
  - 日志级别设置
  - 日志输出
- 🔲 待补充
  - 日志文件轮转
  - 多线程并发日志
  - 日志过滤
  - 性能测试（日志吞吐量）

#### 2.5.2 版本信息 (osal_version)

**已有测试**: `tests/unit/osal/test_osal_version.c`

**测试用例**:
- ✅ 基础功能
  - 版本号获取
  - 版本字符串格式

### 2.6 压力测试

**已有测试**: `tests/unit/osal/test_osal_stress.c`

**测试场景**:
- ✅ 资源耗尽测试
- 🔲 待补充
  - 长时间运行稳定性（24小时+）
  - 高并发场景（1000+线程）
  - 内存压力测试
  - CPU密集型场景

## 3. HAL层测试计划

### 3.1 CAN驱动 (hal_can)

**已有测试**: `tests/unit/hal/test_hal_can.c`

**测试用例**:
- ✅ 基础功能
  - 初始化/去初始化
  - 发送/接收CAN帧
  - 过滤器设置
  - 统计信息获取
- 🔲 待补充
  - 扩展帧测试（29位ID）
  - 错误帧处理
  - 总线关闭/恢复
  - 环回模式测试
  - 高负载测试（满带宽）
  - 多CAN接口并发
  - 错误注入测试

### 3.2 串口驱动 (hal_serial)

**已有测试**: `tests/unit/hal/test_hal_serial.c`

**测试用例**:
- ✅ 基础功能
  - 初始化/去初始化
  - 发送/接收数据
  - 波特率配置
- 🔲 待补充
  - 奇偶校验测试
  - 流控制测试（RTS/CTS）
  - 大数据传输
  - 超时处理
  - 多串口并发
  - 错误恢复（帧错误、溢出）

### 3.3 I2C驱动 (hal_i2c)

**已有测试**: `tests/unit/hal/test_hal_i2c.c`

**测试用例**:
- ✅ 基础功能
  - 初始化/去初始化
  - 读写操作
  - 地址扫描
- 🔲 待补充
  - 10位地址模式
  - 重复起始条件
  - 时钟拉伸
  - 多主机仲裁
  - 总线恢复
  - DMA传输（如果支持）

### 3.4 SPI驱动 (hal_spi)

**已有测试**: `tests/unit/hal/test_hal_spi.c`

**测试用例**:
- ✅ 基础功能
  - 初始化/去初始化
  - 传输操作
  - 模式配置
- 🔲 待补充
  - 不同SPI模式测试（0-3）
  - 片选控制
  - 全双工传输
  - 大数据传输
  - 多SPI设备切换
  - DMA传输（如果支持）

### 3.5 硬件集成测试

**测试文件**: 🔲 待创建 `tests/integration/hal/`

**测试场景**:
- 🔲 CAN环回测试（物理环回）
- 🔲 串口对接测试（TX-RX短接）
- 🔲 I2C真实设备测试（EEPROM等）
- 🔲 SPI真实设备测试（Flash等）
- 🔲 多总线并发测试

## 4. 测试基础设施

### 4.1 测试框架增强

**当前状态**: 使用自研libtest框架

**待改进**:
- 🔲 测试报告生成（HTML/XML格式）
- 🔲 代码覆盖率集成（gcov/lcov）
- 🔲 性能基准数据库
- 🔲 CI/CD集成
- 🔲 测试失败自动分析

### 4.2 Mock和Stub

**需求**:
- 🔲 硬件Mock框架（模拟CAN/UART/I2C/SPI设备）
- 🔲 系统调用Stub（注入错误）
- 🔲 时间Mock（加速时间流逝）

### 4.3 测试工具

**需要开发**:
- 🔲 CAN总线模拟器
- 🔲 串口数据生成器
- 🔲 I2C/SPI协议分析器
- 🔲 性能分析工具

## 5. 测试执行策略

### 5.1 测试分层

**L0 - 单元测试**:
- 每个API独立测试
- 快速执行（<1秒/模块）
- 无硬件依赖

**L1 - 集成测试**:
- 模块间交互测试
- 中等执行时间（<10秒/场景）
- 可选硬件依赖

**L2 - 系统测试**:
- 端到端场景测试
- 长时间执行（分钟级）
- 需要真实硬件

**L3 - 压力测试**:
- 极限场景测试
- 长时间执行（小时级）
- 需要真实硬件

### 5.2 测试环境

**虚拟环境**:
- vcan0（虚拟CAN）
- pty（虚拟串口）
- loopback（网络）

**真实硬件**:
- TI AM6254开发板
- CAN分析仪
- 逻辑分析仪

### 5.3 自动化测试

**触发条件**:
- 每次代码提交（L0单元测试）
- 每日构建（L0+L1）
- 每周构建（L0+L1+L2）
- 发布前（全量测试）

**测试矩阵**:
```
架构 × 编译器 × 优化级别
- x86_64 × GCC × Debug/Release
- ARM32 × GCC × Debug/Release
- ARM64 × GCC × Debug/Release
- RISC-V64 × GCC × Debug/Release
```

## 6. 测试优先级

### P0 - 关键路径（必须100%通过）

**OSAL**:
- 互斥锁、信号量、条件变量
- 线程创建/销毁
- 内存分配/释放
- 时钟/时间

**HAL**:
- CAN初始化/发送/接收
- 串口初始化/发送/接收

### P1 - 核心功能（必须≥95%通过）

**OSAL**:
- 原子操作
- 文件操作
- 套接字
- 日志系统

**HAL**:
- I2C读写
- SPI传输
- 错误处理

### P2 - 增强功能（必须≥90%通过）

**OSAL**:
- 信号处理
- 进程管理
- 环境变量

**HAL**:
- 高级配置
- 统计信息
- 性能优化

### P3 - 边界场景（必须≥80%通过）

- 资源耗尽
- 异常输入
- 并发竞争
- 长时间运行

## 7. 测试指标

### 7.1 质量指标

| 指标 | 目标 | 当前 |
|------|------|------|
| API覆盖率 | 100% | ~60% |
| 代码覆盖率 | ≥85% | 待测量 |
| 分支覆盖率 | ≥80% | 待测量 |
| 测试用例数 | ≥500 | ~150 |
| P0通过率 | 100% | 待测量 |
| P1通过率 | ≥95% | 待测量 |

### 7.2 性能指标

| 操作 | 目标延迟 | 当前 |
|------|----------|------|
| 互斥锁加锁 | <1μs | 待测量 |
| 信号量P操作 | <2μs | 待测量 |
| 线程创建 | <100μs | 待测量 |
| CAN发送 | <50μs | 待测量 |
| 串口发送 | <10μs/字节 | 待测量 |

### 7.3 稳定性指标

| 场景 | 目标 | 当前 |
|------|------|------|
| 24小时压力测试 | 0崩溃 | 待测试 |
| 内存泄漏 | 0字节/小时 | 待测试 |
| 资源泄漏 | 0句柄/小时 | 待测试 |

## 8. 测试交付物

### 8.1 测试代码

- [ ] 完整的单元测试套件
- [ ] 集成测试套件
- [ ] 压力测试套件
- [ ] 性能基准测试

### 8.2 测试文档

- [ ] 测试用例设计文档
- [ ] 测试执行报告
- [ ] 缺陷跟踪报告
- [ ] 性能基准报告

### 8.3 测试工具

- [ ] 自动化测试脚本
- [ ] 硬件模拟工具
- [ ] 性能分析工具
- [ ] 覆盖率报告工具

## 9. 风险和缓解

### 9.1 硬件依赖风险

**风险**: 部分测试需要真实硬件，CI环境无法执行

**缓解**:
- 开发硬件模拟器
- 使用条件编译跳过硬件测试
- 建立专用硬件测试环境

### 9.2 测试时间风险

**风险**: 全量测试耗时过长

**缓解**:
- 测试分层执行
- 并行测试执行
- 增量测试策略

### 9.3 平台兼容性风险

**风险**: 跨平台测试覆盖不足

**缓解**:
- 使用QEMU模拟器
- 建立多架构CI环境
- 定期在真实硬件验证

## 10. 实施计划

### Phase 1: 基础完善（2周）

- [ ] 补充OSAL缺失的测试模块（thread/process/socket/stdio）
- [ ] 完善现有测试的边界条件
- [ ] 建立代码覆盖率基线

### Phase 2: HAL增强（2周）

- [ ] 补充HAL各驱动的高级功能测试
- [ ] 开发硬件模拟框架
- [ ] 建立硬件集成测试环境

### Phase 3: 性能和压力（1周）

- [ ] 开发性能基准测试
- [ ] 实施长时间压力测试
- [ ] 建立性能回归检测

### Phase 4: 自动化和CI（1周）

- [ ] 集成到CI/CD流程
- [ ] 自动化测试报告生成
- [ ] 建立测试失败告警机制

## 11. 详细测试步骤

### 11.1 环境准备

#### 11.1.1 开发环境搭建

**步骤1: 安装基础工具**
```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install -y build-essential cmake git

# 安装交叉编译工具链
sudo apt-get install -y gcc-arm-linux-gnueabihf \
                        gcc-aarch64-linux-gnu \
                        gcc-riscv64-linux-gnu
```

**步骤2: 安装测试工具**
```bash
# 代码覆盖率工具
sudo apt-get install -y lcov gcovr

# 内存检测工具
sudo apt-get install -y valgrind

# 性能分析工具
sudo apt-get install -y linux-tools-generic perf
```

**步骤3: 克隆代码仓库**
```bash
git clone <repository-url> EMS
cd EMS
```

#### 11.1.2 虚拟硬件环境搭建

**步骤1: 配置虚拟CAN接口**
```bash
# 加载CAN内核模块
sudo modprobe can can_raw vcan

# 创建虚拟CAN接口
sudo ip link add dev vcan0 type vcan
sudo ip link set vcan0 up

# 验证接口
ip link show vcan0
```

**步骤2: 配置虚拟串口**
```bash
# 使用socat创建虚拟串口对
sudo apt-get install -y socat
socat -d -d pty,raw,echo=0 pty,raw,echo=0

# 或使用tty0tty内核模块
git clone https://github.com/freemed/tty0tty.git
cd tty0tty/module
make
sudo insmod tty0tty.ko
# 创建 /dev/tnt0 和 /dev/tnt1 虚拟串口对
```

**步骤3: 配置权限**
```bash
# 添加用户到dialout组（串口访问）
sudo usermod -a -G dialout $USER

# 添加用户到can组（CAN访问）
sudo groupadd can
sudo usermod -a -G can $USER

# 重新登录使权限生效
```

### 11.2 编译测试程序

#### 11.2.1 本地架构编译

**步骤1: 配置构建（Release模式）**
```bash
cmake --preset release
```

**预期输出**:
```
-- The C compiler identification is GNU 11.4.0
-- Configuring done
-- Generating done
-- Build files have been written to: /home/user/EMS/build/release
```

**步骤2: 编译**
```bash
cmake --build --preset release -j$(nproc)
```

**预期输出**:
```
[ 10%] Building C object osal/CMakeFiles/osal.dir/src/...
[ 20%] Building C object hal/CMakeFiles/hal.dir/src/...
...
[100%] Built target ems-test
```

**步骤3: 验证编译产物**
```bash
ls -lh build/release/bin/
ls -lh build/release/lib/
```

**预期输出**:
```
build/release/bin/
  ems-test       # 主测试程序
  osal-test      # OSAL层测试（符号链接）
  hal-test       # HAL层测试（符号链接）
  sample_app     # 示例应用

build/release/lib/
  libosal.a      # OSAL静态库
  libhal.a       # HAL静态库
  libpcl.a       # PCL静态库
  libpdl.a       # PDL静态库
```

#### 11.2.2 Debug模式编译（用于调试和覆盖率）

**步骤1: 配置Debug构建**
```bash
cmake --preset debug
```

**步骤2: 编译**
```bash
cmake --build --preset debug -j$(nproc)
```

**步骤3: 验证调试符号**
```bash
file build/debug/bin/ems-test
# 预期输出: ELF 64-bit LSB executable, ... not stripped
```

#### 11.2.3 交叉编译

**ARM32编译**:
```bash
# 步骤1: 配置
cmake --preset arm32

# 步骤2: 编译
cmake --build build/arm32 -j$(nproc)

# 步骤3: 验证
file build/arm32/bin/ems-test
# 预期输出: ELF 32-bit LSB executable, ARM, ...
```

**ARM64编译**:
```bash
cmake --preset arm64
cmake --build build/arm64 -j$(nproc)
file build/arm64/bin/ems-test
# 预期输出: ELF 64-bit LSB executable, ARM aarch64, ...
```

**RISC-V64编译**:
```bash
cmake --preset riscv64
cmake --build build/riscv64 -j$(nproc)
file build/riscv64/bin/ems-test
# 预期输出: ELF 64-bit LSB executable, UCB RISC-V, ...
```

### 11.3 执行单元测试

#### 11.3.1 运行所有测试

**步骤1: 执行全量测试**
```bash
./build/release/bin/ems-test -a
```

**预期输出**:
```
========================================
EMS Test Framework v1.0
========================================
Running all tests...

[OSAL] osal_mutex
  ✓ test_mutex_create_success
  ✓ test_mutex_create_nullpointer
  ✓ test_mutex_lockunlock_success
  ...

[HAL] hal_can
  ✓ test_hal_can_init_success
  ⊘ test_hal_can_send_recv (skipped: vcan0 not available)
  ...

========================================
Test Summary
========================================
Total:   150 tests
Passed:  145 tests
Failed:  0 tests
Skipped: 5 tests
Time:    2.345s
========================================
```

**步骤2: 检查退出码**
```bash
echo $?
# 预期输出: 0 (所有测试通过)
```

#### 11.3.2 运行特定层测试

**OSAL层测试**:
```bash
# 方法1: 使用符号链接
./build/release/bin/osal-test -a

# 方法2: 使用层级过滤
./build/release/bin/ems-test -L OSAL
```

**HAL层测试**:
```bash
./build/release/bin/hal-test -a
# 或
./build/release/bin/ems-test -L HAL
```

#### 11.3.3 运行特定模块测试

**步骤1: 列出所有模块**
```bash
./build/release/bin/ems-test -l
```

**预期输出**:
```
Available test modules:
  [OSAL] osal_mutex          - OSAL互斥锁测试
  [OSAL] osal_semaphore      - OSAL信号量测试
  [OSAL] osal_thread         - OSAL线程测试
  [HAL]  hal_can             - HAL CAN驱动测试
  ...
```

**步骤2: 运行指定模块**
```bash
./build/release/bin/ems-test -m osal_mutex
```

**预期输出**:
```
[OSAL] osal_mutex
  ✓ test_mutex_create_success (0.001s)
  ✓ test_mutex_create_nullpointer (0.000s)
  ✓ test_mutex_lockunlock_success (0.001s)
  ✓ test_mutex_lock_nullpointer (0.000s)
  ✓ test_mutex_unlock_nullpointer (0.000s)
  ✓ test_mutex_delete_success (0.001s)
  ✓ test_mutex_delete_nullpointer (0.000s)
  ✓ test_mutex_protect_shared_resource (0.015s)

Module: osal_mutex - 8/8 passed (0.019s)
```

#### 11.3.4 交互式测试

**步骤1: 启动交互模式**
```bash
./build/release/bin/ems-test -i
```

**步骤2: 选择测试**
```
========================================
EMS Test Framework - Interactive Mode
========================================

Select test layer:
  1. OSAL
  2. HAL
  3. PCL
  4. PDL
  5. All layers
  0. Exit

Your choice: 1

Select OSAL module:
  1. osal_mutex
  2. osal_semaphore
  3. osal_thread
  ...
  0. Back

Your choice: 1

Running [OSAL] osal_mutex...
```

### 11.4 代码覆盖率测试

#### 11.4.1 生成覆盖率报告

**步骤1: 使用Debug构建并启用覆盖率**
```bash
# 清理之前的构建
rm -rf build/debug

# 配置Debug构建（自动启用gcov）
cmake --preset debug

# 编译
cmake --build --preset debug -j$(nproc)
```

**步骤2: 运行测试生成覆盖率数据**
```bash
# 清理旧的覆盖率数据
find build/debug -name "*.gcda" -delete

# 运行测试
./build/debug/bin/ems-test -a
```

**步骤3: 生成HTML报告（使用lcov）**
```bash
# 捕获覆盖率数据
lcov --capture \
     --directory build/debug \
     --output-file build/debug/coverage.info \
     --rc lcov_branch_coverage=1

# 过滤系统头文件和测试代码
lcov --remove build/debug/coverage.info \
     '/usr/*' \
     '*/tests/*' \
     --output-file build/debug/coverage_filtered.info \
     --rc lcov_branch_coverage=1

# 生成HTML报告
genhtml build/debug/coverage_filtered.info \
        --output-directory build/debug/coverage_html \
        --branch-coverage \
        --title "EMS Coverage Report"

# 查看报告
xdg-open build/debug/coverage_html/index.html
```

**步骤4: 生成文本摘要**
```bash
lcov --list build/debug/coverage_filtered.info
```

**预期输出**:
```
                                      |Lines       |Functions  |Branches
Filename                              |Rate     Num|Rate    Num|Rate     Num
================================================================================
osal/src/ipc/osal_mutex.c             |88.5%    104|100.0%    8|82.3%     34
osal/src/ipc/osal_semaphore.c         |85.2%     81|100.0%    6|78.9%     28
hal/src/linux/hal_can.c               |82.1%    156|100.0%   12|75.4%     52
...
================================================================================
                                Total:|85.3%   2145|98.7%   156|80.1%    678
```

#### 11.4.2 使用gcovr生成报告

**步骤1: 安装gcovr**
```bash
pip3 install gcovr
```

**步骤2: 生成HTML报告**
```bash
gcovr --root . \
      --filter 'osal/.*' \
      --filter 'hal/.*' \
      --exclude 'tests/.*' \
      --html-details build/debug/coverage_gcovr.html \
      --print-summary
```

**步骤3: 生成XML报告（用于CI）**
```bash
gcovr --root . \
      --filter 'osal/.*' \
      --filter 'hal/.*' \
      --exclude 'tests/.*' \
      --xml build/debug/coverage.xml
```

### 11.5 内存泄漏检测

#### 11.5.1 使用Valgrind检测

**步骤1: 运行Valgrind**
```bash
valgrind --leak-check=full \
         --show-leak-kinds=all \
         --track-origins=yes \
         --verbose \
         --log-file=valgrind_report.txt \
         ./build/debug/bin/ems-test -m osal_mutex
```

**步骤2: 查看报告**
```bash
cat valgrind_report.txt
```

**预期输出（无泄漏）**:
```
==12345== HEAP SUMMARY:
==12345==     in use at exit: 0 bytes in 0 blocks
==12345==   total heap usage: 1,234 allocs, 1,234 frees, 45,678 bytes allocated
==12345==
==12345== All heap blocks were freed -- no leaks are possible
==12345==
==12345== ERROR SUMMARY: 0 errors from 0 contexts
```

**步骤3: 检测特定模块**
```bash
# 检测OSAL层所有模块
for module in osal_mutex osal_semaphore osal_thread; do
    echo "Checking $module..."
    valgrind --leak-check=full \
             --log-file=valgrind_${module}.txt \
             ./build/debug/bin/ems-test -m $module
done

# 汇总结果
grep "ERROR SUMMARY" valgrind_*.txt
```

#### 11.5.2 使用AddressSanitizer

**步骤1: 重新编译启用ASan**
```bash
# 配置
cmake --preset debug \
      -DCMAKE_C_FLAGS="-fsanitize=address -fno-omit-frame-pointer"

# 编译
cmake --build --preset debug -j$(nproc)
```

**步骤2: 运行测试**
```bash
ASAN_OPTIONS=detect_leaks=1:symbolize=1 \
./build/debug/bin/ems-test -a
```

**预期输出（无问题）**:
```
========================================
Test Summary
========================================
Total:   150 tests
Passed:  150 tests
Failed:  0 tests

=================================================================
==12345==ERROR: LeakSanitizer: detected memory leaks
(如果有泄漏会显示详细信息)
```

### 11.6 性能基准测试

#### 11.6.1 运行性能测试

**步骤1: 编译Release版本**
```bash
cmake --preset release
cmake --build --preset release -j$(nproc)
```

**步骤2: 运行性能测试模块**
```bash
./build/release/bin/ems-test -m osal_performance
```

**预期输出**:
```
[OSAL] Performance Benchmarks
  ✓ Mutex lock/unlock: 0.234 μs/op (4,273,504 ops/s)
  ✓ Semaphore wait/post: 0.456 μs/op (2,192,982 ops/s)
  ✓ Thread create/join: 45.6 μs/op (21,930 ops/s)
  ✓ Memory alloc/free: 0.123 μs/op (8,130,081 ops/s)
```

**步骤3: 保存基准数据**
```bash
./build/release/bin/ems-test -m osal_performance > baseline_performance.txt
```

**步骤4: 对比性能回归**
```bash
# 修改代码后重新测试
./build/release/bin/ems-test -m osal_performance > current_performance.txt

# 对比
diff baseline_performance.txt current_performance.txt
```

#### 11.6.2 使用perf分析

**步骤1: 运行perf记录**
```bash
sudo perf record -g ./build/release/bin/ems-test -m osal_mutex
```

**步骤2: 查看报告**
```bash
sudo perf report
```

**步骤3: 生成火焰图**
```bash
# 安装FlameGraph工具
git clone https://github.com/brendangregg/FlameGraph.git

# 生成火焰图
sudo perf script | FlameGraph/stackcollapse-perf.pl | \
     FlameGraph/flamegraph.pl > flamegraph.svg

# 查看
xdg-open flamegraph.svg
```

### 11.7 压力测试

#### 11.7.1 运行压力测试

**步骤1: 运行压力测试模块**
```bash
./build/release/bin/ems-test -m osal_stress
```

**预期输出**:
```
[OSAL] Stress Tests
  ✓ High concurrency (1000 threads): PASS (15.234s)
  ✓ Memory stress (10000 allocs): PASS (2.456s)
  ✓ Long running (1 hour): RUNNING...
```

**步骤2: 后台运行长时间测试**
```bash
nohup ./build/release/bin/ems-test -m osal_stress_long > stress_test.log 2>&1 &

# 记录进程ID
echo $! > stress_test.pid

# 监控进程
watch -n 10 'ps aux | grep ems-test'

# 监控资源使用
watch -n 5 'cat /proc/$(cat stress_test.pid)/status | grep -E "VmSize|VmRSS|Threads"'
```

**步骤3: 检查结果**
```bash
# 等待测试完成
wait $(cat stress_test.pid)

# 查看日志
tail -100 stress_test.log

# 检查是否有崩溃
dmesg | grep -i "segfault\|killed"
```

#### 11.7.2 24小时稳定性测试

**步骤1: 准备测试脚本**
```bash
cat > run_24h_test.sh <<'EOF'
#!/bin/bash
START_TIME=$(date +%s)
DURATION=$((24 * 3600))  # 24小时

while true; do
    CURRENT_TIME=$(date +%s)
    ELAPSED=$((CURRENT_TIME - START_TIME))
    
    if [ $ELAPSED -ge $DURATION ]; then
        echo "24-hour test completed"
        break
    fi
    
    echo "[$(date)] Running test iteration..."
    ./build/release/bin/ems-test -a
    
    if [ $? -ne 0 ]; then
        echo "[$(date)] TEST FAILED!"
        exit 1
    fi
    
    sleep 60  # 每分钟运行一次
done
EOF

chmod +x run_24h_test.sh
```

**步骤2: 启动测试**
```bash
nohup ./run_24h_test.sh > 24h_test.log 2>&1 &
echo $! > 24h_test.pid
```

**步骤3: 监控测试**
```bash
# 实时查看日志
tail -f 24h_test.log

# 检查内存泄漏
watch -n 300 'ps aux | grep ems-test | grep -v grep'

# 检查系统资源
vmstat 60 > vmstat_24h.log &
```

### 11.8 跨平台测试

#### 11.8.1 使用QEMU测试ARM

**步骤1: 安装QEMU**
```bash
sudo apt-get install -y qemu-user-static
```

**步骤2: 运行ARM32测试**
```bash
# 编译ARM32版本
cmake --preset arm32
cmake --build build/arm32 -j$(nproc)

# 使用QEMU运行
qemu-arm-static -L /usr/arm-linux-gnueabihf \
                build/arm32/bin/ems-test -a
```

**步骤3: 运行ARM64测试**
```bash
cmake --preset arm64
cmake --build build/arm64 -j$(nproc)

qemu-aarch64-static -L /usr/aarch64-linux-gnu \
                    build/arm64/bin/ems-test -a
```

**步骤4: 运行RISC-V64测试**
```bash
cmake --preset riscv64
cmake --build build/riscv64 -j$(nproc)

qemu-riscv64-static -L /usr/riscv64-linux-gnu \
                    build/riscv64/bin/ems-test -a
```

#### 11.8.2 真实硬件测试

**步骤1: 传输测试程序到目标板**
```bash
# 使用scp传输
scp build/arm64/bin/ems-test root@192.168.1.100:/tmp/

# 或使用NFS挂载
sudo mount -t nfs 192.168.1.100:/export /mnt/target
cp build/arm64/bin/ems-test /mnt/target/
```

**步骤2: 在目标板上运行**
```bash
# SSH登录目标板
ssh root@192.168.1.100

# 运行测试
cd /tmp
chmod +x ems-test
./ems-test -a
```

**步骤3: 收集测试结果**
```bash
# 保存测试日志
./ems-test -a > test_result_arm64.log 2>&1

# 传回主机
scp test_result_arm64.log user@host:/path/to/results/
```

### 11.9 CI/CD集成测试

#### 11.9.1 本地模拟CI流程

**步骤1: 创建CI测试脚本**
```bash
cat > ci_test.sh <<'EOF'
#!/bin/bash
set -e  # 遇到错误立即退出

echo "========================================="
echo "CI Test Pipeline"
echo "========================================="

# 1. 清理环境
echo "[1/6] Cleaning..."
rm -rf build/

# 2. 编译Release版本
echo "[2/6] Building Release..."
cmake --preset release
cmake --build --preset release -j$(nproc)

# 3. 运行单元测试
echo "[3/6] Running unit tests..."
./build/release/bin/ems-test -a

# 4. 编译Debug版本
echo "[4/6] Building Debug..."
cmake --preset debug
cmake --build --preset debug -j$(nproc)

# 5. 生成覆盖率报告
echo "[5/6] Generating coverage..."
./build/debug/bin/ems-test -a
lcov --capture --directory build/debug --output-file coverage.info
lcov --list coverage.info

# 6. 交叉编译验证
echo "[6/6] Cross-compilation check..."
cmake --preset arm64
cmake --build build/arm64 -j$(nproc)

echo "========================================="
echo "CI Test Pipeline: PASSED"
echo "========================================="
EOF

chmod +x ci_test.sh
```

**步骤2: 运行CI测试**
```bash
./ci_test.sh
```

**步骤3: 检查结果**
```bash
echo $?  # 应该返回0
```

### 11.10 测试报告生成

#### 11.10.1 生成JUnit XML报告

**步骤1: 修改测试框架支持XML输出**
```bash
./build/release/bin/ems-test -a --output-xml=test_results.xml
```

**步骤2: 验证XML格式**
```bash
xmllint --noout test_results.xml
echo $?  # 应该返回0
```

#### 11.10.2 生成HTML测试报告

**步骤1: 使用Python脚本生成**
```bash
cat > generate_report.py <<'EOF'
#!/usr/bin/env python3
import xml.etree.ElementTree as ET
import sys

def generate_html_report(xml_file, html_file):
    tree = ET.parse(xml_file)
    root = tree.getroot()
    
    with open(html_file, 'w') as f:
        f.write('<html><head><title>Test Report</title></head><body>')
        f.write('<h1>EMS Test Report</h1>')
        f.write('<table border="1">')
        f.write('<tr><th>Test</th><th>Status</th><th>Time</th></tr>')
        
        for testcase in root.iter('testcase'):
            name = testcase.get('name')
            time = testcase.get('time')
            status = 'PASS' if testcase.find('failure') is None else 'FAIL'
            f.write(f'<tr><td>{name}</td><td>{status}</td><td>{time}s</td></tr>')
        
        f.write('</table></body></html>')

if __name__ == '__main__':
    generate_html_report('test_results.xml', 'test_report.html')
EOF

chmod +x generate_report.py
python3 generate_report.py
```

**步骤2: 查看报告**
```bash
xdg-open test_report.html
```

## 12. 附录

### 12.1 测试命令速查

```bash
# 运行所有测试
./build/release/bin/ems-test -a

# 运行OSAL层测试
./build/release/bin/osal-test -a

# 运行HAL层测试
./build/release/bin/hal-test -a

# 运行特定模块
./build/release/bin/ems-test -m test_osal_mutex

# 交互式选择测试
./build/release/bin/ems-test -i

# 生成覆盖率报告
cmake --preset debug
cmake --build --preset debug
./build/debug/bin/ems-test -a
lcov --capture --directory build/debug --output-file coverage.info
genhtml coverage.info --output-directory coverage_html
```

### 12.2 测试编写规范

#### 12.2.1 测试文件结构

```c
/* 测试文件模板 */
#include "test_framework.h"
#include "osal.h"

/*===========================================================================
 * 测试辅助函数和全局变量
 *===========================================================================*/

static int32_t shared_counter = 0;
static osal_mutex_t *test_mutex = NULL;

/* 辅助函数: 初始化测试环境 */
static void setup_test_env(void)
{
    shared_counter = 0;
    OSAL_MutexCreate(&test_mutex);
}

/* 辅助函数: 清理测试环境 */
static void teardown_test_env(void)
{
    if (test_mutex != NULL) {
        OSAL_MutexDelete(test_mutex);
        test_mutex = NULL;
    }
}

/*===========================================================================
 * 基础功能测试
 *===========================================================================*/

/* 测试用例: 创建成功 */
TEST_CASE(test_module_create_success)
{
    /* 1. 准备测试数据 */
    osal_mutex_t *mutex = NULL;
    
    /* 2. 执行被测函数 */
    int32_t ret = OSAL_MutexCreate(&mutex);
    
    /* 3. 验证结果 */
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_NOT_NULL(mutex);
    
    /* 4. 清理资源 */
    OSAL_MutexDelete(mutex);
}

/* 测试用例: 参数验证 - 空指针 */
TEST_CASE(test_module_create_null_pointer)
{
    int32_t ret = OSAL_MutexCreate(NULL);
    TEST_ASSERT_EQUAL(OSAL_ERR_INVALID_POINTER, ret);
}

/*===========================================================================
 * 边界条件测试
 *===========================================================================*/

/* 测试用例: 边界值测试 */
TEST_CASE(test_module_boundary_conditions)
{
    /* 测试最小值 */
    /* 测试最大值 */
    /* 测试零值 */
}

/*===========================================================================
 * 并发测试
 *===========================================================================*/

/* 线程函数 */
static void* concurrent_test_thread(void *arg)
{
    /* 线程逻辑 */
    return NULL;
}

/* 测试用例: 多线程并发 */
TEST_CASE(test_module_concurrent_access)
{
    setup_test_env();
    
    osal_thread_t threads[10];
    
    /* 创建多个线程 */
    for (int32_t i = 0; i < 10; i++) {
        OSAL_pthread_create(&threads[i], NULL, concurrent_test_thread, NULL);
    }
    
    /* 等待所有线程完成 */
    for (int32_t i = 0; i < 10; i++) {
        OSAL_pthread_join(threads[i], NULL);
    }
    
    /* 验证结果 */
    TEST_ASSERT_EQUAL(expected_value, shared_counter);
    
    teardown_test_env();
}

/*===========================================================================
 * 性能测试
 *===========================================================================*/

/* 测试用例: 性能基准 */
TEST_CASE(test_module_performance)
{
    const int32_t iterations = 100000;
    uint64_t start_time, end_time;
    
    start_time = OSAL_GetTimeNs();
    
    for (int32_t i = 0; i < iterations; i++) {
        /* 执行被测操作 */
    }
    
    end_time = OSAL_GetTimeNs();
    
    double avg_time_us = (end_time - start_time) / (double)iterations / 1000.0;
    
    /* 输出性能数据 */
    TEST_LOG("Average time: %.3f μs/op", avg_time_us);
    
    /* 验证性能要求 */
    TEST_ASSERT_TRUE(avg_time_us < 1.0);  /* 要求小于1微秒 */
}

/*===========================================================================
 * 压力测试
 *===========================================================================*/

/* 测试用例: 资源耗尽 */
TEST_CASE(test_module_resource_exhaustion)
{
    const int32_t max_resources = 1000;
    osal_mutex_t *mutexes[max_resources];
    int32_t created = 0;
    
    /* 尝试创建大量资源 */
    for (int32_t i = 0; i < max_resources; i++) {
        int32_t ret = OSAL_MutexCreate(&mutexes[i]);
        if (ret != OSAL_SUCCESS) {
            break;
        }
        created++;
    }
    
    TEST_LOG("Created %d resources", created);
    
    /* 清理资源 */
    for (int32_t i = 0; i < created; i++) {
        OSAL_MutexDelete(mutexes[i]);
    }
    
    /* 验证至少能创建一定数量 */
    TEST_ASSERT_TRUE(created >= 100);
}

/*===========================================================================
 * 测试模块注册
 *===========================================================================*/

TEST_MODULE_BEGIN(test_module_name, "OSAL")
    /* 基础功能测试 */
    TEST_CASE_REF(test_module_create_success)
    TEST_CASE_REF(test_module_create_null_pointer)
    
    /* 边界条件测试 */
    TEST_CASE_REF(test_module_boundary_conditions)
    
    /* 并发测试 */
    TEST_CASE_REF(test_module_concurrent_access)
    
    /* 性能测试 */
    TEST_CASE_REF(test_module_performance)
    
    /* 压力测试 */
    TEST_CASE_REF(test_module_resource_exhaustion)
TEST_MODULE_END(test_module_name, "OSAL")
```

#### 12.2.2 测试断言宏

```c
/* 相等性断言 */
TEST_ASSERT_EQUAL(expected, actual)           // 相等
TEST_ASSERT_NOT_EQUAL(expected, actual)       // 不相等

/* 布尔断言 */
TEST_ASSERT_TRUE(condition)                   // 为真
TEST_ASSERT_FALSE(condition)                  // 为假

/* 指针断言 */
TEST_ASSERT_NULL(pointer)                     // 空指针
TEST_ASSERT_NOT_NULL(pointer)                 // 非空指针

/* 字符串断言 */
TEST_ASSERT_STR_EQUAL(expected, actual)       // 字符串相等
TEST_ASSERT_STR_NOT_EQUAL(expected, actual)   // 字符串不相等

/* 内存断言 */
TEST_ASSERT_MEM_EQUAL(expected, actual, size) // 内存内容相等

/* 范围断言 */
TEST_ASSERT_IN_RANGE(value, min, max)         // 在范围内
TEST_ASSERT_GREATER_THAN(value, threshold)    // 大于
TEST_ASSERT_LESS_THAN(value, threshold)       // 小于

/* 跳过测试 */
TEST_SKIP_IF(condition, "reason")             // 条件满足时跳过

/* 日志输出 */
TEST_LOG("format", ...)                       // 输出测试日志
```

#### 12.2.3 测试命名规范

**测试文件命名**:
```
tests/unit/<layer>/test_<module>.c
```

**测试用例命名**:
```c
test_<module>_<function>_<scenario>

示例:
test_mutex_create_success          // 成功场景
test_mutex_create_null_pointer     // 空指针场景
test_mutex_lock_timeout            // 超时场景
test_mutex_concurrent_access       // 并发场景
```

**测试模块命名**:
```c
TEST_MODULE_BEGIN(<module_name>, "<layer>")

示例:
TEST_MODULE_BEGIN(osal_mutex, "OSAL")
TEST_MODULE_BEGIN(hal_can, "HAL")
```

#### 12.2.4 测试覆盖清单

每个模块测试应包含以下类型:

**1. 功能测试**
- [ ] 正常流程测试
- [ ] 所有公共API测试
- [ ] 返回值验证

**2. 参数验证测试**
- [ ] 空指针测试
- [ ] 无效参数测试
- [ ] 边界值测试

**3. 错误处理测试**
- [ ] 错误码验证
- [ ] 异常恢复测试
- [ ] 资源清理验证

**4. 并发测试**
- [ ] 多线程竞争测试
- [ ] 线程安全验证
- [ ] 死锁检测

**5. 性能测试**
- [ ] 操作延迟测试
- [ ] 吞吐量测试
- [ ] 性能回归检测

**6. 压力测试**
- [ ] 资源耗尽测试
- [ ] 长时间运行测试
- [ ] 高负载测试

### 12.3 常见问题排查

#### 12.3.1 编译错误

**问题1: 找不到头文件**
```
error: osal.h: No such file or directory
```

**解决方案**:
```bash
# 检查include路径
cmake --preset debug -DCMAKE_VERBOSE_MAKEFILE=ON
cmake --build --preset debug

# 或手动指定
cmake --preset debug -DCMAKE_C_FLAGS="-I/path/to/include"
```

**问题2: 链接错误**
```
undefined reference to `OSAL_MutexCreate'
```

**解决方案**:
```bash
# 检查库是否生成
ls -l build/debug/lib/libosal.a

# 检查链接顺序
cmake --build --preset debug -- VERBOSE=1
```

#### 12.3.2 运行时错误

**问题1: 段错误**
```
Segmentation fault (core dumped)
```

**解决方案**:
```bash
# 使用gdb调试
gdb ./build/debug/bin/ems-test
(gdb) run -m test_osal_mutex
(gdb) bt  # 查看堆栈

# 或使用core dump
ulimit -c unlimited
./build/debug/bin/ems-test -m test_osal_mutex
gdb ./build/debug/bin/ems-test core
```

**问题2: 测试超时**
```
Test timeout after 30 seconds
```

**解决方案**:
```bash
# 增加超时时间
./build/debug/bin/ems-test -m test_osal_mutex --timeout=60

# 或使用gdb单步调试
gdb --args ./build/debug/bin/ems-test -m test_osal_mutex
```

#### 12.3.3 硬件相关问题

**问题1: CAN接口不可用**
```
[ERROR] Failed to open CAN interface: vcan0
```

**解决方案**:
```bash
# 检查接口是否存在
ip link show vcan0

# 创建虚拟CAN接口
sudo modprobe vcan
sudo ip link add dev vcan0 type vcan
sudo ip link set vcan0 up
```

**问题2: 串口权限不足**
```
[ERROR] Permission denied: /dev/ttyS0
```

**解决方案**:
```bash
# 添加用户到dialout组
sudo usermod -a -G dialout $USER

# 或临时使用sudo
sudo ./build/release/bin/ems-test -m test_hal_serial

# 或修改设备权限
sudo chmod 666 /dev/ttyS0
```

### 12.4 参考资料

- [测试框架文档](../tests/README.md)
- [OSAL API文档](../osal/README.md)
- [HAL API文档](../hal/README.md)
- [编码规范](CODING_STANDARDS.md)
- [架构设计](ARCHITECTURE.md)

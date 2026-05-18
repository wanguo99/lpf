# PMC系统实现总结

## 完成时间
2024年（基于EMS v1.0架构）

## 项目概述

基于EMS框架实现了PMC (Payload Management Controller) 载荷管理控制器，采用5进程架构设计，实现了完整的遥控遥测、数据采集、健康监测和进程管理功能。

## 架构设计

### 5进程架构

```
┌─────────────────────────────────────────────────────────┐
│                    pmc_supervisor                        │
│              (进程监督和生命周期管理)                      │
└─────────────────────────────────────────────────────────┘
                          │
        ┌─────────────────┼─────────────────┐
        │                 │                 │
┌───────▼──────┐  ┌──────▼───────┐  ┌─────▼────────┐
│  pmc_comm    │  │pmc_collector │  │  pmc_health  │
│  (遥控遥测)   │  │  (数据采集)   │  │  (健康监测)   │
└──────────────┘  └──────────────┘  └──────────────┘
        │                 │                 │
        └─────────────────┼─────────────────┘
                          │
                  ┌───────▼────────┐
                  │   pmc_logger   │
                  │   (日志管理)    │
                  └────────────────┘
```

### 进程间通信

- 使用OSAL共享内存接口（`osal_shm.h`）
- libpmc提供IPC辅助函数封装
- 心跳机制确保进程健康状态

## 已实现的功能

### 1. libpmc公共库 (`libs/libpmc/`)

#### 文件结构
```
libs/libpmc/
├── include/
│   ├── libpmc_ipc.h          # IPC辅助函数接口
│   └── libpmc_protocol.h     # 协议解析接口
├── src/
│   ├── libpmc_ipc.c          # IPC实现
│   └── libpmc_protocol.c     # 协议解析实现
└── CMakeLists.txt            # 构建配置
```

#### 核心功能
- **共享内存管理**：封装OSAL共享内存接口
- **遥控遥测数据结构**：定义标准数据格式
- **协议解析**：遥控命令解析和遥测数据封装
- **心跳机制**：进程健康状态监控
- **日志接口**：统一日志收集

### 2. Communication进程 (`apps/pmc_comm/`)

#### 功能
- 通过CAN总线接收卫星遥控命令
- 解析遥控协议帧
- 将遥控数据写入共享内存
- 从共享内存读取遥测数据
- 封装遥测协议帧并发送

#### 关键实现
- 双任务架构：遥控接收任务 + 遥测发送任务
- 使用HAL_CAN接口进行CAN通信
- 调用libpmc协议解析函数
- 定期更新心跳

### 3. Collector进程 (`apps/pmc_collector/`)

#### 功能
- 采集BMC设备状态（温度、电压、电流）
- 采集MCU设备状态
- 采集FPGA设备状态
- 将采集数据写入共享内存

#### 关键实现
- 使用PDL接口访问各类设备
- 定期采集（100ms周期）
- 数据聚合和格式化
- 心跳更新

### 4. Health进程 (`apps/pmc_health/`)

#### 功能
- 监控MCU心跳
- 监控FPGA心跳
- 监控CPLD心跳
- 故障检测和告警

#### 关键实现
- 使用PDL接口读取设备心跳
- 心跳超时检测（2秒超时）
- 故障日志记录
- 自身心跳更新

### 5. Supervisor进程 (`apps/pmc_supervisor/`)

#### 功能
- 启动和监控其他4个进程
- 检测进程崩溃
- 自动重启崩溃的进程
- 处理系统关闭信号
- 优雅停止所有进程

#### 关键实现
- 使用fork/exec启动子进程
- 监控子进程心跳（通过共享内存）
- SIGCHLD信号处理
- 进程重启计数和限制
- 优雅关闭流程

### 6. Logger进程 (`apps/pmc_logger/`)

#### 功能
- 从共享内存收集日志
- 写入日志文件
- 日志文件轮转（10MB限制）
- 按日期组织日志文件

#### 关键实现
- 日志收集任务（从共享内存读取）
- 文件写入和刷新
- 自动轮转机制
- 日志格式化（时间戳、级别、模块、消息）

## 构建系统

### 目录结构
```
EMS/
├── libs/
│   └── libpmc/              # PMC公共库
│       ├── include/
│       ├── src/
│       └── CMakeLists.txt
├── apps/
│   ├── pmc_comm/            # Communication进程
│   ├── pmc_collector/       # Collector进程
│   ├── pmc_health/          # Health进程
│   ├── pmc_supervisor/      # Supervisor进程
│   ├── pmc_logger/          # Logger进程
│   ├── CMakeLists.txt       # Apps构建配置
│   └── README.md            # Apps文档
├── scripts/
│   ├── pmc_control.sh       # 进程控制脚本
│   └── pmc.service          # systemd服务配置
└── CMakeLists.txt           # 根构建配置
```

### 依赖关系
```
pmc_comm ──┐
pmc_collector ──┤
pmc_health ──├──> libpmc ──> osal
pmc_supervisor ──┤
pmc_logger ──┘
```

### 编译命令
```bash
# 完整编译
mkdir -p build && cd build
cmake ..
make -j$(nproc)

# 输出位置
build/bin/pmc_comm
build/bin/pmc_collector
build/bin/pmc_health
build/bin/pmc_supervisor
build/bin/pmc_logger
```

## 运行和部署

### 开发环境

#### 使用控制脚本
```bash
# 启动所有进程
./scripts/pmc_control.sh start

# 查看状态
./scripts/pmc_control.sh status

# 停止所有进程
./scripts/pmc_control.sh stop

# 重启
./scripts/pmc_control.sh restart
```

#### 手动启动（调试）
```bash
# 按顺序启动
./build/bin/pmc_logger &
./build/bin/pmc_comm &
./build/bin/pmc_collector &
./build/bin/pmc_health &
./build/bin/pmc_supervisor &
```

### 生产环境

#### systemd服务
```bash
# 安装
sudo cp scripts/pmc.service /etc/systemd/system/
sudo cp scripts/pmc_control.sh /usr/local/bin/
sudo cp build/bin/pmc_* /usr/local/bin/

# 启动
sudo systemctl daemon-reload
sudo systemctl enable pmc
sudo systemctl start pmc

# 管理
sudo systemctl status pmc
sudo systemctl restart pmc
sudo systemctl stop pmc
```

## 日志系统

### 日志位置
- 系统日志：`/var/log/pmc/pmc_YYYYMMDD.log`
- systemd日志：`journalctl -u pmc`

### 日志格式
```
[2024-01-15 10:30:45] [INFO ] [COMM] 接收到遥控命令: cmd_id=0x01
[2024-01-15 10:30:45] [DEBUG] [COLLECTOR] 采集BMC数据: temp=45.2°C
[2024-01-15 10:30:46] [WARN ] [HEALTH] MCU心跳超时
[2024-01-15 10:30:46] [ERROR] [SUPERVISOR] 进程崩溃: pmc_health
```

### 日志轮转
- 单个文件最大：10MB
- 按日期分割：每天一个文件
- 自动轮转：达到大小限制时自动创建新文件

## 设计原则

### 1. 架构分层
- **OSAL层**：提供底层IPC接口（共享内存、消息队列）
- **libpmc层**：封装业务相关的IPC辅助函数和协议解析
- **Apps层**：独立进程，通过libpmc进行通信

### 2. 进程独立性
- 每个进程完全独立，有独立的main函数
- 不共享代码文件，通过链接libpmc库获得公共功能
- 进程间仅通过IPC通信，无直接依赖

### 3. 故障隔离
- 单个进程崩溃不影响其他进程
- Supervisor进程自动检测和重启崩溃进程
- 心跳机制确保进程健康状态

### 4. 可维护性
- 清晰的目录结构
- 统一的日志系统
- 完善的构建系统
- 详细的文档

## 技术亮点

1. **跨平台设计**：完全基于OSAL抽象层，支持Linux/macOS
2. **模块化架构**：进程独立，职责清晰
3. **健壮性**：进程监督、自动重启、心跳检测
4. **可观测性**：统一日志收集、日志轮转
5. **易部署**：提供控制脚本和systemd服务配置

## 测试建议

### 单元测试
- libpmc协议解析函数测试
- IPC辅助函数测试
- 心跳机制测试

### 集成测试
- 进程间通信测试
- 遥控遥测端到端测试
- 进程崩溃恢复测试

### 压力测试
- 高频遥控命令处理
- 大量遥测数据采集
- 长时间运行稳定性

## 后续优化方向

1. **性能优化**
   - 使用无锁队列替代共享内存
   - 优化协议解析性能
   - 减少内存拷贝

2. **功能增强**
   - 支持配置文件热加载
   - 增加性能监控指标
   - 支持远程管理接口

3. **可靠性提升**
   - 增加进程重启次数限制
   - 实现看门狗机制
   - 增加数据校验

4. **可观测性**
   - 集成Prometheus监控
   - 增加性能指标采集
   - 支持分布式追踪

## 参考文档

- [EMS整体架构](../docs/architecture/0-EMS_Architecture_Refactor_v1.0.md)
- [应用层设计](../docs/architecture/1-6-app_design.md)
- [OSAL接口文档](../osal/README.md)
- [Apps使用指南](../apps/README.md)

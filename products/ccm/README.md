# CCM Product Build Guide

## 产品架构

CCM 产品线包含多个型号，每个型号启用不同的应用组合。

### 产品型号

| 型号 | 应用组合 | 说明 |
|------|---------|------|
| H200-100P V1 | collector + comm | 基础型号 |
| H200-100P V2 | collector + comm + health | 增强型号 |
| H200-200P | 全部应用 | 旗舰型号 |

### 应用说明

- **collector**: 数据采集应用
- **comm**: 通信应用
- **health**: 健康监控应用
- **logger**: 日志管理应用
- **supervisor**: 监控管理应用

## 构建步骤

### 1. 选择产品配置

```bash
cd /home/wanguo/EMS/products/ccm_product

# 加载 H200-100P V1 配置
cp configs/h200_100p_v1_defconfig .config.mk

# 或加载 H200-100P V2 配置
cp configs/h200_100p_v2_defconfig .config.mk

# 或加载 H200-200P 配置
cp configs/h200_200p_defconfig .config.mk
```

### 2. 自定义配置（可选）

```bash
# 使用 menuconfig 图形界面调整配置
python3 project.py menuconfig
```

### 3. 编译

```bash
# 编译所有启用的应用
python3 project.py build

# 编译结果在 build/ 目录
ls -lh build/collector build/comm build/health build/logger build/supervisor
```

### 4. 清理

```bash
# 清理构建产物
python3 project.py clean

# 完全清理（包括配置）
python3 project.py distclean
```

## 目录结构

```
ccm_product/
├── CMakeLists.txt          # 产品构建配置
├── project.py              # 构建脚本
├── config_defaults.mk      # 默认配置
├── .config.mk              # 当前配置（从 configs/ 复制）
│
├── configs/                # 产品型号配置
│   ├── h200_100p_v1_defconfig
│   ├── h200_100p_v2_defconfig
│   └── h200_200p_defconfig
│
├── components/             # 产品组件（库）
│   ├── libccm/            # CCM 通用库
│   └── h200_100p_am625/   # H200 平台库
│
└── apps/                   # 应用组件
    ├── app_collector/
    ├── app_comm/
    ├── app_health/
    ├── app_logger/
    └── app_supervisor/
```

## 添加新产品型号

1. 创建新的 defconfig 文件：
   ```bash
   cp configs/h200_200p_defconfig configs/new_product_defconfig
   ```

2. 编辑配置，启用/禁用应用：
   ```
   CONFIG_APP_COLLECTOR=y
   CONFIG_APP_COMM=y
   CONFIG_APP_HEALTH=n
   ...
   ```

3. 使用新配置：
   ```bash
   cp configs/new_product_defconfig .config.mk
   python3 project.py build
   ```

## 注意事项

- 所有产品型号共享相同的 SDK 组件和产品组件
- 只有应用组件可以通过配置启用/禁用
- 修改 `.config.mk` 后需要重新编译
- 不要直接修改 `config_defaults.mk`，它是默认配置模板

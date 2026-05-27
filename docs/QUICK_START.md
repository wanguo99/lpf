# EMS 配置和构建快速参考

## 📋 可用配置

| 配置文件 | 平台 | 用途 | 优化 | 库类型 |
|---------|------|------|------|--------|
| `x86_64_full_defconfig` | x86_64 | 开发调试 | -O2 | 静态+动态 |
| `x86_64_minimal_defconfig` | x86_64 | 生产部署 | -Os | 仅静态 |
| `x86_64_test_defconfig` | x86_64 | 自动化测试 | -O2 | 静态+动态 |
| `arm64_full_defconfig` | ARM64 | 开发调试 | -O0 | 动态 |
| `arm64_minimal_defconfig` | ARM64 | 生产部署 | -Os | 仅静态 |
| `arm64_test_defconfig` | ARM64 | 自动化测试 | -O0 | 静态+动态 |

## 🚀 快速开始

### 场景 1：x86_64 本地开发

```bash
# 1. 加载配置
make x86_64_full_defconfig

# 2. 生成构建文件
cmake -B build-cmake

# 3. 编译
cmake --build build-cmake -j$(nproc)

# 4. 运行
./build-cmake/bin/ccm_collector
```

### 场景 2：ARM64 交叉编译（生产）

```bash
# 1. 安装交叉编译工具链
sudo apt-get install gcc-aarch64-linux-gnu

# 2. 加载最小化配置
make arm64_minimal_defconfig

# 3. 生成构建文件
cmake -B build-cmake

# 4. 编译
cmake --build build-cmake -j$(nproc)

# 5. 部署到目标设备
scp build-cmake/bin/* root@target:/usr/local/bin/
scp build-cmake/lib/*.a root@target:/usr/local/lib/
```

### 场景 3：自定义配置

```bash
# 1. 从基础配置开始
make x86_64_full_defconfig

# 2. 交互式修改（图形界面）
make menuconfig
# 进入菜单，启用/禁用需要的功能

# 3. 保存自定义配置
make savedefconfig
cp defconfig configs/my_custom_defconfig

# 4. 构建
cmake -B build-cmake
cmake --build build-cmake -j$(nproc)
```

## 🔧 常用命令

### 配置管理

```bash
# 查看可用配置
ls configs/*_defconfig

# 加载指定配置
make <config_name>_defconfig

# 交互式配置（menuconfig）
make menuconfig

# 保存当前配置
make savedefconfig

# 清理配置
make clean
```

### 构建管理

```bash
# 生成构建文件
cmake -B build-cmake

# 编译（并行）
cmake --build build-cmake -j$(nproc)

# 清理构建产物
rm -rf build-cmake

# 安装
cmake --install build-cmake --prefix /usr/local
```

### 查看配置

```bash
# 查看当前配置
cat .config

# 查看特定选项
grep CONFIG_HAL .config

# 查看 CMake 变量
cmake -B build-cmake -LAH | grep CONFIG_
```

## 📊 配置对比

### 功能启用对比

| 功能 | Full | Minimal | Test |
|------|------|---------|------|
| OSAL 全部功能 | ✅ | ❌ | ✅ |
| HAL 全部驱动 | ✅ | ⚠️ 部分 | ✅ |
| PCL | ✅ | ❌ | ✅ |
| PDL | ✅ | ❌ | ✅ |
| ACL | ✅ | ❌ | ✅ |
| 测试框架 | ❌ | ❌ | ✅ |
| 调试日志 | ✅ | ❌ | ✅ |

### 二进制大小对比（估算）

| 平台 | Full | Minimal | 节省 |
|------|------|---------|------|
| x86_64 | ~2MB | ~500KB | 75% |
| ARM64 | ~1.5MB | ~400KB | 73% |

## 🎯 选择指南

### 根据使用场景选择

- **本地开发调试** → `x86_64_full_defconfig`
- **嵌入式生产部署** → `arm64_minimal_defconfig`
- **CI/CD 自动化测试** → `x86_64_test_defconfig`
- **交叉编译开发** → `arm64_full_defconfig`

### 根据优化目标选择

- **最快编译速度** → Full（-O2）
- **最小二进制大小** → Minimal（-Os）
- **最佳调试体验** → Full（-O0 for ARM64）
- **最真实性能测试** → Test（-O2）

## 💡 高级技巧

### 1. 快速切换配置

```bash
# 创建配置切换脚本
cat > switch_config.sh << 'EOF'
#!/bin/bash
make clean
make $1_defconfig
rm -rf build-cmake
cmake -B build-cmake
cmake --build build-cmake -j$(nproc)
EOF

chmod +x switch_config.sh

# 使用
./switch_config.sh x86_64_full
./switch_config.sh arm64_minimal
```

### 2. 查看配置差异

```bash
# 比较两个配置
diff configs/x86_64_full_defconfig configs/x86_64_minimal_defconfig
```

### 3. 验证配置

```bash
# 加载配置后验证
make x86_64_full_defconfig
grep "CONFIG_OSAL=y" .config && echo "✅ OSAL enabled" || echo "❌ OSAL disabled"
grep "CONFIG_HAL=y" .config && echo "✅ HAL enabled" || echo "❌ HAL disabled"
```

### 4. 批量测试所有配置

```bash
# 测试所有配置是否能编译
for config in configs/*_defconfig; do
    echo "Testing $(basename $config)..."
    make clean
    make $(basename $config)
    cmake -B build-cmake
    cmake --build build-cmake -j$(nproc) || echo "❌ Failed: $config"
done
```

## 🐛 故障排除

### 问题：配置不生效

```bash
# 解决方案：清理并重新配置
make clean
rm -rf build-cmake .config
make x86_64_full_defconfig
cmake -B build-cmake
```

### 问题：找不到交叉编译工具链

```bash
# 解决方案：安装工具链
sudo apt-get install gcc-aarch64-linux-gnu

# 验证
aarch64-linux-gnu-gcc --version
```

### 问题：编译错误

```bash
# 解决方案：检查配置
cat .config | grep CONFIG_HAL
cat .config | grep CONFIG_OSAL

# 重新加载配置
make x86_64_full_defconfig
cmake -B build-cmake
```

## 📚 相关文档

- [configs/README.md](../configs/README.md) - 详细配置说明
- [docs/CMAKE_BUILD_GUIDE.md](../docs/CMAKE_BUILD_GUIDE.md) - CMake 构建指南
- [CLAUDE.md](../CLAUDE.md) - 项目完整指南

---

**提示**：首次使用建议从 `x86_64_full_defconfig` 开始，熟悉后再尝试其他配置。

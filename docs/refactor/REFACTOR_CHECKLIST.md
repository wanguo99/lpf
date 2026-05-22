# EMS 构建系统重构检查清单

## 分支信息
- **分支**: `feature/refactor-build-system`
- **基于**: v1.0.0 (commit 2e6083a)
- **创建日期**: 2026-05-22

---

## Phase 1: 准备工作 ✅

- [x] 创建特性分支
- [x] 编写重构计划文档
- [ ] 创建测试基准
- [ ] 备份当前构建脚本

**测试基准命令**:
```bash
# 记录当前构建时间
make mrproper
time make ccm_h200_am625_debug_defconfig
time make -j$(nproc)

# 记录构建产物
find .staging -type f > /tmp/ems-before-refactor.txt
md5sum .staging/bin/* .staging/lib/*.a > /tmp/ems-before-md5.txt
```

---

## Phase 2: 创建 Makefile.compiler ✅

### 文件创建
- [x] `scripts/Makefile.compiler` (193 行)
- [x] `scripts/gcc-version.sh`
- [x] `scripts/ld-version.sh`

### 功能实现
- [x] 编译器类型检测 (`cc-name`)
- [x] 编译器版本检测 (`cc-version`, `cc-fullversion`)
- [x] 链接器版本检测 (`ld-version`)
- [x] 编译器选项测试 (`cc-option`, `cc-option-yn`)
- [x] 链接器选项测试 (`ld-option`)
- [x] AR 选项测试 (`ar-option`)
- [x] LLVM 工具链支持

### 集成
- [x] 从 `Kbuild.include` 移除重复函数
- [x] 在 `Makefile.build` 中包含 `Makefile.compiler`
- [x] 更新顶层 Makefile（如需要）

### 测试
- [x] 基本编译测试: `make core/osal/`
- [x] LLVM 测试: `make LLVM=1 core/osal/`
- [ ] 交叉编译测试: `make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- core/osal/`
- [x] 编译器选项测试: 验证 `cc-option` 工作正常

**完成日期**: 2026-05-22
**提交**: 已提交

---

## Phase 3: 创建 Makefile.lib.build ✅

### 文件创建
- [x] `scripts/Makefile.lib.build` (143 行)

### 功能实现
- [x] 静态库构建规则 (`lib-y`)
- [x] 动态库构建规则 (`so-y`)
- [x] 头文件安装规则 (`header-y`)
- [x] 库名智能处理

### 集成
- [x] 从 `Makefile.build` 移除库构建代码 (255-297, 299-354, 471-505 行)
- [x] 在 `Makefile.build` 中包含 `Makefile.lib.build`

### 测试
- [x] 静态库构建: `make core/osal/ && ls .staging/lib/libosal.a`
- [x] 动态库构建: 启用 `CONFIG_OSAL_BUILD_SHARED` 后测试
- [x] 头文件安装: `find .staging/include -name "osal*.h"`
- [x] 库名处理: 验证 `lib-y += osal` → `libosal.a`

**完成日期**: 2026-05-23
**提交**: 已提交
**行数变化**: Makefile.build 从 524 行减少到 390 行 (-134行)

---

## Phase 4: 创建 Makefile.app.build ⏳

### 文件创建
- [ ] `scripts/Makefile.app.build` (~80 行)

### 功能实现
- [ ] 应用程序构建规则 (`app-y`)
- [ ] 应用程序链接规则
- [ ] 应用程序安装规则

### 集成
- [ ] 从 `Makefile.build` 移除应用构建代码 (356-395 行)
- [ ] 在 `Makefile.build` 中包含 `Makefile.app.build`

### 测试
- [ ] 应用构建: `make products/ccm/apps/ccm_collector/`
- [ ] 应用安装: `ls .staging/bin/ccm_collector`
- [ ] 链接测试: `ldd .staging/bin/ccm_collector`
- [ ] RPATH 测试: `readelf -d .staging/bin/ccm_collector | grep RPATH`

---

## Phase 5: 创建 Makefile.mod.build ⏳

### 文件创建
- [ ] `scripts/Makefile.mod.build` (~100 行)

### 功能实现
- [ ] 内核模块构建规则 (`obj-m`)
- [ ] 模块链接规则
- [ ] 模块安装规则

### 集成
- [ ] 从 `Makefile.build` 移除模块构建代码 (397-427 行)
- [ ] 在 `Makefile.build` 中包含 `Makefile.mod.build`

### 测试
- [ ] 完整构建: `make clean && make -j$(nproc)`
- [ ] 验证不影响现有流程

---

## Phase 6: 重构 Makefile.build ⏳

### 代码重构
- [ ] 移除已拆分的代码
- [ ] 保留核心编译规则
- [ ] 添加包含语句
- [ ] 优化代码结构
- [ ] 完善注释

### 目标
- [ ] 行数减少到 ~250 行
- [ ] 职责清晰
- [ ] 代码优雅

### 测试
- [ ] 完整构建: `make mrproper && make ccm_h200_am625_debug_defconfig && make -j$(nproc)`
- [ ] 增量构建: 修改文件后 `make`
- [ ] 外部构建: `make O=/tmp/ems-build`

---

## Phase 7: 测试和验证 ⏳

### 功能测试
- [ ] 完整构建
- [ ] 增量构建
- [ ] 外部构建
- [ ] 清理测试
- [ ] 配置测试
- [ ] 安装测试
- [ ] 交叉编译
- [ ] LLVM 编译

### 产物验证
- [ ] 静态库正确生成
- [ ] 动态库正确生成
- [ ] 应用程序正确生成
- [ ] 头文件正确安装

### 性能测试
- [ ] 记录重构后构建时间
- [ ] 对比重构前后差异
- [ ] 确保性能无明显下降 (±5%)

### 回归测试
- [ ] 对比构建产物
- [ ] 对比二进制文件 MD5
- [ ] 验证功能一致性

---

## Phase 8: 文档更新 ⏳

### 文档更新
- [ ] `docs/BUILD_SYSTEM.md` - 添加新文件说明
- [ ] `docs/BUILD_GUIDE.md` - 更新构建流程
- [ ] `CLAUDE.md` - 更新构建系统章节
- [ ] `docs/refactor/BUILD_SYSTEM_REFACTOR_SUMMARY.md` - 创建重构总结

### 内容检查
- [ ] 架构图准确
- [ ] 示例代码可运行
- [ ] 说明完整清晰

---

## Phase 9: 代码审查和合并 ⏳

### 审查
- [ ] 代码风格检查
- [ ] 注释完整性检查
- [ ] 错误处理检查

### 合并准备
- [ ] 创建 Pull Request
- [ ] 编写 PR 描述
- [ ] 附上测试结果

### 合并
- [ ] 解决冲突
- [ ] 更新 CHANGELOG
- [ ] 合并到 master
- [ ] 创建 tag v1.1.0

---

## 验收标准

### 功能标准
- [ ] 所有现有功能正常工作
- [ ] 构建产物与重构前一致
- [ ] 构建时间无明显变化 (±5%)
- [ ] 支持所有现有构建选项

### 质量标准
- [ ] 代码结构清晰
- [ ] 注释完整
- [ ] 符合编码规范
- [ ] 文档完整

### 可维护性标准
- [ ] 职责单一
- [ ] 易于扩展
- [ ] 易于调试

---

## 快速命令参考

### 测试命令
```bash
# 完整构建
make mrproper
make ccm_h200_am625_debug_defconfig
make -j$(nproc)

# 增量构建
touch core/osal/src/posix/lib/osal_thread.c
make core/osal/

# 外部构建
make O=/tmp/ems-build ccm_h200_am625_debug_defconfig
make O=/tmp/ems-build -j$(nproc)

# 清理
make clean
make mrproper

# 性能测试
time make -j$(nproc)

# 产物对比
diff -r /tmp/before/.staging /tmp/after/.staging
md5sum .staging/bin/* .staging/lib/*
```

### 调试命令
```bash
# 详细输出
make V=1 core/osal/

# 查看变量
make -p | grep "^cc-name"

# 检查依赖
cat core/osal/.osal_thread.o.cmd
```

---

## 问题跟踪

### 已知问题
- 无

### 待解决问题
- 无

### 风险项
- 性能下降风险 - 需要密切监控
- 兼容性风险 - 需要充分测试

---

**检查清单版本**: v1.0  
**最后更新**: 2026-05-22  
**当前阶段**: Phase 1 (准备工作)

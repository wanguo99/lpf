# OSAL 重构路线图

## 目标
将 OSAL 从高层封装改为薄封装层，遵循 OSAL_xxx 命名规范（xxx 为小写）

## 已完成工作（本次会话）

### 1. 命名规范化
- ✅ **util 模块**（26 API）：版本、日志 API 全部小写化
- ✅ **lib 模块**（18 API）：errno、stdio、heap、flock API 小写化
- ✅ **sys 模块**（15 API）：时间、进程相关 API 小写化

### 2. 架构重构
- ✅ **sys/Process 模块**：删除 6 个高层 API，添加 4 个 POSIX 薄封装
  - 删除：ProcessCreate/Kill/Wait/Exists/GetId/GetParentId
  - 添加：getppid, execvp, fork, waitpid
  - 更新 supervisor 代码使用标准 POSIX 接口

### 3. 提交记录
```
fce656d - refactor: rename OSAL_Printf to OSAL_printf
214ae12 - refactor(osal/util): rename util module APIs to lowercase
2c70e86 - refactor(osal/lib): rename lib module APIs to lowercase
5bbf10a - refactor(osal/sys): rename sys module APIs to lowercase (part 1)
4858508 - refactor(osal/sys): remove Process high-level API, use POSIX directly
```

## 下一步工作

### 阶段 2：IPC 模块重构（最大工作量）

#### 优先级 1：简单模块（先易后难）
1. **Cond 模块**（42 uses）
   - 改为 pthread_cond_* 薄封装
   - API: init, destroy, wait, timedwait, signal, broadcast

2. **Rwlock 模块**（32 uses）
   - 改为 pthread_rwlock_* 薄封装
   - API: init, destroy, rdlock, wrlock, tryrdlock, trywrlock, unlock

3. **Semaphore 模块**（51 uses）
   - 改为 sem_* 薄封装
   - API: init, destroy, wait, trywait, timedwait, post

#### 优先级 2：核心模块
4. **Mutex 模块**（366 uses）⚠️ 最多使用
   - 改为 pthread_mutex_* 薄封装
   - API: init, destroy, lock, trylock, unlock
   - 考虑：是否保留 MutexAttr* API（设置类型、协议等）

5. **Thread 模块**（120 uses）
   - 合并 sys/osal_thread.h 和 ipc 相关
   - 改为 pthread_* 薄封装
   - API: create, join, detach, self, exit
   - ThreadAttr: init, destroy, set* (栈大小、调度等)

6. **Atomic 模块**（220 uses）
   - 评估是否完全移除，直接用 C11 _Atomic 或 GCC __sync_*
   - 或者提供极薄的封装用于跨平台

#### 优先级 3：特殊模块
7. **Shm 模块**（80 uses）
   - 改为 shm_open/mmap/munmap 薄封装
   - API: open, close, map, unmap, unlink

8. **Shm_cache 模块**（17 uses）
   - 评估是否为应用层逻辑
   - 可能移到 HAL 或 PDL 层，或完全删除

### 阶段 3：sys 模块剩余部分

9. **Signal 模块**（35 uses）
   - 改为 sigaction/sigprocmask/sigwait 薄封装
   - 删除：SignalRegister/Block/Unblock/Ignore/Default
   - 添加：sigaction, sigprocmask, sigwait, signal

10. **Sched 模块**（60 uses）
    - 改为 sched_* 和 pthread_setschedparam 薄封装
    - 删除：SchedGetPolicy/SetPolicy/GetPriority/SetPriority 等
    - 添加：sched_getscheduler, sched_setscheduler, sched_getparam 等

### 阶段 4：验证和测试
11. 更新所有测试用例
12. 验证 CCM 应用功能
13. 性能基准测试
14. 文档更新

## 预计工作量

| 阶段 | 模块数 | API 数 | 使用次数 | 预计时间 |
|------|--------|--------|----------|----------|
| 已完成 | 3 | 59 | ~200 | ✅ 完成 |
| 阶段 2 (IPC) | 6 | ~60 | ~900 | 15-20 小时 |
| 阶段 3 (sys) | 2 | ~20 | ~95 | 3-5 小时 |
| 阶段 4 (验证) | - | - | - | 2-3 小时 |
| **总计** | **11** | **~139** | **~1195** | **20-28 小时** |

## 技术决策

### Mutex/Thread/Semaphore 重构方案
```c
// 旧方案（高层封装，隐藏 pthread 类型）
osal_mutex_t *mutex;
OSAL_MutexCreate(&mutex);
OSAL_MutexLock(mutex);
OSAL_MutexUnlock(mutex);
OSAL_MutexDelete(mutex);

// 新方案（薄封装，直接暴露 pthread 类型）
pthread_mutex_t mutex;
OSAL_pthread_mutex_init(&mutex, NULL);
OSAL_pthread_mutex_lock(&mutex);
OSAL_pthread_mutex_unlock(&mutex);
OSAL_pthread_mutex_destroy(&mutex);
```

**优点**：
- 符合 POSIX 标准，易于移植
- 代码更清晰，减少抽象层
- 性能更好（减少间接调用和内存分配）

**缺点**：
- API 变化较大，需要大量代码修改
- pthread 类型暴露给用户代码

### Atomic 重构方案选择

**方案 A：完全移除 OSAL_Atomic，使用 C11 _Atomic**
```c
// 旧代码
OSAL_AtomicLoad(&counter);

// 新代码
_Atomic int counter;
atomic_load(&counter);
```

**方案 B：保留极薄封装，兼容非 C11 编译器**
```c
#ifdef __STDC_VERSION__ >= 201112L
  #define OSAL_atomic_load(ptr) atomic_load(ptr)
#else
  #define OSAL_atomic_load(ptr) __sync_fetch_and_add(ptr, 0)
#endif
```

**推荐**：方案 B，提供最小封装以保证跨平台兼容性

## 执行建议

1. **分批次进行**：每次处理 1-2 个模块，完成后验证编译并提交
2. **优先测试代码**：先更新测试用例，确保新 API 正确
3. **保持向后兼容**：可以暂时保留旧 API 作为 deprecated wrapper
4. **文档先行**：每个模块重构前，先写清楚新旧 API 映射关系

## 风险评估

| 风险 | 严重性 | 缓解措施 |
|------|--------|----------|
| 大量代码需要修改 | 高 | 使用脚本批量替换，分阶段验证 |
| 测试覆盖率不足 | 中 | 补充单元测试，运行集成测试 |
| API 语义变化 | 中 | 仔细审查每个 API 的行为差异 |
| 性能回退 | 低 | 运行性能基准测试 |

## 后续行动

下次会话从 **Semaphore 模块**开始，理由：
1. 使用量适中（51 uses）
2. API 简单，容易理解
3. 可以作为其他 IPC 模块的模板

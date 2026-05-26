# EMS 编码规范

## C 语言规范

### 标准

- **语言标准**: ISO C90 (ANSI C)
- **编码风格**: Linux Kernel Coding Style
- **命名规范**: snake_case（小写加下划线）

### 基本规则

#### 1. 缩进和格式

```c
/* 使用 Tab 缩进（8 空格宽度） */
void function_name(void)
{
	int a;
	int b;
	
	if (condition) {
		do_something();
	} else {
		do_other();
	}
}
```

#### 2. 变量声明

```c
/* ✓ 正确：所有声明在代码块开头 */
void func(void)
{
	int a = 1;
	int b = 2;
	char *str;
	
	printf("%d\n", a);
}

/* ✗ 错误：混合声明和代码（C99 特性） */
void func(void)
{
	int a = 1;
	printf("%d\n", a);
	int b = 2;  /* 错误 */
}
```

#### 3. 循环变量

```c
/* ✓ 正确：在循环外声明 */
int i;
for (i = 0; i < 10; i++) {
	/* ... */
}

/* ✗ 错误：在 for 循环中声明（C99 特性） */
for (int i = 0; i < 10; i++) {  /* 错误 */
	/* ... */
}
```

#### 4. 命名规范

```c
/* 函数名：小写加下划线 */
int osal_thread_create(void);
void hal_can_send_message(void);

/* 变量名：小写加下划线 */
int thread_count;
char *device_name;

/* 宏定义：大写加下划线 */
#define MAX_BUFFER_SIZE 1024
#define OSAL_SUCCESS 0

/* 结构体：小写加下划线 */
struct osal_thread {
	int id;
	char name[32];
};

/* 类型定义：小写加下划线 + _t */
typedef struct osal_thread osal_thread_t;
```

### 注释规范

#### 1. 文件头注释

```c
/**
 * @file osal_thread.c
 * @brief OSAL 线程管理实现
 * @author Your Name
 * @date 2026-05-26
 */
```

#### 2. 函数注释

```c
/**
 * @brief 创建线程
 * @param name 线程名称
 * @param entry 线程入口函数
 * @param arg 线程参数
 * @return 成功返回线程 ID，失败返回负数错误码
 */
int osal_thread_create(const char *name, void *(*entry)(void *), void *arg);
```

#### 3. 代码注释

```c
/* 单行注释使用 C 风格 */
int count = 0;  /* 计数器 */

/*
 * 多行注释使用这种格式
 * 每行开头对齐星号
 */
```

### 错误处理

```c
/* ✓ 正确：统一的错误码 */
int osal_thread_create(void)
{
	if (invalid_param) {
		return -EINVAL;
	}
	
	if (no_memory) {
		return -ENOMEM;
	}
	
	return 0;  /* 成功 */
}

/* ✓ 正确：检查返回值 */
int ret;

ret = osal_thread_create();
if (ret < 0) {
	fprintf(stderr, "Failed to create thread: %d\n", ret);
	return ret;
}
```

### 内存管理

```c
/* ✓ 正确：检查 malloc 返回值 */
char *buf = malloc(size);
if (buf == NULL) {
	return -ENOMEM;
}

/* 使用后释放 */
free(buf);
buf = NULL;  /* 避免悬空指针 */

/* ✗ 错误：不检查返回值 */
char *buf = malloc(size);
strcpy(buf, str);  /* 可能崩溃 */
```

### 头文件保护

```c
/* osal_thread.h */
#ifndef OSAL_THREAD_H
#define OSAL_THREAD_H

/* 声明 */

#endif /* OSAL_THREAD_H */
```

## Makefile 规范

### module.mk 文件

```makefile
# 模块名
MODULE_NAME := osal

# 源文件列表
osal_SRCS := \
	core/osal/src/posix/lib/osal_errno.c \
	core/osal/src/posix/lib/osal_heap.c \
	core/osal/src/posix/sys/osal_thread.c

# 编译标志
osal_CFLAGS := \
	-Iinclude/osal \
	-Iinclude/hal

# 链接标志
osal_LDFLAGS := \
	-lpthread \
	-lrt

# 目标文件
osal_OBJS := $(call srcs_to_objs,$(osal_SRCS))

# 构建目标
osal_TARGET := $(STAGING_DIR)/lib/libosal.so
ALL_TARGETS += $(osal_TARGET)

# 构建规则
$(eval $(call build_shared_lib,$(osal_TARGET),$(osal_OBJS),$(osal_LDFLAGS)))
```

## Git 提交规范

### 提交消息格式

```
<type>(<scope>): <subject>

<body>

<footer>
```

### 类型（type）

- `feat`: 新功能
- `fix`: 修复 bug
- `docs`: 文档更新
- `style`: 代码格式（不影响功能）
- `refactor`: 重构
- `perf`: 性能优化
- `test`: 测试相关
- `chore`: 构建/工具链相关

### 示例

```
feat(osal): 添加线程优先级设置接口

- 新增 osal_thread_set_priority() 函数
- 支持 POSIX 和 RTOS 平台
- 添加单元测试

Closes #123
```

## 代码审查清单

- [ ] 符合 C90 标准（无 C99/C11 特性）
- [ ] 遵循 Linux Kernel 编码风格
- [ ] 变量声明在代码块开头
- [ ] 检查所有返回值
- [ ] 释放所有分配的内存
- [ ] 添加必要的注释
- [ ] 通过编译（无警告）
- [ ] 通过单元测试
- [ ] 更新相关文档

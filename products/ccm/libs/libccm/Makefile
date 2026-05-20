# ============================================================================
# libccm 库构建 - Kbuild 框架
# ============================================================================

# 定义库目标
obj-s := libccm.a
obj-d := libccm.so

# 源文件目录
subdir-y += src/

# 编译标志
CFLAGS_LIB := -I$(srctree)/products/ccm/libs/libccm/include
CFLAGS_LIB += -I$(srctree)/core/osal/include
CFLAGS_LIB += -I$(srctree)/core/hal/include
CFLAGS_LIB += -I$(srctree)/include

# 链接库
LDLIBS := -L$(objtree)/lib -losal

# 包含 Kbuild 构建框架
include $(scriptdir)/build/Makefile.build


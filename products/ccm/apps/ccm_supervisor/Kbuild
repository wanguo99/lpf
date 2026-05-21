# =============================================================================
# CCM Supervisor 应用构建配置
# =============================================================================

# -----------------------------------------------------------------------------
# 1. 可执行程序目标
# -----------------------------------------------------------------------------
app-y += ccm_supervisor

# -----------------------------------------------------------------------------
# 2. 源文件列表
# -----------------------------------------------------------------------------
ccm_supervisor-objs := \
	src/main.o \
	src/ccm_supervisor.o

# -----------------------------------------------------------------------------
# 3. 编译标志
# -----------------------------------------------------------------------------
ccflags-y += -I$(src)/include
ccflags-y += -I$(srctree)/core/osal/include
ccflags-y += -I$(srctree)/core/hal/include
ccflags-y += -I$(srctree)/core/pcl/include
ccflags-y += -I$(srctree)/core/pcl/include/api
ccflags-y += -I$(srctree)/core/pdl/include
ccflags-y += -I$(srctree)/core/acl/include
ccflags-y += -I$(srctree)/products/ccm/libs/libccm/include
ccflags-y += -I$(srctree)/include

# -----------------------------------------------------------------------------
# 4. 链接标志
# -----------------------------------------------------------------------------
ccm_supervisor-ldflags := -L$(objtree)/lib
ccm_supervisor-ldflags += -Wl,-rpath,$$ORIGIN/../lib
ccm_supervisor-ldflags += -lh200_am625 -lccm
ccm_supervisor-ldflags += -lacl -lpdl -lpcl -lhal -losal

# -----------------------------------------------------------------------------
# 5. 清理规则
# -----------------------------------------------------------------------------
clean-files := ccm_supervisor

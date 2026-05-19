# ============================================================================
# PDL 模块构建
# ============================================================================

# 必须在包含 Makefile.lib 之前定义
GENERATED_DIR ?= $(objtree)/include/generated

include $(srctree)/scripts/Makefile.lib

# 模块名称
MODULE := pdl
LIB_NAME := libpdl.so

# 输出目录
BUILD_DIR ?= $(objtree)/core/$(MODULE)
LIB_DIR := $(objtree)/lib

# 源文件目录
SRC_DIR := $(srctree)/core/$(MODULE)/src
INC_DIR := $(srctree)/core/$(MODULE)/include

# 读取 Kconfig 配置
AUTOCONF := $(GENERATED_DIR)/autoconf.h

# 根据 Kconfig 条件编译驱动
SRCS :=

# PDL MCU 驱动
ifneq ($(shell grep -q "^\#define CONFIG_PDL_MCU_SUPPORT 1" $(AUTOCONF) 2>/dev/null && echo y),)
  SRCS += pdl_mcu/pdl_mcu.c
  SRCS += pdl_mcu/pdl_mcu_can.c
  SRCS += pdl_mcu/pdl_mcu_protocol.c
  SRCS += pdl_mcu/pdl_mcu_serial.c
endif

# PDL Satellite 驱动
ifneq ($(shell grep -q "^\#define CONFIG_PDL_SATELLITE_SUPPORT 1" $(AUTOCONF) 2>/dev/null && echo y),)
  SRCS += pdl_satellite/pdl_satellite.c
  SRCS += pdl_satellite/pdl_satellite_can.c
endif

# PDL BMC 驱动
ifneq ($(shell grep -q "^\#define CONFIG_PDL_BMC_SUPPORT 1" $(AUTOCONF) 2>/dev/null && echo y),)
  SRCS += pdl_bmc/pdl_bmc.c
  SRCS += pdl_bmc/pdl_bmc_transport.c
  SRCS += pdl_bmc/pdl_bmc_ipmi.c
  SRCS += pdl_bmc/pdl_bmc_redfish.c
endif

# PDL Watchdog 驱动
ifneq ($(shell grep -q "^\#define CONFIG_PDL_WATCHDOG_SUPPORT 1" $(AUTOCONF) 2>/dev/null && echo y),)
  SRCS += pdl_watchdog.c
endif

# 目标文件
OBJS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRCS))

# 编译标志
CFLAGS_MODULE := -I$(INC_DIR)
CFLAGS_MODULE += -I$(srctree)/core/osal/include
CFLAGS_MODULE += -I$(srctree)/core/hal/include
CFLAGS_MODULE += -I$(srctree)/core/pcl/include

# 链接库
LDLIBS := -L$(LIB_DIR) -losal -lhal -lpcl

# 默认目标
.PHONY: all clean

all: $(LIB_DIR)/$(LIB_NAME)
	@echo "PDL module built successfully"

# 编译规则
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(call cmd,cc_o_c)

# 链接规则
$(LIB_DIR)/$(LIB_NAME): $(OBJS) | $(LIB_DIR)
	$(call cmd,ld_so)

# 创建输出目录
$(LIB_DIR):
	@mkdir -p $@

# 清理
# 包含依赖文件（自动跟踪头文件变化）
-include $(OBJS:.o=.d)

clean:
	@rm -rf $(BUILD_DIR)
	@rm -f $(LIB_DIR)/$(LIB_NAME)

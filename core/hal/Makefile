# ============================================================================
# HAL 模块构建
# ============================================================================

# 必须在包含 Makefile.lib 之前定义
GENERATED_DIR ?= $(objtree)/include/generated

include $(srctree)/scripts/Makefile.lib

# 模块名称
MODULE := hal
LIB_NAME := libhal.so

# 输出目录
BUILD_DIR ?= $(objtree)/core/$(MODULE)
LIB_DIR := $(objtree)/lib

# 源文件目录
SRC_DIR := $(srctree)/core/$(MODULE)/src/linux
INC_DIR := $(srctree)/core/$(MODULE)/include

# 读取 Kconfig 配置
AUTOCONF := $(GENERATED_DIR)/autoconf.h

# 根据 Kconfig 条件编译驱动
SRCS :=

# 检查配置并添加源文件
ifneq ($(shell grep -q "^\#define CONFIG_HAL_CAN 1" $(AUTOCONF) 2>/dev/null && echo y),)
  SRCS += hal_can_linux.c
endif

ifneq ($(shell grep -q "^\#define CONFIG_HAL_UART 1" $(AUTOCONF) 2>/dev/null && echo y),)
  SRCS += hal_serial_linux.c
endif

ifneq ($(shell grep -q "^\#define CONFIG_HAL_I2C 1" $(AUTOCONF) 2>/dev/null && echo y),)
  SRCS += hal_i2c_linux.c
endif

ifneq ($(shell grep -q "^\#define CONFIG_HAL_SPI 1" $(AUTOCONF) 2>/dev/null && echo y),)
  SRCS += hal_spi_linux.c
endif

ifneq ($(shell grep -q "^\#define CONFIG_HAL_GPIO 1" $(AUTOCONF) 2>/dev/null && echo y),)
  SRCS += hal_gpio_linux.c
endif

ifneq ($(shell grep -q "^\#define CONFIG_HAL_WATCHDOG 1" $(AUTOCONF) 2>/dev/null && echo y),)
  SRCS += hal_watchdog_linux.c
endif

# 目标文件
OBJS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRCS))

# 编译标志
CFLAGS_MODULE := -I$(INC_DIR)
CFLAGS_MODULE += -I$(srctree)/core/osal/include

# 链接库
LDLIBS := -L$(LIB_DIR) -losal

# 默认目标
.PHONY: all clean

all: $(LIB_DIR)/$(LIB_NAME)
	@echo "HAL module built successfully"

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

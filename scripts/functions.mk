# =============================================================================
# 辅助函数
# =============================================================================

# 函数：将源文件路径转换为目标文件路径
# 用法：$(call src_to_obj,src/file.c)
# 结果：build/src/file.o
src_to_obj = $(patsubst %.c,$(BUILD_DIR)/%.o,$(1))

# 函数：将源文件列表转换为目标文件列表
# 用法：$(call srcs_to_objs,$(SRCS))
srcs_to_objs = $(foreach src,$(1),$(call src_to_obj,$(src)))

# 函数：提取模块名
# 用法：$(call get_module_name,core/osal)
# 结果：osal
get_module_name = $(notdir $(1))

# 函数：生成库文件名
# 用法：$(call get_lib_name,osal)
# 结果：libosal.so
get_lib_name = lib$(1).so

# 函数：生成静态库文件名
# 用法：$(call get_static_lib_name,osal)
# 结果：libosal.a
get_static_lib_name = lib$(1).a

# 函数：检查配置选项
# 用法：$(call check_config,CONFIG_OSAL)
# 结果：如果 CONFIG_OSAL=y，返回 y，否则返回空
check_config = $(filter y,$($(1)))

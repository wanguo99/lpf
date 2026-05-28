# =============================================================================
# PCL 源文件配置
# =============================================================================

# 定义当前模块目录
pcl_src_dir := core/pcl/src

# API 功能（细粒度控制）
ifeq ($(CONFIG_PCL_API),y)
pcl_SRCS += $(pcl_src_dir)/pcl_api.c
endif

ifeq ($(CONFIG_PCL_REGISTER),y)
pcl_SRCS += $(pcl_src_dir)/pcl_register.c
endif

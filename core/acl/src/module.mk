# =============================================================================
# ACL 源文件配置
# =============================================================================

# 定义当前模块目录
acl_src_dir := core/acl/src

# API 功能（细粒度控制）
ifeq ($(CONFIG_ACL_API),y)
acl_SRCS += $(acl_src_dir)/acl_api.c
endif

ifeq ($(CONFIG_ACL_API_V2),y)
acl_SRCS += $(acl_src_dir)/acl_api_v2.c
endif

ifeq ($(CONFIG_ACL_TELEMETRY_CACHE),y)
acl_SRCS += $(acl_src_dir)/acl_telemetry_cache.c
endif

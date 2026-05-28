################# Add include #################
# 添加平台配置头文件目录
list(APPEND ADD_INCLUDE ${PLATFORM_CONFIG_INCLUDE})
###############################################

############## Add source files ###############
file(GLOB_RECURSE ADD_SRCS "src/*.c")

# 添加平台配置源文件
list(APPEND ADD_SRCS ${PLATFORM_CONFIG_SRCS})
###############################################

###### Add required/dependent components ######
# CCM Collector 依赖核心组件和产品库（配置文件直接编译进应用）
list(APPEND ADD_REQUIREMENTS osal hal pcl pdl acl libccm)
###############################################

#### Add compile option for this component ####
# 应用程序特定的编译选项
###############################################

# 注册 main 组件
register_component()

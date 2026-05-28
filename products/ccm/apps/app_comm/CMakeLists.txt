################# Add include #################
# list(APPEND ADD_INCLUDE "include")
###############################################

############## Add source files ###############
file(GLOB_RECURSE ADD_SRCS "src/*.c")
###############################################

###### Add required/dependent components ######
# CCM Comm 依赖核心组件和产品库
list(APPEND ADD_REQUIREMENTS osal hal pcl pdl acl libccm)
###############################################

#### Add compile option for this component ####
# 应用程序特定的编译选项
###############################################

# 注册 main 组件
register_component()

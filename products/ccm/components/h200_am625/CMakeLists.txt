################# Add include #################
list(APPEND ADD_INCLUDE "acl"
    )
list(APPEND ADD_INCLUDE "pcl"
    )
###############################################

############## Add source files ###############
# ACL 配置源文件
file(GLOB ACL_SRCS "acl/*.c")
list(APPEND ADD_SRCS ${ACL_SRCS})

# PCL 配置源文件
file(GLOB PCL_SRCS "pcl/*.c")
list(APPEND ADD_SRCS ${PCL_SRCS})
###############################################

###### Add required/dependent components ######
# h200_am625 依赖核心组件
list(APPEND ADD_REQUIREMENTS osal hal pcl pdl acl)
###############################################

#### Add compile option for this component ####
# 平台特定的编译选项
list(APPEND ADD_DEFINITIONS -DH200_AM625=1)
###############################################

# 注册组件
register_component()

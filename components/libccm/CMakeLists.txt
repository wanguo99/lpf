################# Add include #################
list(APPEND ADD_INCLUDE "include"
    )
# 兼容 #include "libccm/xxx.h" 路径
get_filename_component(PARENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" DIRECTORY)
list(APPEND ADD_INCLUDE "${PARENT_DIR}")
###############################################

############## Add source files ###############
list(APPEND ADD_SRCS
    "src/libccm_ipc.c"
    "src/libccm_protocol.c"
)
###############################################

###### Add required/dependent components ######
# libccm 依赖 OSAL
list(APPEND ADD_REQUIREMENTS osal)
###############################################

#### Add compile option for this component ####
# 产品库特定的编译选项
###############################################

# 注册组件（默认静态库）
register_component()

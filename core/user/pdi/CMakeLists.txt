################# Add include #################
list(APPEND ADD_INCLUDE "include")
list(APPEND ADD_INCLUDE "${SDK_PATH}/core/uapi")
list(APPEND ADD_PUBLIC_HEADER_DIRS "${SDK_PATH}/core/uapi")
###############################################

############## Add source files ###############
list(APPEND ADD_SRCS "src/pdi_mcu.c")
###############################################

###### Add required/dependent components ######
if(CONFIG_OSAL)
    list(APPEND ADD_REQUIREMENTS osal)
endif()
###############################################

# 注册组件（使用公共函数）
list(APPEND ADD_INCLUDE "include/pdi")

if(CONFIG_PDI_BUILD_SHARED)
    register_component(SHARED)
else()
    register_component()
endif()

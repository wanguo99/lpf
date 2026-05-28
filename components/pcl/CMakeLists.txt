################# Add include #################
list(APPEND ADD_INCLUDE "include"
    )
# 兼容 #include "pcl/xxx.h" 路径
get_filename_component(PARENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" DIRECTORY)
list(APPEND ADD_INCLUDE "${PARENT_DIR}")

# 添加平台配置头文件路径（如果有）
if(PCL_EXTRA_INCLUDE)
    list(APPEND ADD_INCLUDE ${PCL_EXTRA_INCLUDE})
endif()
###############################################

############## Add source files ###############
# PCL 核心源文件
list(APPEND ADD_SRCS
    "src/pcl_api.c"
    "src/pcl_register.c"
)

# 添加平台配置源文件（如果有）
if(PCL_EXTRA_SRCS)
    list(APPEND ADD_SRCS ${PCL_EXTRA_SRCS})
endif()
###############################################

###### Add required/dependent components ######
# PCL 依赖 OSAL 和 HAL
list(APPEND ADD_REQUIREMENTS osal hal)
###############################################

#### Add compile option for this component ####
# 根据平台配置添加宏定义
if(CONFIG_PCL_PLATFORM_TI_AM625)
    list(APPEND ADD_DEFINITIONS -DPCL_PLATFORM_TI_AM625=1)

    if(CONFIG_PCL_PRODUCT_H200_100P)
        list(APPEND ADD_DEFINITIONS -DPCL_PRODUCT_H200_100P=1)
    elseif(CONFIG_PCL_PRODUCT_H200_200P)
        list(APPEND ADD_DEFINITIONS -DPCL_PRODUCT_H200_200P=1)
    endif()
elseif(CONFIG_PCL_PLATFORM_VENDOR_DEMO)
    list(APPEND ADD_DEFINITIONS -DPCL_PLATFORM_VENDOR_DEMO=1)
endif()
###############################################

# 注册组件
if(CONFIG_PCL_BUILD_SHARED)
    register_component(SHARED)
else()
    register_component()
endif()

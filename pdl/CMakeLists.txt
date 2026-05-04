# Peripheral Driver Layer (PDL) 模块
# 统一外设管理层

# ============================================================================
# 1. 接口库 (pdl_public_api) - 仅头文件，供上层模块使用
# ============================================================================

# 目录变量
set(PDL_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(PDL_INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(pdl_public_api INTERFACE)

target_include_directories(pdl_public_api INTERFACE
    $<BUILD_INTERFACE:${PDL_INC_DIR}>
    $<INSTALL_INTERFACE:include>
)

# ============================================================================
# 2. 实现库 (pdl) - 包含外设驱动实现
# ============================================================================

# PDL 源文件（所有平台通用）
set(PDL_SOURCES
    # MCU外设驱动（独立封装）
    ${PDL_SRC_DIR}/pdl_mcu/pdl_mcu.c
    ${PDL_SRC_DIR}/pdl_mcu/pdl_mcu_can.c
    ${PDL_SRC_DIR}/pdl_mcu/pdl_mcu_serial.c
    ${PDL_SRC_DIR}/pdl_mcu/pdl_mcu_protocol.c

    # 卫星平台服务（独立封装）
    ${PDL_SRC_DIR}/pdl_satellite/pdl_satellite.c
    ${PDL_SRC_DIR}/pdl_satellite/pdl_satellite_can.c

    # BMC服务（独立封装）
    ${PDL_SRC_DIR}/pdl_bmc/pdl_bmc.c
    ${PDL_SRC_DIR}/pdl_bmc/pdl_bmc_transport.c
    ${PDL_SRC_DIR}/pdl_bmc/pdl_bmc_ipmi.c
    ${PDL_SRC_DIR}/pdl_bmc/pdl_bmc_redfish.c
)

# 创建PDL库（类型由BUILD_SHARED_LIBS控制）
add_library(pdl ${PDL_SOURCES})

# 链接接口库（继承头文件路径）
target_link_libraries(pdl PUBLIC pdl_public_api)

# 依赖下层接口库（仅头文件依赖）
target_link_libraries(pdl PUBLIC
    ems::osal_public_api
    ems::hal_public_api
    ems::pcl_public_api
)

# 链接下层实现库（运行时依赖）
target_link_libraries(pdl PRIVATE
    ems::osal
    ems::hal
    ems::pcl
)

# 内部头文件目录（私有）
target_include_directories(pdl PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/pdl_mcu
    ${CMAKE_CURRENT_SOURCE_DIR}/src/pdl_satellite
    ${CMAKE_CURRENT_SOURCE_DIR}/src/pdl_bmc
)

# 编译选项
target_compile_options(pdl PRIVATE
    -Wall
    -Wextra
    -Werror
)

# ============================================================================
# 3. 安装规则
# ============================================================================
install(TARGETS pdl pdl_public_api
    EXPORT pdl-targets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/
    DESTINATION include/pdl
    FILES_MATCHING PATTERN "*.h"
)

install(EXPORT pdl-targets
    FILE pdl-targets.cmake
    NAMESPACE ems::
    DESTINATION lib/cmake/pdl
)

message(STATUS "PDL configured (MCU + Satellite + BMC drivers)")

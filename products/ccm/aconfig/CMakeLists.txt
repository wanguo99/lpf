if(NOT CONFIG_CCM)
    return()
endif()

# CCM ACONFIG 库
add_library(ccm_aconfig STATIC
    src/ccm_aconfig.c
)

target_include_directories(ccm_aconfig
    PUBLIC
        ${CMAKE_SOURCE_DIR}/products/ccm/include
)

target_link_libraries(ccm_aconfig
    PUBLIC
        osal
        aconfig
)

# 安装
install(TARGETS ccm_aconfig
    ARCHIVE DESTINATION lib
)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/products/ccm/include/ccm/
    DESTINATION include/ccm
    FILES_MATCHING PATTERN "ccm_aconfig*.h"
)

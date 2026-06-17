if(NOT CONFIG_CCM)
    return()
endif()

# PMC ACONFIG 库
add_library(pmc_aconfig STATIC
    src/pmc_aconfig.c
)

target_include_directories(pmc_aconfig
    PUBLIC
        ${CMAKE_SOURCE_DIR}/products/pmc/include
)

target_link_libraries(pmc_aconfig
    PUBLIC
        osal
        aconfig
)

# 安装
install(TARGETS pmc_aconfig
    ARCHIVE DESTINATION lib
)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/products/pmc/include/pmc/
    DESTINATION include/pmc
    FILES_MATCHING PATTERN "pmc_aconfig*.h"
)

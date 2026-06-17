if(NOT CONFIG_PRODUCT_PMC)
    return()
endif()

list(APPEND ADD_INCLUDE
    "${CMAKE_SOURCE_DIR}/products/pmc/include"
    "${CMAKE_SOURCE_DIR}/products/pmc/include/pmc"
)

if(PLATFORM_CONFIG_INCLUDE)
    list(APPEND ADD_INCLUDE ${PLATFORM_CONFIG_INCLUDE})
endif()

list(APPEND ADD_SRCS
    "src/pmc_runtime.c"
)

if(CONFIG_PROJECT_NAME)
    if(PLATFORM_CONFIG_BASE_DIR)
        set(PMC_PLATFORM_CONFIG_DIR "${PLATFORM_CONFIG_BASE_DIR}/${CONFIG_PROJECT_NAME}")
    else()
        set(PMC_PLATFORM_CONFIG_DIR "${CMAKE_SOURCE_DIR}/products/pmc/configs/projects/${CONFIG_PROJECT_NAME}")
    endif()

    if(CONFIG_ACONFIG)
        set(PMC_PLATFORM_ACONFIG_SRC "${PMC_PLATFORM_CONFIG_DIR}/aconfig/aconfig_${CONFIG_PROJECT_NAME}.c")
        if(EXISTS ${PMC_PLATFORM_ACONFIG_SRC})
            list(APPEND ADD_SRCS ${PMC_PLATFORM_ACONFIG_SRC})
        else()
            message(FATAL_ERROR "PMC ACONFIG source not found: ${PMC_PLATFORM_ACONFIG_SRC}")
        endif()
    endif()

    set(PMC_RUNTIME_REGISTER_C "${CMAKE_CURRENT_BINARY_DIR}/pmc_runtime_register.c")
    set(PMC_RUNTIME_REGISTER_SRC [=[
#include "osal.h"

#ifdef CONFIG_PCONFIG
#include "pconfig.h"
extern const pconfig_platform_config_t pconfig_@CONFIG_PROJECT_NAME@;
int32_t PMC_Runtime_RegisterPConfig(void)
{
    int32_t ret = PCONFIG_register(&pconfig_@CONFIG_PROJECT_NAME@);
    if (ret == OSAL_SUCCESS) {
        return PCONFIG_SetBoard(&pconfig_@CONFIG_PROJECT_NAME@);
    }
    if (PCONFIG_GetBoard() != NULL) {
        return PCONFIG_SetBoard(&pconfig_@CONFIG_PROJECT_NAME@);
    }
    return ret;
}
#endif

#ifdef CONFIG_ACONFIG
#include "aconfig.h"
extern const aconfig_config_table_t aconfig_@CONFIG_PROJECT_NAME@;
int32_t PMC_Runtime_RegisterAConfig(void)
{
    return ACONFIG_register_table(&aconfig_@CONFIG_PROJECT_NAME@);
}
#endif
]=])
    string(REPLACE "@CONFIG_PROJECT_NAME@" "${CONFIG_PROJECT_NAME}"
           PMC_RUNTIME_REGISTER_SRC "${PMC_RUNTIME_REGISTER_SRC}")
    file(WRITE ${PMC_RUNTIME_REGISTER_C} "${PMC_RUNTIME_REGISTER_SRC}")
    list(APPEND ADD_SRCS ${PMC_RUNTIME_REGISTER_C})
endif()

list(APPEND ADD_REQUIREMENTS osal)

if(CONFIG_PCONFIG)
    list(APPEND ADD_REQUIREMENTS pconfig)
endif()

if(CONFIG_ACONFIG)
    list(APPEND ADD_REQUIREMENTS aconfig pmc_aconfig)
endif()

register_component()

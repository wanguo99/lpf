add_library(pdm_core INTERFACE)
target_include_directories(pdm_core INTERFACE
    ${SDK_PATH}/kernel/include
    ${SDK_PATH}/kernel/pdm/core
    ${SDK_PATH}/kernel/pdm/bus
)

add_library(pdm INTERFACE)
target_include_directories(pdm INTERFACE
    ${SDK_PATH}/kernel/include
    ${SDK_PATH}/kernel/pdm/core
    ${SDK_PATH}/kernel/pdm/bus
)

add_library(pdm INTERFACE)
target_include_directories(pdm INTERFACE
    ${SDK_PATH}/kernel/include
    ${SDK_PATH}/kernel/pdm
    ${SDK_PATH}/kernel/pdm/bus
    ${SDK_PATH}/kernel/pdm/diag
    ${SDK_PATH}/kernel/pdm/registry
    ${SDK_PATH}/kernel/pdm/log
    ${SDK_PATH}/kernel/pdm/drivers
)

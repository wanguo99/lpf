add_library(lpf_core INTERFACE)
target_include_directories(lpf_core INTERFACE
    ${SDK_PATH}/kernel/include
    ${SDK_PATH}/kernel/include/lpf
    ${SDK_PATH}/kernel/lpf-core/core
    ${SDK_PATH}/kernel/lpf-core/protocol
    ${SDK_PATH}/kernel/lpf-core/compat
    ${SDK_PATH}/kernel/lpf-core/soc
    ${SDK_PATH}/kernel/lpf-core/include
    ${SDK_PATH}/kernel/lpf-core/config/src
    ${SDK_PATH}/kernel/lpf-core/hw
    ${SDK_PATH}/kernel/lpf-core/peripheral/mcu
    ${SDK_PATH}/kernel/lpf-core/peripheral/led
)

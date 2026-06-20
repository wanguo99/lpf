add_executable(lpf_config_normalizer_test
    test_lpf_config_normalizer.c
    "${SDK_PATH}/kernel/lpf/config/src/lpf_config_normalizer.c"
    "${SDK_PATH}/kernel/lpf/config/configs/kernel/x86_mock_modules/1.0.0/lpf_config_kernel_x86_mock_modules.c"
)

target_include_directories(lpf_config_normalizer_test PRIVATE
    "${SDK_PATH}/kernel/include"
    "${SDK_PATH}/kernel/include/lpf"
    "${SDK_PATH}/kernel/lpf/config/src"
    "${SDK_PATH}/user/osal/include/osal"
)

add_test(NAME lpf_config_normalizer COMMAND lpf_config_normalizer_test)

add_executable(lpf_config_dt_parser_test
    test_lpf_config_dt_parser.c
    "${SDK_PATH}/kernel/lpf/config/src/lpf_config_dt_parser.c"
    "${SDK_PATH}/kernel/lpf/config/src/lpf_config_normalizer.c"
    "${SDK_PATH}/kernel/lpf/config/configs/kernel/x86_mock_modules/1.0.0/lpf_config_kernel_x86_mock_modules.c"
    "${SDK_PATH}/user/osal/src/lib/osal_string.c"
)

target_include_directories(lpf_config_dt_parser_test PRIVATE
    "${SDK_PATH}/kernel/include"
    "${SDK_PATH}/kernel/include/lpf"
    "${SDK_PATH}/kernel/lpf/config/src"
    "${SDK_PATH}/user/osal/include/osal"
)

target_link_libraries(lpf_config_dt_parser_test PRIVATE osal)

add_test(NAME lpf_config_dt_parser COMMAND lpf_config_dt_parser_test)

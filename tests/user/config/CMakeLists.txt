add_executable(lpf_config_normalizer_test
    test_lpf_config_normalizer.c
    test_lpf_config_compare.c
    "${SDK_PATH}/kernel/lpf-core/config/src/lpf_config_normalizer.c"
    "${SDK_PATH}/kernel/lpf-core/config/src/lpf_config_validator.c"
    "${SDK_PATH}/kernel/lpf-configs/lpf_config_static_start.c"
    "${SDK_PATH}/kernel/lpf-configs/configs/kernel/x86_mock_modules/lpf_config_kernel_x86_mock_modules_v1.c"
    "${SDK_PATH}/kernel/lpf-configs/lpf_config_static_end.c"
)

target_include_directories(lpf_config_normalizer_test PRIVATE
    "${SDK_PATH}/kernel/include"
    "${SDK_PATH}/kernel/include/lpf"
    "${SDK_PATH}/kernel/lpf-core/config/src"
    "${SDK_PATH}/user/osal/include/osal"
)

target_compile_definitions(lpf_config_normalizer_test PRIVATE
    OSAL_LOG_COMPILE_LEVEL=5
)

add_test(NAME lpf_config_normalizer COMMAND lpf_config_normalizer_test)

add_executable(lpf_config_dt_parser_test
    test_lpf_config_dt_parser.c
    test_lpf_config_compare.c
    "${SDK_PATH}/kernel/lpf-core/config/src/lpf_config_dt_parser.c"
    "${SDK_PATH}/kernel/lpf-core/config/src/lpf_config_normalizer.c"
    "${SDK_PATH}/kernel/lpf-core/config/src/lpf_config_validator.c"
    "${SDK_PATH}/kernel/lpf-configs/lpf_config_static_start.c"
    "${SDK_PATH}/kernel/lpf-configs/configs/kernel/x86_mock_modules/lpf_config_kernel_x86_mock_modules_v1.c"
    "${SDK_PATH}/kernel/lpf-configs/lpf_config_static_end.c"
    "${SDK_PATH}/user/osal/src/lib/osal_string.c"
)

target_include_directories(lpf_config_dt_parser_test PRIVATE
    "${SDK_PATH}/kernel/include"
    "${SDK_PATH}/kernel/include/lpf"
    "${SDK_PATH}/kernel/lpf-core/config/src"
    "${SDK_PATH}/user/osal/include/osal"
)

target_compile_definitions(lpf_config_dt_parser_test PRIVATE
    OSAL_LOG_COMPILE_LEVEL=5
)

target_link_libraries(lpf_config_dt_parser_test PRIVATE osal)

add_test(NAME lpf_config_dt_parser COMMAND lpf_config_dt_parser_test)

add_executable(lpf_architecture_boundaries_test
    test_lpf_architecture_boundaries.c
)

target_compile_definitions(lpf_architecture_boundaries_test PRIVATE
    SDK_SOURCE_DIR="${SDK_PATH}"
)

add_test(NAME lpf_architecture_boundaries
    COMMAND lpf_architecture_boundaries_test
)

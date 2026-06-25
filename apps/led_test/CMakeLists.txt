add_executable(led_test main.c)
target_link_libraries(led_test PRIVATE pdi osal)
target_include_directories(led_test PRIVATE
    ${CMAKE_SOURCE_DIR}/user/pdi/include
    ${CMAKE_SOURCE_DIR}/include
)

install(TARGETS led_test DESTINATION bin)

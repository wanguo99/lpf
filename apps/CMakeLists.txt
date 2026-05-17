# Apps层 - 应用程序

# 添加示例应用
if(BUILD_SAMPLE_APP)
    add_subdirectory(sample_app)
endif()

# 添加看门狗应用
if(BUILD_WATCHDOG_APP)
    add_subdirectory(watchdog_app)
endif()

# 添加遥控进程（实时）
add_subdirectory(telecommand)

# 添加遥测进程（后台）
add_subdirectory(telemetry)

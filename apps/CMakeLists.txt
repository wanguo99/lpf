if(CONFIG_PRINT_TEST)
    add_subdirectory(print_test)
endif()

if(CONFIG_SMART_MONITOR)
    add_subdirectory(smart_monitor)
endif()

if(CONFIG_MEMORY_MONITOR)
    add_subdirectory(memory_monitor)
endif()

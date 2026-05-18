# Apps层 - 应用程序

# 添加示例应用
if(BUILD_SAMPLE_APP)
    add_subdirectory(sample_app)
endif()

# PMC产品 - 5进程架构
# 注意：这些进程依赖libpmc库，确保libs目录已在根CMakeLists.txt中添加

# 1. Communication进程 - 遥控遥测通信
add_subdirectory(pmc_comm)

# 2. Collector进程 - 数据采集
add_subdirectory(pmc_collector)

# 3. Health进程 - 健康监测
add_subdirectory(pmc_health)

# 4. Supervisor进程 - 进程监督管理
add_subdirectory(pmc_supervisor)

# 5. Logger进程 - 日志管理
add_subdirectory(pmc_logger)

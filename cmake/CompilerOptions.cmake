# =============================================================================
# 编译选项配置
# =============================================================================
# 功能：根据 Kconfig 配置设置编译器选项
# 用法：include(cmake/CompilerOptions.cmake)
# =============================================================================

message(STATUS "Configuring compiler options...")

# =============================================================================
# 优化级别（根据 Kconfig 配置）
# =============================================================================

if(CONFIG_OPT_O0)
    add_compile_options(-O0)
    message(STATUS "  [Compiler] Optimization: -O0 (no optimization)")
elseif(CONFIG_OPT_O2)
    add_compile_options(-O2)
    message(STATUS "  [Compiler] Optimization: -O2 (standard)")
elseif(CONFIG_OPT_O3)
    add_compile_options(-O3)
    message(STATUS "  [Compiler] Optimization: -O3 (aggressive)")
elseif(CONFIG_OPT_OS)
    add_compile_options(-Os)
    message(STATUS "  [Compiler] Optimization: -Os (size)")
else()
    # 默认使用 -O2
    add_compile_options(-O2)
    message(STATUS "  [Compiler] Optimization: -O2 (default)")
endif()

# =============================================================================
# 调试选项（根据 Kconfig 配置）
# =============================================================================

if(CONFIG_BUILD_TYPE_DEBUG)
    add_compile_options(-g3)
    add_compile_definitions(DEBUG)
    message(STATUS "  [Compiler] Debug: enabled (-g3)")
else()
    add_compile_definitions(NDEBUG)
    message(STATUS "  [Compiler] Debug: disabled")
endif()

# =============================================================================
# 警告选项
# =============================================================================

add_compile_options(
    -Wall
    -Wundef
    -Wstrict-prototypes
    -Wno-trigraphs
    -fno-strict-aliasing
    -fno-common
    -Werror-implicit-function-declaration
    -Wno-format-security
    -fomit-frame-pointer
    -fPIC
)

message(STATUS "  [Compiler] Warning flags: enabled")

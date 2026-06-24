add_executable(print_test
    print_test.c
)

install(TARGETS print_test
    RUNTIME DESTINATION ${INSTALL_BINDIR}
    COMPONENT tools
)

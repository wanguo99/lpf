cmd_scripts/kconfig/mnconf-common.o := gcc -Wp,-MMD,scripts/kconfig/.mnconf-common.o.d -I/home/wanguo/CSPD/ES-Middleware/scripts/include   -c -o scripts/kconfig/mnconf-common.o scripts/kconfig/mnconf-common.c

deps_scripts/kconfig/mnconf-common.o := \
  scripts/kconfig/mnconf-common.c \
  /home/wanguo/CSPD/ES-Middleware/scripts/include/list.h \
  /home/wanguo/CSPD/ES-Middleware/scripts/include/list_types.h \
  scripts/kconfig/expr.h \
  /home/wanguo/CSPD/ES-Middleware/scripts/include/list_types.h \
  scripts/kconfig/mnconf-common.h \

scripts/kconfig/mnconf-common.o: $(deps_scripts/kconfig/mnconf-common.o)

$(deps_scripts/kconfig/mnconf-common.o):

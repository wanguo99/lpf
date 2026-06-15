cmd_scripts/kconfig/conf.o := gcc -Wp,-MMD,scripts/kconfig/.conf.o.d -I/home/wanguo/CSPD/ES-Middleware/scripts/include   -c -o scripts/kconfig/conf.o scripts/kconfig/conf.c

deps_scripts/kconfig/conf.o := \
  scripts/kconfig/conf.c \
  scripts/kconfig/internal.h \
  /home/wanguo/CSPD/ES-Middleware/scripts/include/hashtable.h \
  /home/wanguo/CSPD/ES-Middleware/scripts/include/array_size.h \
  /home/wanguo/CSPD/ES-Middleware/scripts/include/list.h \
  /home/wanguo/CSPD/ES-Middleware/scripts/include/list_types.h \
  scripts/kconfig/lkc.h \
    $(wildcard include/config/prefix.h) \
  scripts/kconfig/expr.h \
  /home/wanguo/CSPD/ES-Middleware/scripts/include/list_types.h \
  scripts/kconfig/lkc_proto.h \

scripts/kconfig/conf.o: $(deps_scripts/kconfig/conf.o)

$(deps_scripts/kconfig/conf.o):

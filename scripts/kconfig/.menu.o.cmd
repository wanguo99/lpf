cmd_scripts/kconfig/menu.o := gcc -Wp,-MMD,scripts/kconfig/.menu.o.d -I/home/wanguo/CSPD/ES-Middleware/scripts/include   -c -o scripts/kconfig/menu.o scripts/kconfig/menu.c

deps_scripts/kconfig/menu.o := \
  scripts/kconfig/menu.c \
  /home/wanguo/CSPD/ES-Middleware/scripts/include/list.h \
  /home/wanguo/CSPD/ES-Middleware/scripts/include/list_types.h \
  /home/wanguo/CSPD/ES-Middleware/scripts/include/xalloc.h \
  scripts/kconfig/lkc.h \
    $(wildcard include/config/prefix.h) \
  scripts/kconfig/expr.h \
  /home/wanguo/CSPD/ES-Middleware/scripts/include/list_types.h \
  scripts/kconfig/lkc_proto.h \
  scripts/kconfig/internal.h \
  /home/wanguo/CSPD/ES-Middleware/scripts/include/hashtable.h \
  /home/wanguo/CSPD/ES-Middleware/scripts/include/array_size.h \
  /home/wanguo/CSPD/ES-Middleware/scripts/include/list.h \

scripts/kconfig/menu.o: $(deps_scripts/kconfig/menu.o)

$(deps_scripts/kconfig/menu.o):

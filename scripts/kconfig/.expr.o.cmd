cmd_scripts/kconfig/expr.o := gcc -Wp,-MMD,scripts/kconfig/.expr.o.d -I/home/wanguo/CSPD/ES-Middleware/scripts/include   -c -o scripts/kconfig/expr.o scripts/kconfig/expr.c

deps_scripts/kconfig/expr.o := \
  scripts/kconfig/expr.c \
  /home/wanguo/CSPD/ES-Middleware/scripts/include/hash.h \
  /home/wanguo/CSPD/ES-Middleware/scripts/include/xalloc.h \
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

scripts/kconfig/expr.o: $(deps_scripts/kconfig/expr.o)

$(deps_scripts/kconfig/expr.o):

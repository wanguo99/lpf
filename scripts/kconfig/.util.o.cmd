cmd_scripts/kconfig/util.o := gcc -Wp,-MMD,scripts/kconfig/.util.o.d -I/home/wanguo/CSPD/ES-Middleware/scripts/include   -c -o scripts/kconfig/util.o scripts/kconfig/util.c

deps_scripts/kconfig/util.o := \
  scripts/kconfig/util.c \
  /home/wanguo/CSPD/ES-Middleware/scripts/include/hash.h \
  /home/wanguo/CSPD/ES-Middleware/scripts/include/hashtable.h \
  /home/wanguo/CSPD/ES-Middleware/scripts/include/array_size.h \
  /home/wanguo/CSPD/ES-Middleware/scripts/include/list.h \
  /home/wanguo/CSPD/ES-Middleware/scripts/include/list_types.h \
  /home/wanguo/CSPD/ES-Middleware/scripts/include/xalloc.h \
  scripts/kconfig/lkc.h \
    $(wildcard include/config/prefix.h) \
  scripts/kconfig/expr.h \
  /home/wanguo/CSPD/ES-Middleware/scripts/include/list_types.h \
  scripts/kconfig/lkc_proto.h \

scripts/kconfig/util.o: $(deps_scripts/kconfig/util.o)

$(deps_scripts/kconfig/util.o):

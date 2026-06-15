cmd_scripts/kconfig/confdata.o := gcc -Wp,-MMD,scripts/kconfig/.confdata.o.d -I/home/wanguo/CSPD/ES-Middleware/scripts/include   -c -o scripts/kconfig/confdata.o scripts/kconfig/confdata.c

deps_scripts/kconfig/confdata.o := \
  scripts/kconfig/confdata.c \
    $(wildcard include/config/foo.h) \
    $(wildcard include/config/x.h) \
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

scripts/kconfig/confdata.o: $(deps_scripts/kconfig/confdata.o)

$(deps_scripts/kconfig/confdata.o):

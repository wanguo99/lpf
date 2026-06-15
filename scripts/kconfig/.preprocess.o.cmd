cmd_scripts/kconfig/preprocess.o := gcc -Wp,-MMD,scripts/kconfig/.preprocess.o.d -I/home/wanguo/CSPD/ES-Middleware/scripts/include   -c -o scripts/kconfig/preprocess.o scripts/kconfig/preprocess.c

deps_scripts/kconfig/preprocess.o := \
  scripts/kconfig/preprocess.c \
  /home/wanguo/CSPD/ES-Middleware/scripts/include/array_size.h \
  /home/wanguo/CSPD/ES-Middleware/scripts/include/list.h \
  /home/wanguo/CSPD/ES-Middleware/scripts/include/list_types.h \
  /home/wanguo/CSPD/ES-Middleware/scripts/include/xalloc.h \
  scripts/kconfig/internal.h \
  /home/wanguo/CSPD/ES-Middleware/scripts/include/hashtable.h \
  /home/wanguo/CSPD/ES-Middleware/scripts/include/array_size.h \
  /home/wanguo/CSPD/ES-Middleware/scripts/include/list.h \
  scripts/kconfig/lkc.h \
    $(wildcard include/config/prefix.h) \
  scripts/kconfig/expr.h \
  /home/wanguo/CSPD/ES-Middleware/scripts/include/list_types.h \
  scripts/kconfig/lkc_proto.h \
  scripts/kconfig/preprocess.h \

scripts/kconfig/preprocess.o: $(deps_scripts/kconfig/preprocess.o)

$(deps_scripts/kconfig/preprocess.o):

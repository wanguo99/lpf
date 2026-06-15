cmd_scripts/kconfig/parser.tab.o := gcc -Wp,-MMD,scripts/kconfig/.parser.tab.o.d -I/home/wanguo/CSPD/ES-Middleware/scripts/include  -I /home/wanguo/CSPD/ES-Middleware/scripts/kconfig -c -o scripts/kconfig/parser.tab.o scripts/kconfig/parser.tab.c

deps_scripts/kconfig/parser.tab.o := \
  scripts/kconfig/parser.tab.c \
    $(wildcard include/config/nls.h) \
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
  /home/wanguo/CSPD/ES-Middleware/scripts/include/list_types.h \
  scripts/kconfig/preprocess.h \
  scripts/kconfig/parser.tab.h \

scripts/kconfig/parser.tab.o: $(deps_scripts/kconfig/parser.tab.o)

$(deps_scripts/kconfig/parser.tab.o):

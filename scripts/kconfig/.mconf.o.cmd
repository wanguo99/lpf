cmd_scripts/kconfig/mconf.o := gcc -Wp,-MMD,scripts/kconfig/.mconf.o.d -I/home/wanguo/CSPD/ES-Middleware/scripts/include  -D_GNU_SOURCE -I/usr/include/ncursesw -c -o scripts/kconfig/mconf.o scripts/kconfig/mconf.c

deps_scripts/kconfig/mconf.o := \
  scripts/kconfig/mconf.c \
  /home/wanguo/CSPD/ES-Middleware/scripts/include/list.h \
  /home/wanguo/CSPD/ES-Middleware/scripts/include/list_types.h \
  /home/wanguo/CSPD/ES-Middleware/scripts/include/xalloc.h \
  scripts/kconfig/lkc.h \
    $(wildcard include/config/prefix.h) \
  scripts/kconfig/expr.h \
  /home/wanguo/CSPD/ES-Middleware/scripts/include/list_types.h \
  scripts/kconfig/lkc_proto.h \
  scripts/kconfig/lxdialog/dialog.h \
  /usr/include/ncursesw/ncurses.h \
  /usr/include/ncursesw/ncurses_dll.h \
  /usr/include/ncursesw/unctrl.h \
  /usr/include/ncursesw/curses.h \
  scripts/kconfig/mnconf-common.h \

scripts/kconfig/mconf.o: $(deps_scripts/kconfig/mconf.o)

$(deps_scripts/kconfig/mconf.o):

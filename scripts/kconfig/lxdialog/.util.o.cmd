cmd_scripts/kconfig/lxdialog/util.o := gcc -Wp,-MMD,scripts/kconfig/lxdialog/.util.o.d -I/home/wanguo/CSPD/ES-Middleware/scripts/include  -D_GNU_SOURCE -I/usr/include/ncursesw -c -o scripts/kconfig/lxdialog/util.o scripts/kconfig/lxdialog/util.c

deps_scripts/kconfig/lxdialog/util.o := \
  scripts/kconfig/lxdialog/util.c \
  scripts/kconfig/lxdialog/dialog.h \
  /usr/include/ncursesw/ncurses.h \
  /usr/include/ncursesw/ncurses_dll.h \
  /usr/include/ncursesw/unctrl.h \
  /usr/include/ncursesw/curses.h \

scripts/kconfig/lxdialog/util.o: $(deps_scripts/kconfig/lxdialog/util.o)

$(deps_scripts/kconfig/lxdialog/util.o):

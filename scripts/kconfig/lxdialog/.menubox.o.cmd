cmd_scripts/kconfig/lxdialog/menubox.o := gcc -Wp,-MMD,scripts/kconfig/lxdialog/.menubox.o.d -I/home/wanguo/CSPD/ES-Middleware/scripts/include  -D_GNU_SOURCE -I/usr/include/ncursesw -c -o scripts/kconfig/lxdialog/menubox.o scripts/kconfig/lxdialog/menubox.c

deps_scripts/kconfig/lxdialog/menubox.o := \
  scripts/kconfig/lxdialog/menubox.c \
  scripts/kconfig/lxdialog/dialog.h \
  /usr/include/ncursesw/ncurses.h \
  /usr/include/ncursesw/ncurses_dll.h \
  /usr/include/ncursesw/unctrl.h \
  /usr/include/ncursesw/curses.h \

scripts/kconfig/lxdialog/menubox.o: $(deps_scripts/kconfig/lxdialog/menubox.o)

$(deps_scripts/kconfig/lxdialog/menubox.o):

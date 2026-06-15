cmd_scripts/kconfig/lxdialog/checklist.o := gcc -Wp,-MMD,scripts/kconfig/lxdialog/.checklist.o.d -I/home/wanguo/CSPD/ES-Middleware/scripts/include  -D_GNU_SOURCE -I/usr/include/ncursesw -c -o scripts/kconfig/lxdialog/checklist.o scripts/kconfig/lxdialog/checklist.c

deps_scripts/kconfig/lxdialog/checklist.o := \
  scripts/kconfig/lxdialog/checklist.c \
  scripts/kconfig/lxdialog/dialog.h \
  /usr/include/ncursesw/ncurses.h \
  /usr/include/ncursesw/ncurses_dll.h \
  /usr/include/ncursesw/unctrl.h \
  /usr/include/ncursesw/curses.h \

scripts/kconfig/lxdialog/checklist.o: $(deps_scripts/kconfig/lxdialog/checklist.o)

$(deps_scripts/kconfig/lxdialog/checklist.o):

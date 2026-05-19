# Standalone Kconfig Tools Makefile
# Simplified version for building conf and mconf outside kernel tree

HOSTCC := gcc
HOSTCXX := g++

# ncurses configuration
NCURSES_CFLAGS := $(shell pkg-config --cflags ncurses 2>/dev/null)
NCURSES_LIBS := $(shell pkg-config --libs ncurses 2>/dev/null || echo -lncurses)

# Common flags
HOSTCFLAGS := -Wall -Wmissing-prototypes -Wstrict-prototypes -O2 -fomit-frame-pointer
HOSTCFLAGS += -I. -I../include -DCURSES_LOC="<ncurses.h>"

# Targets
.PHONY: all conf mconf clean

all: conf mconf

# conf - command-line configuration tool
conf-objs := conf.o confdata.o expr.o lexer.lex.o menu.o parser.tab.o \
             preprocess.o symbol.o util.o
conf: $(conf-objs)
	$(HOSTCC) -o $@ $^

# mconf - menuconfig (ncurses interface)
mconf-objs := mconf.o mnconf-common.o confdata.o expr.o lexer.lex.o menu.o parser.tab.o \
              preprocess.o symbol.o util.o lxdialog/checklist.o lxdialog/inputbox.o \
              lxdialog/menubox.o lxdialog/textbox.o lxdialog/util.o lxdialog/yesno.o
mconf: $(mconf-objs)
	$(HOSTCC) -o $@ $^ $(NCURSES_LIBS)

# Parser generation (must be done first)
parser.tab.c parser.tab.h: parser.y
	bison -d -o parser.tab.c -t -l $<

lexer.lex.c: lexer.l parser.tab.h
	flex -o $@ -L $<

# Compilation rules
%.o: %.c
	$(HOSTCC) $(HOSTCFLAGS) -c -o $@ $<

lxdialog/%.o: lxdialog/%.c
	$(HOSTCC) $(HOSTCFLAGS) $(NCURSES_CFLAGS) -c -o $@ $<

# lexer.lex.o depends on parser.tab.h
lexer.lex.o: lexer.lex.c parser.tab.h

# Clean
clean:
	rm -f conf mconf nconf gconf qconf *.o lxdialog/*.o
	rm -f parser.tab.c parser.tab.h lexer.lex.c
	rm -f .*.cmd *.cmd

.PHONY: help
help:
	@echo "Kconfig Tools Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  all (default) - Build conf and mconf"
	@echo "  conf          - Build command-line config tool"
	@echo "  mconf         - Build menuconfig (ncurses)"
	@echo "  clean         - Remove built files"
	@echo ""
	@echo "Usage:"
	@echo "  make          # Build both tools"
	@echo "  make conf     # Build only conf"
	@echo "  make mconf    # Build only mconf"

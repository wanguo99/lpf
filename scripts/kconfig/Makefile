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

# Quiet mode (like Linux kernel)
ifeq ($(V),1)
  Q =
else
  Q = @
endif

# Targets
.PHONY: all conf mconf clean defconfig menuconfig oldconfig syncconfig savedefconfig olddefconfig

all: conf mconf

# Configuration targets
# These targets invoke the kconfig tools to process configuration

# Kconfig file location (passed from top-level Makefile)
Kconfig ?= Kconfig
srctree ?= .
objtree ?= .

# Get the absolute path of this Makefile's directory
KCONFIG_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

defconfig: conf
	$(Q)cd $(objtree) && $(KCONFIG_DIR)conf --defconfig=$(srctree)/configs/defconfig $(srctree)/$(Kconfig)

%_defconfig: conf
	$(Q)cd $(objtree) && $(KCONFIG_DIR)conf --defconfig=$(srctree)/configs/$@ $(srctree)/$(Kconfig)

menuconfig: mconf
	$(Q)cd $(objtree) && $(KCONFIG_DIR)mconf $(srctree)/$(Kconfig)

config: conf
	$(Q)cd $(objtree) && $(KCONFIG_DIR)conf --oldaskconfig $(srctree)/$(Kconfig)

oldconfig: conf
	$(Q)cd $(objtree) && $(KCONFIG_DIR)conf --oldconfig $(srctree)/$(Kconfig)

syncconfig: conf
	$(Q)cd $(objtree) && $(KCONFIG_DIR)conf --syncconfig $(srctree)/$(Kconfig)

olddefconfig: conf
	$(Q)cd $(objtree) && $(KCONFIG_DIR)conf --olddefconfig $(srctree)/$(Kconfig)

savedefconfig: conf
	$(Q)cd $(objtree) && $(KCONFIG_DIR)conf --savedefconfig=$(srctree)/defconfig $(srctree)/$(Kconfig)

# conf - command-line configuration tool
conf-objs := conf.o confdata.o expr.o lexer.lex.o menu.o parser.tab.o \
             preprocess.o symbol.o util.o
conf: $(conf-objs)
	$(Q)echo "  HOSTLD  $@"
	$(Q)$(HOSTCC) -o $@ $^

# mconf - menuconfig (ncurses interface)
mconf-objs := mconf.o mnconf-common.o confdata.o expr.o lexer.lex.o menu.o parser.tab.o \
              preprocess.o symbol.o util.o lxdialog/checklist.o lxdialog/inputbox.o \
              lxdialog/menubox.o lxdialog/textbox.o lxdialog/util.o lxdialog/yesno.o
mconf: $(mconf-objs)
	$(Q)echo "  HOSTLD  $@"
	$(Q)$(HOSTCC) -o $@ $^ $(NCURSES_LIBS)

# Parser generation (must be done first)
parser.tab.c parser.tab.h: parser.y
	$(Q)echo "  YACC    $@"
	$(Q)bison -d -o parser.tab.c -t -l $<

lexer.lex.c: lexer.l parser.tab.h
	$(Q)echo "  LEX     $@"
	$(Q)flex -o $@ -L $<

# Compilation rules
%.o: %.c
	$(Q)echo "  HOSTCC  $@"
	$(Q)$(HOSTCC) $(HOSTCFLAGS) -c -o $@ $<

lxdialog/%.o: lxdialog/%.c
	$(Q)echo "  HOSTCC  $@"
	$(Q)$(HOSTCC) $(HOSTCFLAGS) $(NCURSES_CFLAGS) -c -o $@ $<

# lexer.lex.o depends on parser.tab.h
lexer.lex.o: lexer.lex.c parser.tab.h

# Clean
clean:
	$(Q)rm -f conf mconf nconf gconf qconf *.o lxdialog/*.o
	$(Q)rm -f parser.tab.c parser.tab.h lexer.lex.c
	$(Q)rm -f .*.cmd *.cmd
	$(Q)rm -f .config .config.old
	$(Q)rm -rf include/

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

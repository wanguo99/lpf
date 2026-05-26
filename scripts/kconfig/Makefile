# Standalone Kconfig Tools Makefile
# Simplified version for building conf and mconf outside kernel tree

# 清理时跳过所有构建规则
ifneq ($(MAKECMDGOALS),clean)
ifneq ($(obj),)
# 当通过 Makefile.clean 调用时，obj 变量会被设置，此时也跳过构建规则
# 但是定义空的配置目标以避免错误
.PHONY: olddefconfig syncconfig defconfig menuconfig oldconfig savedefconfig
olddefconfig syncconfig defconfig menuconfig oldconfig savedefconfig:
	@:
else

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
  MAKEFLAGS += --no-print-directory
else
  Q = @
  MAKEFLAGS += --no-print-directory -s
endif

# Targets
.PHONY: all conf mconf defconfig menuconfig oldconfig syncconfig savedefconfig olddefconfig

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
	@echo "  HOSTLD  $@"
	$(Q)$(HOSTCC) -o $@ $^

# mconf - menuconfig (ncurses interface)
mconf-objs := mconf.o mnconf-common.o confdata.o expr.o lexer.lex.o menu.o parser.tab.o \
              preprocess.o symbol.o util.o lxdialog/checklist.o lxdialog/inputbox.o \
              lxdialog/menubox.o lxdialog/textbox.o lxdialog/util.o lxdialog/yesno.o
mconf: $(mconf-objs)
	@echo "  HOSTLD  $@"
	$(Q)$(HOSTCC) -o $@ $^ $(NCURSES_LIBS)

# Parser generation (must be done first)
# Note: bison generates both .c and .h files in one invocation
# Use grouped targets (&:) to avoid running the rule twice in parallel builds
parser.tab.c parser.tab.h &: parser.y
	@echo "  YACC    parser.tab.c parser.tab.h"
	$(Q)bison -d -o parser.tab.c -t -l $<

lexer.lex.c: lexer.l parser.tab.h
	@echo "  LEX     $@"
	$(Q)flex -o $@ -L $<

# Compilation rules
%.o: %.c
	$(if $(filter-out 1,$(V)),,@echo "  HOSTCC  $@")
	$(Q)$(HOSTCC) $(HOSTCFLAGS) -c -o $@ $<

lxdialog/%.o: lxdialog/%.c
	$(if $(filter-out 1,$(V)),,@echo "  HOSTCC  $@")
	$(Q)$(HOSTCC) $(HOSTCFLAGS) $(NCURSES_CFLAGS) -c -o $@ $<

# lexer.lex.o depends on parser.tab.h
lexer.lex.o: lexer.lex.c parser.tab.h

endif # obj 检查
endif # MAKECMDGOALS 检查

# 清理变量 - 供 Makefile.clean 使用（参考内核方式）
hostprogs := conf mconf
targets := parser.tab.c parser.tab.h lexer.lex.c
clean-files := .config .config.old
clean-files += conf.o confdata.o expr.o lexer.lex.o menu.o parser.tab.o
clean-files += preprocess.o symbol.o util.o mconf.o mnconf-common.o
clean-files += lxdialog/checklist.o lxdialog/inputbox.o lxdialog/menubox.o
clean-files += lxdialog/textbox.o lxdialog/util.o lxdialog/yesno.o
clean-dirs := include

# Clean - 用于直接调用 make -C scripts/kconfig clean
.PHONY: clean
clean:
	@rm -f conf mconf nconf gconf qconf *.o lxdialog/*.o
	@rm -f parser.tab.c parser.tab.h lexer.lex.c
	@rm -f .*.cmd *.cmd
	@rm -f .config .config.old
	@rm -rf include/

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

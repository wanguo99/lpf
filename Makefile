# =============================================================================
# EMS (Embedded Middleware System) 顶层 Makefile
# =============================================================================
# 版本信息
VERSION = 1
PATCHLEVEL = 0
SUBLEVEL = 0
EXTRAVERSION =
NAME = EMS

# =============================================================================
# 基础设置
# =============================================================================

# 禁用 make 内置规则和变量，提升性能
MAKEFLAGS += -rR --include-dir=$(CURDIR)

# 避免字符集依赖
unexport LC_ALL
LC_COLLATE=C
LC_NUMERIC=C
export LC_COLLATE LC_NUMERIC

# 避免环境变量干扰
unexport GREP_OPTIONS

# =============================================================================
# 静默构建支持 (V=0 静默, V=1 详细)
# =============================================================================

ifeq ("$(origin V)", "command line")
  KBUILD_VERBOSE = $(V)
endif
ifndef KBUILD_VERBOSE
  KBUILD_VERBOSE = 0
endif

ifeq ($(KBUILD_VERBOSE),1)
  quiet =
  Q =
else
  quiet=quiet_
  Q = @
endif

# 检测 make -s (静默模式)
ifneq ($(filter 4.%,$(MAKE_VERSION)),)	# make-4
ifneq ($(filter %s ,$(firstword x$(MAKEFLAGS))),)
  quiet=silent_
endif
else					# make-3.8x
ifneq ($(filter s% -s%,$(MAKEFLAGS)),)
  quiet=silent_
endif
endif

export quiet Q KBUILD_VERBOSE

# =============================================================================
# 外部构建支持 (O=dir 或 KBUILD_OUTPUT)
# =============================================================================

# KBUILD_SRC 在外部构建目录调用时设置
ifeq ($(KBUILD_SRC),)

# 在源码目录调用 make
# 检查是否需要外部构建
ifeq ("$(origin O)", "command line")
  KBUILD_OUTPUT := $(O)
endif

# 默认目标
PHONY := _all
_all:

# 取消顶层 Makefile 的隐式规则
$(CURDIR)/Makefile Makefile: ;

# 检查目录路径中不能包含空格或冒号
ifneq ($(words $(subst :, ,$(CURDIR))), 1)
  $(error main directory cannot contain spaces nor colons)
endif

# 外部构建逻辑
ifneq ($(KBUILD_OUTPUT),)
# 在输出目录调用第二次 make
saved-output := $(KBUILD_OUTPUT)
KBUILD_OUTPUT := $(shell mkdir -p $(KBUILD_OUTPUT) && cd $(KBUILD_OUTPUT) && /bin/pwd)
$(if $(KBUILD_OUTPUT),, \
     $(error failed to create output directory "$(saved-output)"))

PHONY += $(MAKECMDGOALS) sub-make

$(filter-out _all sub-make $(CURDIR)/Makefile, $(MAKECMDGOALS)) _all: sub-make
	@:

sub-make:
	$(Q)$(MAKE) -C $(KBUILD_OUTPUT) KBUILD_SRC=$(CURDIR) \
	-f $(CURDIR)/Makefile $(filter-out _all sub-make,$(MAKECMDGOALS))

# 跳过后续处理
skip-makefile := 1
endif # ifneq ($(KBUILD_OUTPUT),)
endif # ifeq ($(KBUILD_SRC),)

# =============================================================================
# 主构建逻辑（仅在最终调用时执行）
# =============================================================================
ifeq ($(skip-makefile),)

# 不打印 "Entering directory ..."
MAKEFLAGS += --no-print-directory

# 稀疏检查支持 (C=1 检查重新编译的文件, C=2 检查所有文件)
ifeq ("$(origin C)", "command line")
  KBUILD_CHECKSRC = $(C)
endif
ifndef KBUILD_CHECKSRC
  KBUILD_CHECKSRC = 0
endif

PHONY += all
_all: all

# 设置源码树和对象树路径
ifeq ($(KBUILD_SRC),)
        # 在源码树中构建
        srctree := .
else
        ifeq ($(KBUILD_SRC)/,$(dir $(CURDIR)))
                # 在源码树的子目录中构建
                srctree := ..
        else
                srctree := $(KBUILD_SRC)
        endif
endif

objtree		:= .
src		:= $(srctree)
obj		:= $(objtree)

VPATH		:= $(srctree)

export srctree objtree VPATH

# 架构和交叉编译
ARCH		?= $(SUBARCH)
CROSS_COMPILE	?= $(CONFIG_CROSS_COMPILE:"%"=%)

# Kconfig 配置文件
KCONFIG_CONFIG	?= .config
export KCONFIG_CONFIG

# Shell 配置
CONFIG_SHELL := $(shell if [ -x "$$BASH" ]; then echo $$BASH; \
	  else if [ -x /bin/bash ]; then echo /bin/bash; \
	  else echo sh; fi ; fi)

# 主机编译器（用于构建工具）
HOSTCC       = gcc
HOSTCXX      = g++
HOSTCFLAGS   = -Wall -Wmissing-prototypes -Wstrict-prototypes -O2 -fomit-frame-pointer -std=gnu89
HOSTCXXFLAGS = -O2

# 检测 clang
ifeq ($(shell $(HOSTCC) -v 2>&1 | grep -c "clang version"), 1)
HOSTCFLAGS  += -Wno-unused-value -Wno-unused-parameter \
		-Wno-missing-field-initializers -fno-delete-null-pointer-checks
endif

# 构建模式
KBUILD_BUILTIN := 1
export KBUILD_BUILTIN
export KBUILD_CHECKSRC KBUILD_SRC

# 包含通用定义
scripts/Kbuild.include: ;
include scripts/Kbuild.include

# =============================================================================
# 工具链配置
# =============================================================================
AS		= $(CROSS_COMPILE)as
LD		= $(CROSS_COMPILE)ld
CC		= $(CROSS_COMPILE)gcc
CPP		= $(CC) -E
AR		= $(CROSS_COMPILE)ar
NM		= $(CROSS_COMPILE)nm
STRIP		= $(CROSS_COMPILE)strip
OBJCOPY		= $(CROSS_COMPILE)objcopy
OBJDUMP		= $(CROSS_COMPILE)objdump
AWK		= awk
PERL		= perl
PYTHON		= python
CHECK		= sparse

CHECKFLAGS     := -D__linux__ -Dlinux -D__STDC__ -Dunix -D__unix__ \
		  -Wbitwise -Wno-return-void $(CF)
NOSTDINC_FLAGS  =
CFLAGS_KERNEL	=
AFLAGS_KERNEL	=

# 头文件包含路径（兼容 O= 选项）
EMSINCLUDE    := \
		$(if $(KBUILD_SRC), -I$(srctree)/include) \
		-Iinclude -include include/generated/autoconf.h

KBUILD_CPPFLAGS := -D__EMS__

KBUILD_CFLAGS   := -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs \
		   -fno-strict-aliasing -fno-common \
		   -Werror-implicit-function-declaration \
		   -Wno-format-security \
		   -std=gnu89

KBUILD_AFLAGS_KERNEL :=
KBUILD_CFLAGS_KERNEL :=
KBUILD_AFLAGS   := -D__ASSEMBLY__

KERNELVERSION = $(VERSION)$(if $(PATCHLEVEL),.$(PATCHLEVEL)$(if $(SUBLEVEL),.$(SUBLEVEL)))$(EXTRAVERSION)

export VERSION PATCHLEVEL SUBLEVEL KERNELVERSION
export ARCH CONFIG_SHELL HOSTCC HOSTCFLAGS CROSS_COMPILE AS LD CC
export CPP AR NM STRIP OBJCOPY OBJDUMP
export MAKE AWK PERL PYTHON
export HOSTCXX HOSTCXXFLAGS CHECK CHECKFLAGS

export KBUILD_CPPFLAGS NOSTDINC_FLAGS EMSINCLUDE OBJCOPYFLAGS LDFLAGS
export KBUILD_CFLAGS CFLAGS_KERNEL
export KBUILD_AFLAGS AFLAGS_KERNEL
export KBUILD_AFLAGS_KERNEL KBUILD_CFLAGS_KERNEL
export KBUILD_ARFLAGS

# 忽略的文件/目录（用于 find 等命令）
export RCS_FIND_IGNORE := \( -name SCCS -o -name BitKeeper -o -name .svn -o    \
			  -name CVS -o -name .pc -o -name .hg -o -name .git \) \
			  -prune -o
export RCS_TAR_IGNORE := --exclude SCCS --exclude BitKeeper --exclude .svn \
			 --exclude CVS --exclude .pc --exclude .hg --exclude .git

# =============================================================================
# 输出目录配置
# =============================================================================
BIN_DIR := $(objtree)/bin
LIB_DIR := $(objtree)/lib
KO_DIR  := $(objtree)/ko

export BIN_DIR LIB_DIR KO_DIR

# =============================================================================
# 基础工具构建
# =============================================================================
PHONY += scripts_basic
scripts_basic:
	$(Q)$(MAKE) $(build)=scripts/basic

scripts/basic/%: scripts_basic ;

PHONY += outputmakefile
# 在输出目录生成 Makefile（用于方便在输出目录执行 make）
outputmakefile:
ifneq ($(KBUILD_SRC),)
	$(Q)ln -fsn $(srctree) source
	$(Q)$(CONFIG_SHELL) $(srctree)/scripts/mkmakefile \
	    $(srctree) $(objtree) $(VERSION) $(PATCHLEVEL)
endif

# =============================================================================
# Kconfig 配置目标
# =============================================================================

version_h := include/generated/version.h

# 不需要 .config 的目标
no-dot-config-targets := clean mrproper distclean \
			 cscope help% %docs check% coccicheck \
			 $(version_h) headers_% archheaders archscripts \
			 kernelversion %src-pkg

config-targets := 0
mixed-targets  := 0
dot-config     := 1

ifneq ($(filter $(no-dot-config-targets), $(MAKECMDGOALS)),)
	ifeq ($(filter-out $(no-dot-config-targets), $(MAKECMDGOALS)),)
		dot-config := 0
	endif
endif

ifneq ($(filter config %config,$(MAKECMDGOALS)),)
        config-targets := 1
        ifneq ($(words $(MAKECMDGOALS)),1)
                mixed-targets := 1
        endif
endif

# install 也需要单独处理
ifneq ($(filter install,$(MAKECMDGOALS)),)
    mixed-targets := 1
endif

# 混合目标处理（逐个执行）
ifeq ($(mixed-targets),1)
PHONY += $(MAKECMDGOALS) __build_one_by_one

$(filter-out __build_one_by_one, $(MAKECMDGOALS)): __build_one_by_one
	@:

__build_one_by_one:
	$(Q)set -e; \
	for i in $(MAKECMDGOALS); do \
		$(MAKE) -f $(srctree)/Makefile $$i; \
	done

else
ifeq ($(config-targets),1)
# =============================================================================
# *config 目标 - 配置系统
# =============================================================================

-include arch/$(ARCH)/Makefile
export KBUILD_DEFCONFIG KBUILD_KCONFIG

config: scripts_basic outputmakefile FORCE
	$(Q)$(MAKE) $(build)=scripts/kconfig $@

%config: scripts_basic outputmakefile FORCE
	$(Q)$(MAKE) $(build)=scripts/kconfig $@

else
# =============================================================================
# 构建目标
# =============================================================================

# 构建脚本
PHONY += scripts
scripts: scripts_basic include/config/auto.conf include/config/tristate.conf
	$(Q)$(MAKE) $(build)=$(@)

ifeq ($(dot-config),1)
# 读取配置
-include include/config/auto.conf

# 读取 Kconfig 依赖
-include include/config/auto.conf.cmd

# 避免隐式规则
$(KCONFIG_CONFIG) include/config/auto.conf.cmd: ;

# 如果 .config 比 auto.conf 新，需要重新运行 oldconfig
## 注释掉以便在没有 Kconfig 工具时也能构建
## include/config/%.conf: $(KCONFIG_CONFIG) include/config/auto.conf.cmd
#	$(Q)$(MAKE) -f $(srctree)/Makefile silentoldconfig

else
# 虚拟目标
include/config/auto.conf: ;
endif # $(dot-config)

# =============================================================================
# 默认目标：all
# =============================================================================
all: ems

# 架构特定配置
ARCH_CPPFLAGS :=
ARCH_AFLAGS :=
ARCH_CFLAGS :=
-include arch/$(ARCH)/Makefile

# 编译器选项
KBUILD_CFLAGS	+= $(call cc-option,-fno-delete-null-pointer-checks,)

ifdef CONFIG_CC_OPTIMIZE_FOR_SIZE
KBUILD_CFLAGS	+= -Os $(call cc-disable-warning,maybe-uninitialized,)
else
KBUILD_CFLAGS   += -O2
endif

KBUILD_CFLAGS	+= $(call cc-option,--param=allow-store-data-races=0)

# Clang 特定选项
ifeq ($(cc-name),clang)
KBUILD_CPPFLAGS += $(call cc-option,-Qunused-arguments,)
KBUILD_CPPFLAGS += $(call cc-option,-Wno-unknown-warning-option,)
KBUILD_CFLAGS += $(call cc-disable-warning, unused-variable)
KBUILD_CFLAGS += $(call cc-disable-warning, format-invalid-specifier)
KBUILD_CFLAGS += $(call cc-disable-warning, gnu)
KBUILD_CFLAGS += $(call cc-disable-warning, tautological-compare)
KBUILD_CFLAGS += $(call cc-option, -mno-global-merge,)
KBUILD_CFLAGS += $(call cc-option, -fcatch-undefined-behavior)
else
KBUILD_CFLAGS += $(call cc-disable-warning, unused-but-set-variable)
KBUILD_CFLAGS += $(call cc-disable-warning, unused-const-variable)
endif

# 帧指针
ifdef CONFIG_FRAME_POINTER
KBUILD_CFLAGS	+= -fno-omit-frame-pointer -fno-optimize-sibling-calls
else
KBUILD_CFLAGS	+= -fomit-frame-pointer
endif

KBUILD_CFLAGS   += $(call cc-option, -fno-var-tracking-assignments)

# 调试信息
ifdef CONFIG_DEBUG_INFO
ifdef CONFIG_DEBUG_INFO_SPLIT
KBUILD_CFLAGS   += $(call cc-option, -gsplit-dwarf, -g)
else
KBUILD_CFLAGS	+= -g
endif
KBUILD_AFLAGS	+= -Wa,-gdwarf-2
endif

ifdef CONFIG_DEBUG_INFO_DWARF4
KBUILD_CFLAGS	+= $(call cc-option, -gdwarf-4,)
endif

ifdef CONFIG_DEBUG_INFO_REDUCED
KBUILD_CFLAGS 	+= $(call cc-option, -femit-struct-debug-baseonly) \
		   $(call cc-option,-fno-var-tracking)
endif

# 警告选项
KBUILD_CFLAGS += $(call cc-option,-Wdeclaration-after-statement,)
KBUILD_CFLAGS += $(call cc-disable-warning, pointer-sign)
KBUILD_CFLAGS	+= $(call cc-option,-fno-strict-overflow)
KBUILD_CFLAGS   += $(call cc-option,-fconserve-stack)
KBUILD_CFLAGS   += $(call cc-option,-Werror=implicit-int)
KBUILD_CFLAGS   += $(call cc-option,-Werror=strict-prototypes)
KBUILD_CFLAGS   += $(call cc-option,-Werror=date-time)
KBUILD_CFLAGS   += $(call cc-option,-Werror=incompatible-pointer-types)

# AR 确定性模式
KBUILD_ARFLAGS := $(call ar-option,D)

# 检查 asm goto 支持
ifeq ($(shell $(CONFIG_SHELL) $(srctree)/scripts/gcc-goto.sh $(CC)), y)
	KBUILD_CFLAGS += -DCC_HAVE_ASM_GOTO
	KBUILD_AFLAGS += -DCC_HAVE_ASM_GOTO
endif

include scripts/Makefile.extrawarn

# 添加架构和用户自定义标志
KBUILD_CPPFLAGS += $(ARCH_CPPFLAGS) $(KCPPFLAGS)
KBUILD_AFLAGS   += $(ARCH_AFLAGS)   $(KAFLAGS)
KBUILD_CFLAGS   += $(ARCH_CFLAGS)   $(KCFLAGS)

# 默认镜像
export KBUILD_IMAGE ?= ems

# 安装路径
export	INSTALL_PATH ?= ./install

# =============================================================================
# EMS 构建目标
# =============================================================================

# 核心模块和产品模块
core-y		:= core/
products-y	:= products/

# 所有需要构建的目录
ems-dirs	:= $(core-y) $(products-y)

# 创建输出目录
$(shell mkdir -p $(BIN_DIR) $(LIB_DIR) $(KO_DIR))

# EMS 主目标（不链接，只构建所有模块）
PHONY += ems
ems: $(ems-dirs) FORCE
	@echo "  BUILD   EMS $(KERNELVERSION)"

# 下降到子目录构建
PHONY += $(ems-dirs)
$(ems-dirs): prepare scripts
	$(Q)$(MAKE) $(build)=$@

# =============================================================================
# 准备工作
# =============================================================================

PHONY += prepare prepare0 prepare1 prepare2 prepare3

# prepare3: 检查是否在源码树中有残留文件
prepare3:
ifneq ($(KBUILD_SRC),)
	@$(kecho) '  Using $(srctree) as source for EMS'
	$(Q)if [ -f $(srctree)/.config -o -d $(srctree)/include/config ]; then \
		echo >&2 "  $(srctree) is not clean, please run 'make mrproper'"; \
		echo >&2 "  in the '$(srctree)' directory.";\
		/bin/false; \
	fi;
endif

prepare2: prepare3 outputmakefile

prepare1: prepare2 $(version_h) include/config/auto.conf

archprepare:

prepare0: prepare1 scripts_basic
	$(Q)$(MAKE) $(build)=.

prepare: prepare0

# 生成版本头文件
define filechk_version.h
	(echo \#define EMS_VERSION_CODE $(shell                         \
	expr $(VERSION) \* 65536 + 0$(PATCHLEVEL) \* 256 + 0$(SUBLEVEL)); \
	echo '#define EMS_VERSION(a,b,c) (((a) << 16) + ((b) << 8) + (c))';)
endef

$(version_h): $(srctree)/Makefile FORCE
	$(call filechk,version.h)

# =============================================================================
# 清理目标
# =============================================================================

CLEAN_DIRS  += $(BIN_DIR) $(LIB_DIR) $(KO_DIR)
CLEAN_FILES +=

MRPROPER_DIRS  += include/config include/generated .tmp_objdiff
MRPROPER_FILES += .config .config.old .version .old_version \
		  cscope* GPATH GSYMS GTAGS TAGS

# clean - 删除大部分生成文件
clean: rm-dirs  := $(CLEAN_DIRS)
clean: rm-files := $(CLEAN_FILES)
clean-dirs      := $(addprefix _clean_, . $(ems-dirs))

PHONY += $(clean-dirs) clean archclean
$(clean-dirs):
	$(Q)$(MAKE) $(clean)=$(patsubst _clean_%,%,$@)

clean: $(clean-dirs)
	$(call cmd,rmdirs)
	$(call cmd,rmfiles)
	@find . $(RCS_FIND_IGNORE) \
		\( -name '*.[oas]' -o -name '.*.cmd' \
		-o -name '.*.d' -o -name '.*.tmp' \
		-o -name '.tmp_*.o.*' \
		-o -name '*.so' -o -name '*.so.*' \
		-o -name '*.a' \
		-o -name '*.gcno' \) -type f -print | xargs rm -f

# mrproper - 删除所有生成文件，包括 .config
mrproper: rm-dirs  := $(wildcard $(MRPROPER_DIRS))
mrproper: rm-files := $(wildcard $(MRPROPER_FILES))
mrproper-dirs      := $(addprefix _mrproper_, scripts)

PHONY += $(mrproper-dirs) mrproper
$(mrproper-dirs):
	$(Q)$(MAKE) $(clean)=$(patsubst _mrproper_%,%,$@)

mrproper: clean $(mrproper-dirs)
	$(call cmd,rmdirs)
	$(call cmd,rmfiles)

# distclean
PHONY += distclean

distclean: mrproper
	@find $(srctree) $(RCS_FIND_IGNORE) \
		\( -name '*.orig' -o -name '*.rej' -o -name '*~' \
		-o -name '*.bak' -o -name '#*#' -o -name '.*.orig' \
		-o -name '.*.rej' -o -name '*%'  -o -name 'core' \) \
		-type f -print | xargs rm -f

# clean 的简写
clean := -f $(if $(KBUILD_SRC),$(srctree)/)scripts/Makefile.clean obj

# =============================================================================
# 帮助信息
# =============================================================================
PHONY += help
help:
	@echo  'Cleaning targets:'
	@echo  '  clean		  - Remove most generated files but keep the config'
	@echo  '  mrproper	  - Remove all generated files + config + various backup files'
	@echo  '  distclean	  - mrproper + remove editor backup and patch files'
	@echo  ''
	@echo  'Configuration targets:'
	@$(MAKE) -f $(srctree)/scripts/kconfig/Makefile help
	@echo  ''
	@echo  'Build targets:'
	@echo  '  all		  - Build all targets marked with [*]'
	@echo  '* ems		  - Build the EMS framework'
	@echo  '  core		  - Build core modules only'
	@echo  '  products	  - Build product modules only'
	@echo  ''
	@echo  'Other generic targets:'
	@echo  '  dir/            - Build all files in dir and below'
	@echo  '  dir/file.[ois]  - Build specified target only'
	@echo  ''
	@echo  '  make V=0|1 [targets] 0 => quiet build (default), 1 => verbose build'
	@echo  '  make V=2   [targets] 2 => give reason for rebuild of target'
	@echo  '  make O=dir [targets] Locate all output files in "dir", including .config'
	@echo  '  make C=1   [targets] Check all c source with $$CHECK (sparse by default)'
	@echo  '  make C=2   [targets] Force check of all c source with $$CHECK'
	@echo  ''
	@echo  'Execute "make" or "make all" to build all targets marked with [*] '

# 单独构建 core 或 products
PHONY += core products
core: prepare scripts
	$(Q)$(MAKE) $(build)=core

products: prepare scripts
	$(Q)$(MAKE) $(build)=products

endif #ifeq ($(config-targets),1)
endif #ifeq ($(mixed-targets),1)

PHONY += kernelversion

kernelversion:
	@echo $(KERNELVERSION)

# =============================================================================
# 单个目标构建支持
# =============================================================================

build-dir  = $(patsubst %/,%,$(dir $@))
target-dir = $(dir $@)

%.s: %.c prepare scripts FORCE
	$(Q)$(MAKE) $(build)=$(build-dir) $(target-dir)$(notdir $@)
%.i: %.c prepare scripts FORCE
	$(Q)$(MAKE) $(build)=$(build-dir) $(target-dir)$(notdir $@)
%.o: %.c prepare scripts FORCE
	$(Q)$(MAKE) $(build)=$(build-dir) $(target-dir)$(notdir $@)
%.lst: %.c prepare scripts FORCE
	$(Q)$(MAKE) $(build)=$(build-dir) $(target-dir)$(notdir $@)
%.s: %.S prepare scripts FORCE
	$(Q)$(MAKE) $(build)=$(build-dir) $(target-dir)$(notdir $@)
%.o: %.S prepare scripts FORCE
	$(Q)$(MAKE) $(build)=$(build-dir) $(target-dir)$(notdir $@)

# =============================================================================
# 辅助命令
# =============================================================================

quiet_cmd_rmdirs = $(if $(wildcard $(rm-dirs)),CLEAN   $(wildcard $(rm-dirs)))
      cmd_rmdirs = rm -rf $(rm-dirs)

quiet_cmd_rmfiles = $(if $(wildcard $(rm-files)),CLEAN   $(wildcard $(rm-files)))
      cmd_rmfiles = rm -f $(rm-files)

# 读取所有保存的命令行
targets := $(wildcard $(sort $(targets)))
cmd_files := $(wildcard .*.cmd $(foreach f,$(targets),$(dir $(f)).$(notdir $(f)).cmd))

ifneq ($(cmd_files),)
  $(cmd_files): ;	# 不尝试更新依赖文件
  include $(cmd_files)
endif

endif	# skip-makefile

PHONY += FORCE
FORCE:

# 声明 .PHONY 变量的内容为伪目标
.PHONY: $(PHONY)

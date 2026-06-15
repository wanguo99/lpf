# SPDX-License-Identifier: GPL-2.0-only
###
# Makefile.basic lists the most basic programs used during the build process.
# The programs listed herein are what are needed to do the basic stuff,
# such as fix file dependencies.
# This initial step is needed to avoid files to be recompiled
# when kernel configuration changes (which is what happens when
# .config is included by main Makefile.
# ---------------------------------------------------------------------------
# fixdep: 	 Used to generate dependency information during build process

hostprogs	:= fixdep
always-y	:= $(hostprogs)

# fixdep is needed to compile other host programs
$(obj)/%.o: $(obj)/fixdep FORCE
	$(call if_changed_dep,host_cc_o_c)


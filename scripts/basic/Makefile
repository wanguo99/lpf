# =============================================================================
# scripts/basic Makefile
# =============================================================================

hostprogs-y	:= fixdep

# fixdep is needed to compute auto.conf.cmd = dependency file
$(addprefix $(obj)/,$(hostprogs-y)): $(obj)/%: $(src)/%.c
	$(HOSTCC) $(HOSTCFLAGS) -o $@ $<

# Always build fixdep
.PHONY: $(hostprogs-y)

# LPF Refactor Roadmap

## Current Direction

The current architecture is broadly aligned with the long-term plan. The next
work should reduce transitional coupling and add validation rather than change
the target direction.

## Near-Term Priorities

1. Keep LPF Core as the primary device model.
2. Reduce service-local fixed registries where they duplicate LPF Core state.
3. Keep peripheral services dependent on LPF HW and transport APIs only.
4. Expand runtime config backend coverage and tests.
5. Add explicit kernel compatibility policy and feature detection.
6. Expand mock and integration validation.

## Refactor Batches

### Batch 1: Baseline And Registry Cleanup

- Add target architecture and module boundary documents.
- Convert MCU and LED service context registries from fixed arrays to dynamic
  service-owned registries.
- Keep existing `LPF_*_MAX_DEVICES` node limits until the UAPI and node model are
  deliberately changed.
- Update service documentation to explain the transitional distinction between
  service context storage and instance node limits.

### Batch 2: Configuration Coverage

- [x] Add static MCU/LED sample entries for the mock build preset.
- [ ] Add tests proving the same normalized device model is produced from static and
  Device Tree backends.
- [ ] Add a board-profile backend only if there is a real product-line selection
  need that static identity selectors cannot cover.

### Batch 3: Compat Policy

- Add a compatibility policy document.
- Introduce explicit feature-detection helpers for supported kernels.
- Move procfs/debugfs/sysfs API differences behind compat wrappers where needed.
- Validate the selected kernel version targets in CI or a documented local
  matrix.

### Batch 4: Lifecycle And State Hardening

- Define the final userspace event delivery decision.
- Deepen peripheral service state and recovery models.
- Ensure errors consistently update Core device state, per-instance sysfs, and
  discovery snapshots.

### Batch 5: Test System Expansion

- Add dummy peripheral service tests.
- Add mock MCU transport tests.
- Add module-load automation for mock SoC and LPF HW selftests.
- Add ABI and PDI coverage for new peripherals as they are introduced.

## Guardrails

- Do not split MCU and LED into separate kernel modules unless the runtime
  boundary is intentionally redesigned.
- Do not move product-specific behavior into shared framework directories.
- Do not expose kernel-internal service or HW headers to userspace.
- Do not add SoC-specific calls above the SoC adapter layer.

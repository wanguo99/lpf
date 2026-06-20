# LPF Event Delivery Policy

## Decision

LPF v1 does not expose an asynchronous userspace event stream.

The LPF Core event notifier remains a kernel-only mechanism for in-kernel
framework and service coordination. Userspace observes LPF device lifecycle and
health through synchronous snapshots:

- `/dev/pdm_ctl` discovery ioctls for device list, state, errors, and
  capabilities.
- Per-instance sysfs attributes for read-only inspection.
- Read-only procfs service snapshots where available.

Applications that need change detection should poll these snapshot interfaces at
an application-defined cadence.

## Rationale

The current ABI surface already exposes the complete management state needed by
userspace: stable name, type, index, state, last error, error count, driver
name, and capabilities. Adding an event stream now would require extra ABI
decisions that are not yet proven by product requirements, including queue
depth, overflow behavior, replay semantics after open, per-device filtering,
poll/select readiness, and event payload versioning.

Keeping event delivery kernel-only avoids committing those semantics too early
while preserving the existing in-kernel notifier for future services.

## ABI Rules

- Do not add event ioctls, blocking reads, mmap queues, signal delivery,
  netlink, or eventfd plumbing to `/dev/pdm_ctl` in LPF ABI v1.
- Do not add PDI event subscription APIs that imply asynchronous kernel event
  delivery.
- Keep `/dev/pdm_ctl` as a synchronous snapshot and lookup ABI.
- If a future product needs userspace events, define a versioned event ABI in a
  separate plan that specifies buffering, overflow, filtering, replay, and
  compatibility behavior before adding code.

## Current User Workflow

1. Open `/dev/pdm_ctl` through PDI or directly through the control UAPI.
2. Read `lpf_ctl_info` to get the current device count.
3. List or query `lpf_ctl_device_info` snapshots.
4. Poll again when the application needs to detect lifecycle or health changes.

The kernel may emit internal LPF device events for registration, binding, state
changes, errors, removal start, and removal completion, but those events are not
part of the userspace ABI.

# Kernel OSAL

Kernel OSAL wraps Linux kernel APIs needed by kernel HAL, PConfig, and PDM.

It is intentionally separate from `user/osal`, but public OSAL names should
match userspace where the kernel can provide equivalent or close semantics. This
keeps shared middleware code largely unaware of whether it is built for
userspace or kernel modules.

The aggregation entry is `kernel/include/osal/osal.h`. It includes the
kernel-backed OSAL subsets currently available:

- `lib/osal_heap.h`: allocation, free, realloc, and allocation statistics.
- `lib/osal_errno.h`: errno-compatible status codes and status name helpers.
- `lib/osal_string.h`: memory, string, formatting, and basic conversion APIs.
- `ipc/osal_mutex.h`: mutex and mutex attribute wrappers.
- `ipc/osal_semaphore.h`: in-kernel semaphore wrappers.
- `ipc/osal_cond.h`: wait-queue-backed condition-style wait wrappers.
- `ipc/osal_rwlock.h`: read/write lock wrappers.
- `ipc/osal_atomic.h`: 32-bit, 64-bit, and boolean atomics.
- `sys/osal_time.h`: sleep and monotonic/boottime/high-resolution time APIs.
- `sys/osal_thread.h`: kthread-backed create, join, detach, cancel, and
  attribute APIs.
- `sys/osal_usercopy.h`: safe copy helpers for ioctl and character-device
  user buffers.
- `util/osal_log.h`: LOG_* macros and logging control functions backed by
  printk.
- `util/osal_crc.h`: CRC16-CCITT, CRC16-MODBUS, and CRC32 helpers.

Interfaces that have no safe kernel equivalent are intentionally omitted from
the kernel header set or return `OSAL_ERR_NOT_SUPPORTED`; do not tunnel back to
userspace. This currently includes process, environment, signal, POSIX shared
memory, pty/terminal, select/poll, generic file, and generic socket APIs.

HAL-owned Linux subsystem details stay in HAL instead of being hidden behind
generic OSAL wrappers. Current examples are SocketCAN access in HAL CAN and TTY
file operations in HAL serial.

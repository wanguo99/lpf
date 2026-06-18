# PDI

PDI is the userspace API library for the kernel PDM module.

Responsibilities:

- Own the application-facing C API.
- Open and close the PDM device node.
- Marshal requests through UAPI ioctl commands.
- Hide ioctl details from applications.

PDI must not reimplement kernel HAL, PConfig, or PDM business logic.

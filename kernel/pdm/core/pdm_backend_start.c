// SPDX-License-Identifier: GPL-2.0

/* Delimit pdm_backend_entries inside pdm.ko without a linker script. */
asm(
".section pdm_backend_entries,\"a\"\n"
".balign 8\n"
".globl __start_pdm_backend_entries\n"
"__start_pdm_backend_entries:\n"
".previous\n"
);

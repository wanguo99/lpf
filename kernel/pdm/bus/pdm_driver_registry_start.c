// SPDX-License-Identifier: GPL-2.0

/* Delimit pdm_driver_entries inside pdm.ko without a linker script. */
asm(
".section pdm_driver_entries,\"a\"\n"
".balign 8\n"
".globl __start_pdm_driver_entries\n"
"__start_pdm_driver_entries:\n"
".previous\n"
);

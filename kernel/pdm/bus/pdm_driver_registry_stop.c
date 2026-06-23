// SPDX-License-Identifier: GPL-2.0

/* Keep this object after all pdm_driver_register() users in pdm-y. */
asm(
".section pdm_driver_entries,\"a\"\n"
".balign 8\n"
".globl __stop_pdm_driver_entries\n"
"__stop_pdm_driver_entries:\n"
".previous\n"
);

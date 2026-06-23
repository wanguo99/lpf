// SPDX-License-Identifier: GPL-2.0

/* Keep this object after all pdm_backend_register() users in pdm-y. */
asm(
".section pdm_backend_entries,\"a\"\n"
".balign 8\n"
".globl __stop_pdm_backend_entries\n"
"__stop_pdm_backend_entries:\n"
".previous\n"
);

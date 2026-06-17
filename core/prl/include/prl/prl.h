/**
 * @file prl.h
 * @brief PRL Protocol Layer - Unified API Entry Point
 */

#ifndef PRL_H
#define PRL_H

/* Core protocol definitions - always required */
#include "prl_common.h"
#include "prl_device.h"

/* Device-specific protocols - conditional compilation */
#ifdef CONFIG_PRL_MCU
#include "prl_mcu.h"
#endif /* CONFIG_PRL_MCU */

#ifdef CONFIG_PRL_PMC
#include "prl_pmc.h"
#endif /* CONFIG_PRL_PMC */

#ifdef CONFIG_PRL_PMC
#include "prl_pmc.h"
#endif /* CONFIG_PRL_PMC */

#ifdef CONFIG_PRL_GSC
#include "prl_gsc.h"
#endif /* CONFIG_PRL_GSC */

#ifdef CONFIG_PRL_POWER
#include "prl_power.h"
#endif /* CONFIG_PRL_POWER */

#endif /* PRL_H */

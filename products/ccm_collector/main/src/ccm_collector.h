#ifndef PMC_COLLECTOR_H
#define PMC_COLLECTOR_H

#include "osal/osal.h"

/* Collector进程初始化 */
int32_t PMC_Collector_Init(void);

/* Collector进程运行 */
int32_t PMC_Collector_Run(void);

/* Collector进程清理 */
void PMC_Collector_Cleanup(void);

#endif /* PMC_COLLECTOR_H */

#ifndef CCM_COLLECTOR_H
#define CCM_COLLECTOR_H

#include "osal.h"

/* Collector进程初始化 */
int32_t CCM_Collector_Init(void);

/* Collector进程运行 */
int32_t CCM_Collector_Run(void);

/* Collector进程清理 */
void CCM_Collector_Cleanup(void);

#endif /* CCM_COLLECTOR_H */

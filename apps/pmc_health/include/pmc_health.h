#ifndef PMC_HEALTH_H
#define PMC_HEALTH_H

#include "osal.h"

/* Health进程初始化 */
int32 PMC_Health_Init(void);

/* Health进程运行 */
int32 PMC_Health_Run(void);

/* Health进程清理 */
void PMC_Health_Cleanup(void);

#endif /* PMC_HEALTH_H */

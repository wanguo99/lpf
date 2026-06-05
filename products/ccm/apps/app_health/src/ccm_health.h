#ifndef CCM_HEALTH_H
#define CCM_HEALTH_H

#include "osal.h"

/* Health进程初始化 */
int32_t CCM_Health_Init(void);

/* Health进程运行 */
int32_t CCM_Health_Run(void);

/* Health进程清理 */
void CCM_Health_Cleanup(void);

#endif /* CCM_HEALTH_H */

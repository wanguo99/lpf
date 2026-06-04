#ifndef CCM_SUPERVISOR_H
#define CCM_SUPERVISOR_H

#include "osal/osal.h"

/* Supervisor进程初始化 */
int32_t CCM_Supervisor_Init(void);

/* Supervisor进程运行 */
int32_t CCM_Supervisor_Run(void);

/* Supervisor进程清理 */
void CCM_Supervisor_Cleanup(void);

#endif /* CCM_SUPERVISOR_H */

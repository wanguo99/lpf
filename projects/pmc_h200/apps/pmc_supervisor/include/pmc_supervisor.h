#ifndef PMC_SUPERVISOR_H
#define PMC_SUPERVISOR_H

#include "osal.h"

/* Supervisor进程初始化 */
int32 PMC_Supervisor_Init(void);

/* Supervisor进程运行 */
int32 PMC_Supervisor_Run(void);

/* Supervisor进程清理 */
void PMC_Supervisor_Cleanup(void);

#endif /* PMC_SUPERVISOR_H */

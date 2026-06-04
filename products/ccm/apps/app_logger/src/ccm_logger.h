#ifndef CCM_LOGGER_H
#define CCM_LOGGER_H

#include "osal/osal.h"

/* Logger进程初始化 */
int32_t CCM_Logger_Init(void);

/* Logger进程运行 */
int32_t CCM_Logger_Run(void);

/* Logger进程清理 */
void CCM_Logger_Cleanup(void);

#endif /* CCM_LOGGER_H */

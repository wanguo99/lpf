#ifndef PMC_LOGGER_H
#define PMC_LOGGER_H

#include "osal.h"

/* Logger进程初始化 */
int32_t PMC_Logger_Init(void);

/* Logger进程运行 */
int32_t PMC_Logger_Run(void);

/* Logger进程清理 */
void PMC_Logger_Cleanup(void);

#endif /* PMC_LOGGER_H */

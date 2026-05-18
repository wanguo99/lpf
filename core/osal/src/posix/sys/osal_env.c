/************************************************************************
 * OSAL POSIX实现 - 环境变量操作
 ************************************************************************/

#include "sys/osal_env.h"
#include <stdlib.h>

char *OSAL_getenv(const char *name)
{
    char *result = getenv(name);
    return result;
}

int32_t OSAL_setenv(const char *name, const char *value, int32_t overwrite)
{
    int32_t result = setenv(name, value, overwrite);
    return result;
}

int32_t OSAL_unsetenv(const char *name)
{
    int32_t result = unsetenv(name);
    return result;
}

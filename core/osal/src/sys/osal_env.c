/************************************************************************
 * OSAL POSIX实现 - 环境变量操作
 ************************************************************************/

#include "osal.h"
#include <stdlib.h>

char *osal_getenv(const char *name)
{
	char *result = getenv(name);
	return result;
}

int32_t osal_setenv(const char *name, const char *value, int32_t overwrite)
{
	int32_t result = setenv(name, value, overwrite);
	return result;
}

int32_t osal_unsetenv(const char *name)
{
	int32_t result = unsetenv(name);
	return result;
}

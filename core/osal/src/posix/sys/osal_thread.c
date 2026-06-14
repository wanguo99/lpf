/************************************************************************
 * OSAL - POSIX线程薄封装实现
 ************************************************************************/

#include "osal.h"
#include <pthread.h>
#include <errno.h>

int32_t OSAL_pthread_create(pthread_t *thread,
                            const pthread_attr_t *attr,
                            void *(*start_routine)(void *),
                            void *arg)
{
    if (thread == NULL || start_routine == NULL) {
        errno = EINVAL;
        return -1;
    }

    return pthread_create(thread, attr, start_routine, arg);
}

int32_t OSAL_pthread_join(pthread_t thread, void **retval)
{
    return pthread_join(thread, retval);
}

int32_t OSAL_pthread_detach(pthread_t thread)
{
    return pthread_detach(thread);
}

pthread_t OSAL_pthread_self(void)
{
    return pthread_self();
}

void OSAL_pthread_exit(void *retval)
{
    pthread_exit(retval);
}

int32_t OSAL_pthread_cancel(pthread_t thread)
{
    return pthread_cancel(thread);
}

/*===========================================================================
 * 线程属性管理
 *===========================================================================*/

int32_t OSAL_pthread_attr_init(pthread_attr_t *attr)
{
    if (attr == NULL) {
        errno = EINVAL;
        return -1;
    }

    return pthread_attr_init(attr);
}

int32_t OSAL_pthread_attr_destroy(pthread_attr_t *attr)
{
    if (attr == NULL) {
        errno = EINVAL;
        return -1;
    }

    return pthread_attr_destroy(attr);
}

int32_t OSAL_pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize)
{
    if (attr == NULL) {
        errno = EINVAL;
        return -1;
    }

    return pthread_attr_setstacksize(attr, stacksize);
}

int32_t OSAL_pthread_attr_getstacksize(const pthread_attr_t *attr, size_t *stacksize)
{
    if (attr == NULL || stacksize == NULL) {
        errno = EINVAL;
        return -1;
    }

    return pthread_attr_getstacksize(attr, stacksize);
}

int32_t OSAL_pthread_attr_setdetachstate(pthread_attr_t *attr, int32_t detachstate)
{
    if (attr == NULL) {
        errno = EINVAL;
        return -1;
    }

    return pthread_attr_setdetachstate(attr, detachstate);
}

int32_t OSAL_pthread_attr_getdetachstate(const pthread_attr_t *attr, int32_t *detachstate)
{
    if (attr == NULL || detachstate == NULL) {
        errno = EINVAL;
        return -1;
    }

    return pthread_attr_getdetachstate(attr, detachstate);
}

int32_t OSAL_pthread_attr_setschedpolicy(pthread_attr_t *attr, int32_t policy)
{
    if (attr == NULL) {
        errno = EINVAL;
        return -1;
    }

    return pthread_attr_setschedpolicy(attr, policy);
}

int32_t OSAL_pthread_attr_setschedparam(pthread_attr_t *attr, const struct sched_param *param)
{
    if (attr == NULL || param == NULL) {
        errno = EINVAL;
        return -1;
    }

    return pthread_attr_setschedparam(attr, param);
}

/************************************************************************
 * OSAL - POSIX线程封装实现
 ************************************************************************/

#include <osal.h>
#include <pthread.h>
#include <sched.h>

int32_t OSAL_pthread_create(osal_thread_t *thread,
                            void *attr,
                            osal_thread_func_t start_routine,
                            void *arg)
{
    pthread_t pt;
    int32_t ret;
    union {
        void *osal_attr;
        pthread_attr_t *posix_attr;
    } attr_union;
    union {
        pthread_t posix_thread;
        osal_thread_t osal_thread;
    } thread_union;

    attr_union.osal_attr = attr;

    ret = pthread_create(&pt, attr_union.posix_attr, start_routine, arg);
    if (0 == ret && NULL != thread) {
        thread_union.posix_thread = pt;
        *thread = thread_union.osal_thread;
    }
    return ret;
}

int32_t OSAL_pthread_join(osal_thread_t thread, void **retval)
{
    union {
        osal_thread_t osal_thread;
        pthread_t posix_thread;
    } thread_union;

    thread_union.osal_thread = thread;
    return pthread_join(thread_union.posix_thread, retval);
}

int32_t OSAL_ThreadCreate(osal_thread_t *thread, osal_thread_func_t func, void *arg)
{
    int32_t ret;

    if (NULL == thread || NULL == func)
        return OSAL_ERR_INVALID_POINTER;

    ret = OSAL_pthread_create(thread, NULL, func, arg);
    if (0 != ret)
        return OSAL_ERR_GENERIC;

    return OSAL_SUCCESS;
}

int32_t OSAL_ThreadCreateEx(osal_thread_t *thread,
                             const osal_thread_attr_t *attr,
                             osal_thread_func_t func,
                             void *arg)
{
    pthread_attr_t pthread_attr;
    int detach_state;
    int inherit;
    struct sched_param param;
    int32_t ret;

    if (NULL == thread || NULL == func)
        return OSAL_ERR_INVALID_POINTER;

    pthread_attr_init(&pthread_attr);

    /* 设置线程属性 */
    if (attr != NULL)
    {
        /* 设置栈大小 */
        if (attr->stack_size > 0)
        {
            pthread_attr_setstacksize(&pthread_attr, attr->stack_size);
        }

        /* 设置分离状态 */
        detach_state = attr->detached ? PTHREAD_CREATE_DETACHED : PTHREAD_CREATE_JOINABLE;
        pthread_attr_setdetachstate(&pthread_attr, detach_state);

        /* 设置调度属性继承 */
        inherit = attr->inherit_sched ? PTHREAD_INHERIT_SCHED : PTHREAD_EXPLICIT_SCHED;
        pthread_attr_setinheritsched(&pthread_attr, inherit);

        /* 如果不继承，则设置调度策略和优先级 */
        if (!attr->inherit_sched)
        {
            pthread_attr_setschedpolicy(&pthread_attr, attr->sched_policy);

            param.sched_priority = attr->sched_priority;
            pthread_attr_setschedparam(&pthread_attr, &param);
        }
    }

    /* 创建线程 */
    ret = OSAL_pthread_create(thread, &pthread_attr, func, arg);
    pthread_attr_destroy(&pthread_attr);

    if (0 != ret)
        return OSAL_ERR_GENERIC;

    return OSAL_SUCCESS;
}

int32_t OSAL_ThreadJoin(osal_thread_t thread)
{
    int32_t ret;

    ret = OSAL_pthread_join(thread, NULL);
    if (0 != ret)
        return OSAL_ERR_GENERIC;

    return OSAL_SUCCESS;
}

osal_thread_t OSAL_ThreadSelf(void)
{
    pthread_t pt = pthread_self();

    union {
        pthread_t posix_thread;
        osal_thread_t osal_thread;
    } thread_union;

    thread_union.posix_thread = pt;
    return thread_union.osal_thread;
}

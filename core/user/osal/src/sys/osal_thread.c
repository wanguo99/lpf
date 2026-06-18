/************************************************************************
 * OSAL POSIX 后端 - 线程
 ************************************************************************/

#include "osal.h"
#include <pthread.h>
#include <errno.h>

int32_t osal_thread_create(osal_thread_t *thread,
						   const osal_thread_attr_t *attr,
						   void *(*start_routine)(void *), void *arg)
{
	if (thread == NULL || start_routine == NULL) {
		errno = EINVAL;
		return -1;
	}

	return pthread_create(thread, attr, start_routine, arg);
}

int32_t osal_thread_join(osal_thread_t thread, void **retval)
{
	return pthread_join(thread, retval);
}

int32_t osal_thread_detach(osal_thread_t thread)
{
	return pthread_detach(thread);
}

bool osal_thread_equal(osal_thread_t thread1, osal_thread_t thread2)
{
	return pthread_equal(thread1, thread2) != 0;
}

osal_thread_t osal_thread_self(void)
{
	return pthread_self();
}

void osal_thread_exit(void *retval)
{
	pthread_exit(retval);
}

int32_t osal_thread_cancel(osal_thread_t thread)
{
	return pthread_cancel(thread);
}

/*===========================================================================
 * 线程属性管理
 *===========================================================================*/

int32_t osal_thread_attr_init(osal_thread_attr_t *attr)
{
	if (attr == NULL) {
		errno = EINVAL;
		return -1;
	}

	return pthread_attr_init(attr);
}

int32_t osal_thread_attr_destroy(osal_thread_attr_t *attr)
{
	if (attr == NULL) {
		errno = EINVAL;
		return -1;
	}

	return pthread_attr_destroy(attr);
}

int32_t osal_thread_attr_set_stack_size(osal_thread_attr_t *attr,
										osal_size_t stacksize)
{
	if (attr == NULL) {
		errno = EINVAL;
		return -1;
	}

	return pthread_attr_setstacksize(attr, stacksize);
}

int32_t osal_thread_attr_get_stack_size(const osal_thread_attr_t *attr,
										osal_size_t *stacksize)
{
	if (attr == NULL || stacksize == NULL) {
		errno = EINVAL;
		return -1;
	}

	return pthread_attr_getstacksize(attr, stacksize);
}

int32_t osal_thread_attr_set_detach_state(osal_thread_attr_t *attr,
										  int32_t detachstate)
{
	if (attr == NULL) {
		errno = EINVAL;
		return -1;
	}

	return pthread_attr_setdetachstate(attr, detachstate);
}

int32_t osal_thread_attr_get_detach_state(const osal_thread_attr_t *attr,
										  int32_t *detachstate)
{
	if (attr == NULL || detachstate == NULL) {
		errno = EINVAL;
		return -1;
	}

	return pthread_attr_getdetachstate(attr, detachstate);
}

int32_t osal_thread_attr_set_sched_policy(osal_thread_attr_t *attr,
										  int32_t policy)
{
	if (attr == NULL) {
		errno = EINVAL;
		return -1;
	}

	return pthread_attr_setschedpolicy(attr, policy);
}

int32_t osal_thread_attr_set_sched_param(osal_thread_attr_t *attr,
										 const osal_sched_param_t *param)
{
	if (attr == NULL || param == NULL) {
		errno = EINVAL;
		return -1;
	}

	return pthread_attr_setschedparam(attr, param);
}

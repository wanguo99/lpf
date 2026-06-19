// SPDX-License-Identifier: GPL-2.0

#include "osal/osal.h"

#include <linux/completion.h>
#include <linux/kthread.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/sched.h>
#include <linux/slab.h>

struct osal_thread_s {
	struct task_struct *task;
	struct list_head node;
	struct completion done;
	struct mutex lock;
	void *(*start_routine)(void *);
	void *arg;
	void *retval;
	bool detached;
	bool joined;
	bool completed;
};

static DEFINE_MUTEX(g_thread_registry_lock);
static LIST_HEAD(g_thread_registry);

static int32_t osal_thread_errno_to_status(int ret)
{
	int err;

	if (ret >= 0)
		return OSAL_SUCCESS;

	err = -ret;
	if (err == ENOMEM)
		return OSAL_ERR_NO_MEMORY;
	if (err == EINVAL)
		return OSAL_ERR_INVALID_PARAM;
	if (err == EBUSY)
		return OSAL_ERR_BUSY;
	if (err == EINTR)
		return OSAL_ERR_INTERRUPTED;

	return OSAL_ERR_GENERIC;
}

static osal_thread_t osal_thread_find_current_locked(void)
{
	osal_thread_t thread;

	list_for_each_entry(thread, &g_thread_registry, node) {
		if (thread->task == current)
			return thread;
	}

	return NULL;
}

static osal_thread_t osal_thread_find_current(void)
{
	osal_thread_t thread;

	mutex_lock(&g_thread_registry_lock);
	thread = osal_thread_find_current_locked();
	mutex_unlock(&g_thread_registry_lock);

	return thread;
}

static void osal_thread_unregister(osal_thread_t thread)
{
	mutex_lock(&g_thread_registry_lock);
	if (!list_empty(&thread->node))
		list_del_init(&thread->node);
	mutex_unlock(&g_thread_registry_lock);
}

static bool osal_thread_complete(osal_thread_t thread, void *retval)
{
	bool free_thread;

	osal_thread_unregister(thread);

	mutex_lock(&thread->lock);
	thread->retval = retval;
	thread->completed = true;
	complete_all(&thread->done);
	free_thread = thread->detached && !thread->joined;
	mutex_unlock(&thread->lock);

	return free_thread;
}

static int osal_thread_trampoline(void *data)
{
	osal_thread_t thread = data;
	void *retval;

	retval = thread->start_routine(thread->arg);

	if (osal_thread_complete(thread, retval))
		kfree(thread);

	return 0;
}

int32_t osal_thread_create(osal_thread_t *thread,
			   const osal_thread_attr_t *attr,
			   void *(*start_routine)(void *), void *arg)
{
	osal_thread_t ctx;
	const char *name = "osal_thread";

	if (!thread || !start_routine)
		return OSAL_ERR_INVALID_POINTER;

	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return OSAL_ERR_NO_MEMORY;

	init_completion(&ctx->done);
	mutex_init(&ctx->lock);
	INIT_LIST_HEAD(&ctx->node);
	ctx->start_routine = start_routine;
	ctx->arg = arg;
	if (attr) {
		ctx->detached =
			(attr->detach_state == OSAL_THREAD_CREATE_DETACHED);
		if (attr->name[0] != '\0')
			name = attr->name;
	}

	ctx->task = kthread_create(osal_thread_trampoline, ctx, "%s", name);
	if (IS_ERR(ctx->task)) {
		int32_t ret = (int32_t)PTR_ERR(ctx->task);

		kfree(ctx);
		return osal_thread_errno_to_status(ret);
	}

	mutex_lock(&g_thread_registry_lock);
	list_add_tail(&ctx->node, &g_thread_registry);
	mutex_unlock(&g_thread_registry_lock);

	wake_up_process(ctx->task);
	*thread = ctx;
	return OSAL_SUCCESS;
}
EXPORT_SYMBOL_GPL(osal_thread_create);

int32_t osal_thread_join(osal_thread_t thread, void **retval)
{
	if (!thread)
		return OSAL_ERR_INVALID_POINTER;

	mutex_lock(&thread->lock);
	if (thread->detached || thread->joined) {
		mutex_unlock(&thread->lock);
		return OSAL_ERR_INVALID_STATE;
	}
	thread->joined = true;
	mutex_unlock(&thread->lock);

	wait_for_completion(&thread->done);
	if (retval)
		*retval = thread->retval;

	kfree(thread);
	return OSAL_SUCCESS;
}
EXPORT_SYMBOL_GPL(osal_thread_join);

int32_t osal_thread_detach(osal_thread_t thread)
{
	bool free_thread = false;

	if (!thread)
		return OSAL_ERR_INVALID_POINTER;

	mutex_lock(&thread->lock);
	if (thread->joined) {
		mutex_unlock(&thread->lock);
		return OSAL_ERR_INVALID_STATE;
	}
	thread->detached = true;
	free_thread = thread->completed;
	mutex_unlock(&thread->lock);

	if (free_thread)
		kfree(thread);

	return OSAL_SUCCESS;
}
EXPORT_SYMBOL_GPL(osal_thread_detach);

bool osal_thread_equal(osal_thread_t thread1, osal_thread_t thread2)
{
	return thread1 == thread2;
}
EXPORT_SYMBOL_GPL(osal_thread_equal);

osal_thread_t osal_thread_self(void)
{
	return osal_thread_find_current();
}
EXPORT_SYMBOL_GPL(osal_thread_self);

void osal_thread_exit(void *retval)
{
	osal_thread_t thread;

	mutex_lock(&g_thread_registry_lock);
	thread = osal_thread_find_current_locked();
	mutex_unlock(&g_thread_registry_lock);

	if (thread && osal_thread_complete(thread, retval))
		kfree(thread);

	kthread_complete_and_exit(NULL, 0);
}
EXPORT_SYMBOL_GPL(osal_thread_exit);

int32_t osal_thread_cancel(osal_thread_t thread)
{
	int ret;

	if (!thread || !thread->task)
		return OSAL_ERR_INVALID_POINTER;

	mutex_lock(&thread->lock);
	if (thread->detached || thread->joined) {
		mutex_unlock(&thread->lock);
		return OSAL_ERR_INVALID_STATE;
	}
	mutex_unlock(&thread->lock);

	ret = kthread_stop(thread->task);
	wait_for_completion(&thread->done);
	return osal_thread_errno_to_status(ret);
}
EXPORT_SYMBOL_GPL(osal_thread_cancel);

int32_t osal_thread_should_stop(void)
{
	return kthread_should_stop() ? 1 : 0;
}
EXPORT_SYMBOL_GPL(osal_thread_should_stop);

int32_t osal_thread_attr_init(osal_thread_attr_t *attr)
{
	if (!attr)
		return OSAL_ERR_INVALID_POINTER;

	memset(attr, 0, sizeof(*attr));
	attr->detach_state = OSAL_THREAD_CREATE_JOINABLE;
	attr->sched_policy = OSAL_SCHED_OTHER;
	return OSAL_SUCCESS;
}
EXPORT_SYMBOL_GPL(osal_thread_attr_init);

int32_t osal_thread_attr_destroy(osal_thread_attr_t *attr)
{
	if (!attr)
		return OSAL_ERR_INVALID_POINTER;

	return OSAL_SUCCESS;
}
EXPORT_SYMBOL_GPL(osal_thread_attr_destroy);

int32_t osal_thread_attr_set_stack_size(osal_thread_attr_t *attr,
					osal_size_t stacksize)
{
	if (!attr)
		return OSAL_ERR_INVALID_POINTER;

	attr->stack_size = stacksize;
	return OSAL_SUCCESS;
}
EXPORT_SYMBOL_GPL(osal_thread_attr_set_stack_size);

int32_t osal_thread_attr_get_stack_size(const osal_thread_attr_t *attr,
					osal_size_t *stacksize)
{
	if (!attr || !stacksize)
		return OSAL_ERR_INVALID_POINTER;

	*stacksize = attr->stack_size;
	return OSAL_SUCCESS;
}
EXPORT_SYMBOL_GPL(osal_thread_attr_get_stack_size);

int32_t osal_thread_attr_set_detach_state(osal_thread_attr_t *attr,
					  int32_t detachstate)
{
	if (!attr)
		return OSAL_ERR_INVALID_POINTER;
	if (detachstate != OSAL_THREAD_CREATE_JOINABLE &&
	    detachstate != OSAL_THREAD_CREATE_DETACHED)
		return OSAL_ERR_INVALID_PARAM;

	attr->detach_state = detachstate;
	return OSAL_SUCCESS;
}
EXPORT_SYMBOL_GPL(osal_thread_attr_set_detach_state);

int32_t osal_thread_attr_get_detach_state(const osal_thread_attr_t *attr,
					  int32_t *detachstate)
{
	if (!attr || !detachstate)
		return OSAL_ERR_INVALID_POINTER;

	*detachstate = attr->detach_state;
	return OSAL_SUCCESS;
}
EXPORT_SYMBOL_GPL(osal_thread_attr_get_detach_state);

int32_t osal_thread_attr_set_sched_policy(osal_thread_attr_t *attr,
					  int32_t policy)
{
	if (!attr)
		return OSAL_ERR_INVALID_POINTER;

	attr->sched_policy = policy;
	return OSAL_SUCCESS;
}
EXPORT_SYMBOL_GPL(osal_thread_attr_set_sched_policy);

int32_t osal_thread_attr_set_sched_param(osal_thread_attr_t *attr,
					 const osal_sched_param_t *param)
{
	if (!attr || !param)
		return OSAL_ERR_INVALID_POINTER;

	attr->sched_priority = param->sched_priority;
	return OSAL_SUCCESS;
}
EXPORT_SYMBOL_GPL(osal_thread_attr_set_sched_param);

int32_t osal_thread_attr_set_name(osal_thread_attr_t *attr, const char *name)
{
	if (!attr || !name)
		return OSAL_ERR_INVALID_POINTER;

	strscpy(attr->name, name, sizeof(attr->name));
	return OSAL_SUCCESS;
}
EXPORT_SYMBOL_GPL(osal_thread_attr_set_name);

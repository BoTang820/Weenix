#include "config.h"
#include "globals.h"

#include "errno.h"

#include "util/init.h"
#include "util/debug.h"
#include "util/list.h"
#include "util/string.h"

#include "proc/kthread.h"
#include "proc/proc.h"
#include "proc/sched.h"

#include "mm/slab.h"
#include "mm/page.h"

kthread_t *curthr; /* global */
static slab_allocator_t *kthread_allocator = NULL;

#ifdef __MTP__
/* Stuff for the reaper daemon, which cleans up dead detached threads */
static proc_t *reapd = NULL;
static kthread_t *reapd_thr = NULL;
static ktqueue_t reapd_waitq;
static list_t kthread_reapd_deadlist; /* Threads to be cleaned */

static void *kthread_reapd_run(int arg1, void *arg2);
#endif

void kthread_init()
{
        kthread_allocator = slab_allocator_create("kthread", sizeof(kthread_t));
        KASSERT(NULL != kthread_allocator);
}

/**
 * Allocates a new kernel stack.
 *
 * @return a newly allocated stack, or NULL if there is not enough
 * memory available
 */
static char *
alloc_stack(void)
{
        /* extra page for "magic" data */
        char *kstack;
        int npages = 1 + (DEFAULT_STACK_SIZE >> PAGE_SHIFT);
        kstack = (char *)page_alloc_n(npages);

        return kstack;
}

/**
 * Frees a stack allocated with alloc_stack.
 *
 * @param stack the stack to free
 */
static void
free_stack(char *stack)
{
        page_free_n(stack, 1 + (DEFAULT_STACK_SIZE >> PAGE_SHIFT));
}

void kthread_destroy(kthread_t *t)
{
        KASSERT(t && t->kt_kstack);
        free_stack(t->kt_kstack);
        if (list_link_is_linked(&t->kt_plink))
                list_remove(&t->kt_plink);

        slab_obj_free(kthread_allocator, t);
}

/*
 * Allocate a new stack with the alloc_stack function. The size of the
 * stack is DEFAULT_STACK_SIZE.
 *
 * Don't forget to initialize the thread context with the
 * context_setup function. The context should have the same pagetable
 * pointer as the process.
 */
kthread_t *
kthread_create(struct proc *p, kthread_func_t func, long arg1, void *arg2)
{
        KASSERT(NULL != p);

        struct kthread *res_thread = (struct kthread *)slab_obj_alloc(kthread_allocator);
        memset(res_thread, 0, sizeof(kthread_t));
        // res_thread->kt_ctx = NULL;

        res_thread->kt_kstack = alloc_stack();
        context_setup(&res_thread->kt_ctx, func, arg1, arg2, res_thread->kt_kstack, DEFAULT_STACK_SIZE, p->p_pagedir);
        res_thread->kt_proc = p;
        res_thread->kt_cancelled = 0;
        res_thread->kt_retval = 0;   // thread value would be returned
        res_thread->kt_wchan = NULL; // thread on queue

        res_thread->kt_state = KT_RUN; // init state of thread
        if (p->p_pid != PID_IDLE)
        {

                res_thread->kt_state = KT_NO_STATE;
        }
        res_thread->kt_errno = 0;

        list_link_init(&(res_thread->kt_plink));
        list_link_init(&(res_thread->kt_qlink));

        list_insert_tail(&(p->p_threads), &(res_thread->kt_plink));

        // NOT_YET_IMPLEMENTED("PROCS: kthread_create");
        return res_thread;
}

/*
 * If the thread to be cancelled is the current thread, this is
 * equivalent to calling kthread_exit. Otherwise, the thread is
 * sleeping (either on a waitqueue or a runqueue)
 * and we need to set the cancelled and retval fields of the
 * thread. On wakeup, threads should check their cancelled fields and
 * act accordingly.
 *
 * If the thread's sleep is cancellable, cancelling the thread should
 * wake it up from sleep.
 *
 * If the thread's sleep is not cancellable, we do nothing else here.
 *
 */
void kthread_cancel(kthread_t *kthr, void *retval)
{
        KASSERT(NULL != kthr);

        if (kthr == curthr)
        {
                kthread_exit(retval);
        }
        else
        {
                kthr->kt_cancelled = 1; // sched_cancel would give value also, but in case
                kthr->kt_retval = retval;

                sched_cancel(kthr);
        }
        // NOT_YET_IMPLEMENTED("PROCS: kthread_cancel");
}

/*
 * You need to set the thread's retval field and alert the current
 * process that a thread is exiting via proc_thread_exited. You should
 * refrain from setting the thread's state to KT_EXITED until you are
 * sure you won't make any more blocking calls before you invoke the
 * scheduler again.
 *
 * It may seem unneccessary to push the work of cleaning up the thread
 * over to the process. However, if you implement MTP, a thread
 * exiting does not necessarily mean that the process needs to be
 * cleaned up.
 *
 * The void * type of retval is simply convention and does not necessarily
 * indicate that retval is a pointer
 */
void kthread_exit(void *retval)
{
        KASSERT(NULL != curthr);
        // update curthr
        curthr->kt_retval = retval;
        curthr->kt_state = KT_EXITED;
        // NOT_YET_IMPLEMENTED("PROCS: kthread_exit");
        KASSERT(!curthr->kt_wchan);
        KASSERT((!curthr->kt_qlink.l_next) && (!curthr->kt_qlink.l_prev));

        KASSERT(curthr->kt_proc == curproc);

        proc_thread_exited(retval);
        // sched_switch(); // swith !!
}

void copy_thread_values(kthread_t *cloned_thread, kthread_t *thr) {
    cloned_thread->kt_wchan = thr->kt_wchan;
    cloned_thread->kt_state = thr->kt_state;
    cloned_thread->kt_errno = thr->kt_errno;
    cloned_thread->kt_retval = thr->kt_retval;
    cloned_thread->kt_cancelled = thr->kt_cancelled;
}

/*
 * The new thread will need its own context and stack. Think carefully
 * about which fields should be copied and which fields should be
 * freshly initialized.
 *
 * You do not need to worry about this until VM.
 */
kthread_t *kthread_clone(kthread_t *thr)
{
        // NOT_YET_IMPLEMENTED("VM: kthread_clone");
        // Allocate memory for the new thread
        KASSERT(KT_RUN == thr->kt_state);
        dbg(DBG_PRINT, "(GRADING3A 8.a)\n");

        kthread_t *newthr = NULL;
        newthr = (kthread_t *)slab_obj_alloc(kthread_allocator);
        memset(newthr, 0, sizeof(kthread_t));
        newthr->kt_kstack = alloc_stack();

        // Setup the context and copy values from original thread to cloned thread
        context_setup(&newthr->kt_ctx, NULL, NULL, NULL,
                      newthr->kt_kstack, DEFAULT_STACK_SIZE,
                      thr->kt_proc->p_pagedir);

        copy_thread_values(newthr, thr);

        // Initialize list links
        list_link_init(&newthr->kt_qlink);
        list_link_init(&newthr->kt_plink);

        // The process of the cloned thread is not set yet
        newthr->kt_proc = NULL;

        KASSERT(KT_RUN == newthr->kt_state);
        dbg(DBG_PRINT, "(GRADING3A 8.a)\n");
        // Return the cloned thread
        dbg(DBG_PRINT, "(GRADING3A)\n");
        return newthr;
}


/*
 * The following functions will be useful if you choose to implement
 * multiple kernel threads per process. This is strongly discouraged
 * unless your weenix is perfect.
 */
#ifdef __MTP__
int kthread_detach(kthread_t *kthr)
{
        NOT_YET_IMPLEMENTED("MTP: kthread_detach");
        return 0;
}

int kthread_join(kthread_t *kthr, void **retval)
{
        NOT_YET_IMPLEMENTED("MTP: kthread_join");
        return 0;
}

/* ------------------------------------------------------------------ */
/* -------------------------- REAPER DAEMON ------------------------- */
/* ------------------------------------------------------------------ */
static __attribute__((unused)) void
kthread_reapd_init()
{
        NOT_YET_IMPLEMENTED("MTP: kthread_reapd_init");
}
init_func(kthread_reapd_init);
init_depends(sched_init);

void kthread_reapd_shutdown()
{
        NOT_YET_IMPLEMENTED("MTP: kthread_reapd_shutdown");
}

static void *
kthread_reapd_run(int arg1, void *arg2)
{
        NOT_YET_IMPLEMENTED("MTP: kthread_reapd_run");
        return (void *)0;
}
#endif

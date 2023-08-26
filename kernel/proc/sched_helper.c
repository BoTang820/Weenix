#include "globals.h"
#include "errno.h"

#include "main/interrupt.h"

#include "proc/sched.h"
#include "proc/kthread.h"

#include "util/init.h"
#include "util/debug.h"

void ktqueue_enqueue(ktqueue_t *q, kthread_t *thr);
kthread_t * ktqueue_dequeue(ktqueue_t *q);

/*
 * Updates the thread's state and enqueues it on the given
 * queue. Returns when the thread has been woken up with wakeup_on or
 * broadcast_on.
 *
 * Use the private queue manipulation functions above.
 */
void
sched_sleep_on(ktqueue_t *q)
{       
        KASSERT(NULL != q);
        curthr->kt_state = KT_SLEEP;
        ktqueue_enqueue(q,curthr);
        sched_switch();
        dbg(DBG_PRINT, "(GRADING1A)\n");
        // NOT_YET_IMPLEMENTED("PROCS: sched_sleep_on");
}

kthread_t *
sched_wakeup_on(ktqueue_t *q)
{       
        KASSERT(NULL != q);
        struct kthread * res_thread = NULL;
        if(!sched_queue_empty(q)){
                res_thread = ktqueue_dequeue(q);
                KASSERT((res_thread->kt_state == KT_SLEEP) || (res_thread->kt_state == KT_SLEEP_CANCELLABLE));
                dbg(DBG_PRINT, "(GRADING1A 4.a)\n");
                if(res_thread != NULL){
                        res_thread->kt_state = KT_RUN;
                       sched_make_runnable(res_thread); 
                }
        }
        dbg(DBG_PRINT, "(GRADING1A)\n");
        // NOT_YET_IMPLEMENTED("PROCS: sched_wakeup_on");
        return res_thread;
}

void
sched_broadcast_on(ktqueue_t *q)
{
        KASSERT(NULL != q);
        struct kthread * res_thread;
        while(!sched_queue_empty(q)){
                res_thread = ktqueue_dequeue(q);
                res_thread->kt_state = KT_RUN;
                if(res_thread != NULL){
                        dbg(DBG_PRINT, "(GRADING1A)\n");
                       sched_make_runnable(res_thread); // INTERUPT
                }
        }
        dbg(DBG_PRINT, "(GRADING1A)\n");
        // NOT_YET_IMPLEMENTED("PROCS: sched_broadcast_on");
}


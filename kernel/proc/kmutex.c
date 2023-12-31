#include "globals.h"
#include "errno.h"

#include "util/debug.h"

#include "proc/kthread.h"
#include "proc/kmutex.h"

/*
 * IMPORTANT: Mutexes can _NEVER_ be locked or unlocked from an
 * interrupt context. Mutexes are _ONLY_ lock or unlocked from a
 * thread context.
 */

void kmutex_init(kmutex_t *mtx)
{
        KASSERT(mtx != NULL);

        // init holder to null
        mtx->km_holder = NULL;
        KASSERT(mtx->km_holder == NULL);

        // clear wait queue
        mtx->km_waitq.tq_size = 0;
        list_init(&(mtx->km_waitq.tq_list)); // damn, sched_queue_init(&mtx->km_waitq);

        // NOT_YET_IMPLEMENTED("PROCS: kmutex_init");
}

/*
 * This should block the current thread (by sleeping on the mutex's
 * wait queue) if the mutex is already taken.
 *
 * No thread should ever try to lock a mutex it already has locked.
 */
void kmutex_lock(kmutex_t *mtx)
{
        // KASSERT(NULL != mtx);

        // existed and not authoried
        KASSERT(curthr && (curthr != mtx->km_holder));

        if (mtx->km_holder == NULL)
        { // ready to get
                mtx->km_holder = curthr;
        }
        else
        { // not ready to get

                sched_sleep_on(&(mtx->km_waitq)); // damn
        }
        // NOT_YET_IMPLEMENTED("PROCS: kmutex_lock");
}

/*
 * This should do the same as kmutex_lock, but use a cancellable sleep
 * instead. Also, if you are cancelled while holding mtx, you should unlock mtx.
 */
int kmutex_lock_cancellable(kmutex_t *mtx)
{
        KASSERT(mtx != NULL);

        // existed and not authoried
        // KASSERT(curthr);
        // KASSERT(curthr != mtx->km_holder);
        KASSERT(curthr && (curthr != mtx->km_holder));

        if (mtx->km_holder == NULL)
        { // ready to get
                mtx->km_holder = curthr;

                return 0;
        }
        else
        { // not ready to get

                return sched_cancellable_sleep_on(&(mtx->km_waitq));
        }

        // NOT_YET_IMPLEMENTED("PROCS: kmutex_lock_cancellable");
}

/*
 * If there are any threads waiting to take a lock on the mutex, one
 * should be woken up and given the lock.
 *
 * Note: This should _NOT_ be a blocking operation!
 *
 * Note: Ensure the new owner of the mutex enters the run queue.
 *
 * Note: Make sure that the thread on the head of the mutex's wait
 * queue becomes the new owner of the mutex.
 *
 * @param mtx the mutex to unlock
 */
void kmutex_unlock(kmutex_t *mtx)
{
        KASSERT(mtx != NULL);

        // existed and not authoried
        // KASSERT(curthr);
        // KASSERT(curthr != mtx->km_holder);
        KASSERT(curthr && (curthr == mtx->km_holder));

        mtx->km_holder = NULL;
        if (!sched_queue_empty(&(mtx->km_waitq)))
        {

                mtx->km_holder = sched_wakeup_on(&(mtx->km_waitq));
        }
        // NOT_YET_IMPLEMENTED("PROCS: kmutex_unlock");
        KASSERT(curthr != mtx->km_holder); // no return back itself
}

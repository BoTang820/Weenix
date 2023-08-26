#include "types.h"
#include "globals.h"
#include "kernel.h"
#include "errno.h"

#include "util/gdb.h"
#include "util/init.h"
#include "util/debug.h"
#include "util/string.h"
#include "util/printf.h"

#include "mm/mm.h"
#include "mm/page.h"
#include "mm/pagetable.h"
#include "mm/pframe.h"

#include "vm/vmmap.h"
#include "vm/shadowd.h"
#include "vm/shadow.h"
#include "vm/anon.h"

#include "main/acpi.h"
#include "main/apic.h"
#include "main/interrupt.h"
#include "main/gdt.h"

#include "proc/sched.h"
#include "proc/proc.h"
#include "proc/kthread.h"

#include "drivers/dev.h"
#include "drivers/blockdev.h"
#include "drivers/disk/ata.h"
#include "drivers/tty/virtterm.h"
#include "drivers/pci.h"

#include "api/exec.h"
#include "api/syscall.h"

#include "fs/vfs.h"
#include "fs/vnode.h"
#include "fs/vfs_syscall.h"
#include "fs/fcntl.h"
#include "fs/stat.h"

#include "test/kshell/kshell.h"
#include "test/s5fs_test.h"

GDB_DEFINE_HOOK(initialized)

void *bootstrap(int arg1, void *arg2);
void *idleproc_run(int arg1, void *arg2);
kthread_t *initproc_create(void);
void *initproc_run(int arg1, void *arg2);
void *final_shutdown(void);

extern void *faber_thread_test(int arg1, void *arg2);
extern void *sunghan_test(int arg1, void *arg2);
extern void *sunghan_deadlock_test(int arg1, void *arg2);

// for VFS test
#ifdef __VFS__
extern void *vfstest_main(int, void *);
extern int faber_fs_thread_test(kshell_t *ksh, int argc, char **argv);
extern int faber_directory_test(kshell_t *ksh, int argc, char **argv);
#endif

#ifdef __DRIVERS__

int sunghan_test_dummy(kshell_t *kshell, int argc, char **argv)
{
        KASSERT(kshell != NULL);
        proc_t *tmp_proc = proc_create("sunghanTest");
        kthread_t *tmp_thread = kthread_create(tmp_proc, sunghan_test, 0, NULL);
        sched_make_runnable(tmp_thread);
        while (!list_empty(&curproc->p_children))
        {
                do_waitpid(-1, 0, NULL);
        }
        return 0;
}

int sunghan_deadlock_test_dummy(kshell_t *kshell, int argc, char **argv)
{
        KASSERT(kshell != NULL);
        proc_t *tmp_proc = proc_create("sunghanDeadlockTest");
        kthread_t *tmp_thread = kthread_create(tmp_proc, sunghan_deadlock_test, 0, NULL);
        sched_make_runnable(tmp_thread);
        while (!list_empty(&curproc->p_children))
        {
                do_waitpid(-1, 0, NULL);
        }
        return 0;
}

int faber_thread_test_dummy(kshell_t *kshell, int argc, char **argv)
{
        KASSERT(kshell != NULL);
        proc_t *tmp_proc = proc_create("faberThreadTest");
        kthread_t *tmp_thread = kthread_create(tmp_proc, faber_thread_test, 0, NULL);
        sched_make_runnable(tmp_thread);
        while (!list_empty(&curproc->p_children))
        {
                do_waitpid(-1, 0, NULL);
        }
        return 0;
}

#ifdef __VFS__
#ifdef __VFS__
int my_vfs_test()
{
        proc_t *pt_vfs;
        kthread_t *kt_vfs;
        int status;
        pt_vfs = proc_create("VFS");

        kt_vfs = kthread_create(pt_vfs, vfstest_main, 1, NULL);

        sched_make_runnable(kt_vfs);
        do_waitpid(pt_vfs->p_pid, 0, &status);
        dbg(DBG_PRINT, "(GRADING2B)\n");
        return 0;
}
#endif
#endif /* __VFS__ */

#endif
/* __DRIVERS__ */

/**
 * This function is called from kmain, however it is not running in a
 * thread context yet. It should create the idle process which will
 * start executing idleproc_run() in a real thread context.  To start
 * executing in the new process's context call context_make_active(),
 * passing in the appropriate context. This function should _NOT_
 * return.
 *
 * Note: Don't forget to set curproc and curthr appropriately.
 *
 * @param arg1 the first argument (unused)
 * @param arg2 the second argument (unused)
 */
void *
bootstrap(int arg1, void *arg2)
{
        /* If the next line is removed/altered in your submission, 20 points will be deducted. */
        dbgq(DBG_TEST, "SIGNATURE: 53616c7465645f5fef133aebbf8b84d00655774bb15fa449753226371ae1cd519fca62194b226b7c317530c0b0dc92dc\n");
        /* necessary to finalize page table information */
        pt_template_init();

        curproc = proc_create("IDLE");                           // create idle process
        curthr = kthread_create(curproc, idleproc_run, 0, NULL); // create thread for idle process
        KASSERT(NULL != curproc);

        KASSERT(NULL != curthr);

        KASSERT(PID_IDLE == curproc->p_pid);

        context_make_active(&curthr->kt_ctx);
        // NOT_YET_IMPLEMENTED("PROCS: bootstrap");

        panic("weenix returned to bootstrap()!!! BAD!!!\n");
        return NULL;
}

/**
 * Once we're inside of idleproc_run(), we are executing in the context of the
 * first process-- a real context, so we can finally begin running
 * meaningful code.
 *
 * This is the body of process 0. It should initialize all that we didn't
 * already initialize in kmain(), launch the init process (initproc_run),
 * wait for the init process to exit, then halt the machine.
 *
 * @param arg1 the first argument (unused)
 * @param arg2 the second argument (unused)
 */
void *
idleproc_run(int arg1, void *arg2)
{
        int status;
        pid_t child;

        /* create init proc */
        kthread_t *initthr = initproc_create();
        init_call_all();
        GDB_CALL_HOOK(initialized);

        /* Create other kernel threads (in order) */

#ifdef __VFS__
        /* Once you have VFS remember to set the current working directory
         * of the idle and init processes */

        // for debug
        KASSERT(NULL != vfs_root_vn);
        curproc->p_cwd = vfs_root_vn; // set cwd of idle process
        vref(vfs_root_vn);            // increment reference count of vfs_root_vn

        initthr->kt_proc->p_cwd = vfs_root_vn; // set cwd of init process
        vref(vfs_root_vn);                     // increment reference count of vfs_root_vn

        // NOT_YET_IMPLEMENTED("VFS: idleproc_run");

        /* Here you need to make the null, zero, and tty devices using mknod */
        /* You can't do this until you have VFS, check the include/drivers/dev.h
         * file for macros with the device ID's you will need to pass to mknod */
        do_mkdir("/dev");

        // add vfs_is_in_use
        do_mknod("/dev/null", S_IFCHR, MEM_NULL_DEVID);
        do_mknod("/dev/zero", S_IFCHR, MEM_ZERO_DEVID);
        do_mknod("/dev/tty0", S_IFCHR, MKDEVID(2, 0));
        do_mknod("/dev/tty1", S_IFCHR, MKDEVID(2, 1));
        dbg(DBG_PRINT, "(GRADING2A)\n");

        // NOT_YET_IMPLEMENTED("VFS: idleproc_run");
#endif

        /* Finally, enable interrupts (we want to make sure interrupts
         * are enabled AFTER all drivers are initialized) */
        intr_enable();

        /* Run initproc */
        sched_make_runnable(initthr);
        /* Now wait for it */
        child = do_waitpid(-1, 0, &status);
        KASSERT(PID_INIT == child);

        return final_shutdown();
}

/**
 * This function, called by the idle process (within 'idleproc_run'), creates the
 * process commonly refered to as the "init" process, which should have PID 1.
 *
 * The init process should contain a thread which begins execution in
 * initproc_run().
 *
 * @return a pointer to a newly created thread which will execute
 * initproc_run when it begins executing
 */
kthread_t *
initproc_create(void)
{
        proc_t *res_proc = proc_create("init proc");
        KASSERT(NULL != res_proc);

        KASSERT(PID_INIT == res_proc->p_pid);

        kthread_t *res_thread = kthread_create(res_proc, initproc_run, 0, NULL);

        KASSERT(NULL != res_thread);

        // NOT_YET_IMPLEMENTED("PROCS: initproc_create");
        return res_thread;
}

/**
 * The init thread's function changes depending on how far along your Weenix is
 * developed. Before VM/FI, you'll probably just want to have this run whatever
 * tests you've written (possibly in a new process). After VM/FI, you'll just
 * exec "/sbin/init".
 *
 * Both arguments are unused.
 *
 * @param arg1 the first argument (unused)
 * @param arg2 the second argument (unused)
 */
void *
initproc_run(int arg1, void *arg2)
{
#ifdef __DRIVERS__

        kshell_add_command("sunghan", sunghan_test_dummy, "Run sunghan_test().");
        kshell_add_command("deadlock", sunghan_deadlock_test_dummy, "Run sunghan_deadlock_test().");
        kshell_add_command("faber", faber_thread_test_dummy, "Run faber_thread_test().");
#ifdef __VFS__
        kshell_add_command("vfstest", my_vfs_test, "Run vfstest().");
        kshell_add_command("thrtest", faber_fs_thread_test, "Run faber_fs_thread_test().");
        kshell_add_command("dirtest", faber_directory_test, "Run faber_directory_test().");
        dbg(DBG_PRINT, "(GRADING2B)\n");
#endif /* __VFS__ */

#ifdef __VM__
    char *argv[] = {"/sbin/init", NULL};
    char *envp[] = {NULL};
    kernel_execve("/sbin/init", argv, envp);

#else
    kshell_t *kshell = kshell_create(0);
    while (kshell_execute_next(kshell)) {
    }

    kshell_destroy(kshell);
#endif /* __VM__ */


#endif /* __DRIVERS__ */

        return NULL;
}

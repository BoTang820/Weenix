#include "types.h"
#include "globals.h"
#include "errno.h"

#include "util/debug.h"
#include "util/string.h"

#include "proc/proc.h"
#include "proc/kthread.h"

#include "mm/mm.h"
#include "mm/mman.h"
#include "mm/page.h"
#include "mm/pframe.h"
#include "mm/mmobj.h"
#include "mm/pagetable.h"
#include "mm/tlb.h"

#include "fs/file.h"
#include "fs/vnode.h"

#include "vm/shadow.h"
#include "vm/vmmap.h"

#include "api/exec.h"

#include "main/interrupt.h"

/* Pushes the appropriate things onto the kernel stack of a newly forked thread
 * so that it can begin execution in userland_entry.
 * regs: registers the new thread should have on execution
 * kstack: location of the new thread's kernel stack
 * Returns the new stack pointer on success. */
static uint32_t
fork_setup_stack(const regs_t *regs, void *kstack)
{
    /* Pointer argument and dummy return address, and userland dummy return
     * address */
    uint32_t esp = ((uint32_t)kstack) + DEFAULT_STACK_SIZE - (sizeof(regs_t) + 12);
    *(void **)(esp + 4) = (void *)(esp + 8);         /* Set the argument to point to location of struct on stack */
    memcpy((void *)(esp + 8), regs, sizeof(regs_t)); /* Copy over struct */
    return esp;
}

/*
 * The implementation of fork(2). Once this works,
 * you're practically home free. This is what the
 * entirety of Weenix has been leading up to.
 * Go forth and conquer.
 */
proc_t *create_child_process()
{
    proc_t *child = proc_create("childProcess");
    child->p_vmmap = vmmap_clone(curproc->p_vmmap);
    child->p_vmmap->vmm_proc = child;
    dbg(DBG_PRINT, "(GRADING3A)\n");
    return child;
}

void handle_memory_object(vmarea_t *childVmArea, vmarea_t *parentVmArea)
{
    if (MAP_SHARED != (childVmArea->vma_flags & MAP_TYPE))
    {
        mmobj_t *parentSO, *childSO;
        parentSO = shadow_create();

        parentSO->mmo_shadowed = parentVmArea->vma_obj;
        parentSO->mmo_un.mmo_bottom_obj = mmobj_bottom_obj(parentVmArea->vma_obj);

        childSO = shadow_create();
        childSO->mmo_shadowed = parentVmArea->vma_obj;
        childSO->mmo_shadowed->mmo_ops->ref(childSO->mmo_shadowed);
        childSO->mmo_un.mmo_bottom_obj = mmobj_bottom_obj(parentVmArea->vma_obj);
        childSO->mmo_un.mmo_bottom_obj->mmo_ops->ref(childSO->mmo_un.mmo_bottom_obj);
        childVmArea->vma_obj = childSO;

        parentSO->mmo_un.mmo_bottom_obj->mmo_ops->ref(parentSO->mmo_un.mmo_bottom_obj);
        parentVmArea->vma_obj = parentSO;
        dbg(DBG_PRINT, "(GRADING3A)\n");
    }
    else
    {
        childVmArea->vma_obj = parentVmArea->vma_obj;
        childVmArea->vma_obj->mmo_ops->ref(childVmArea->vma_obj);
        dbg(DBG_PRINT, "(GRADING3D 2)\n");
    }
    list_insert_tail(mmobj_bottom_vmas(parentVmArea->vma_obj), &childVmArea->vma_olink);
    dbg(DBG_PRINT, "(GRADING3A)\n");
}

void setup_process_context(kthread_t *childThread, proc_t *childProcess, struct regs *regs)
{
    regs->r_eax = 0;
    childThread->kt_ctx.c_pdptr = childProcess->p_pagedir;
    childThread->kt_ctx.c_esp = fork_setup_stack(regs, childThread->kt_kstack);
    childThread->kt_ctx.c_eip = (uint32_t)userland_entry;
    childThread->kt_ctx.c_kstacksz = DEFAULT_STACK_SIZE;
    childThread->kt_ctx.c_kstack = (uintptr_t)childThread->kt_kstack;
    regs->r_eax = childProcess->p_pid;
    dbg(DBG_PRINT, "(GRADING3A)\n");
}

void copy_parent_files(proc_t *childProcess)
{
    for (int i = 0; i < NFILES; i++)
    {
        childProcess->p_files[i] = curproc->p_files[i];
        if (childProcess->p_files[i] != NULL)
        {
            fref(childProcess->p_files[i]);
            dbg(DBG_PRINT, "(GRADING3A)\n");
        }
        dbg(DBG_PRINT, "(GRADING3A)\n");
    }

    if (childProcess->p_cwd != NULL)
    {
        vput(childProcess->p_cwd);
        dbg(DBG_PRINT, "(GRADING3A)\n");
    }

    childProcess->p_cwd = curproc->p_cwd;
    if (curproc->p_cwd != NULL)
    {
        vref(childProcess->p_cwd);
        dbg(DBG_PRINT, "(GRADING3A)\n");
    }
    dbg(DBG_PRINT, "(GRADING3A)\n");
}

int do_fork(struct regs *regs)
{
    KASSERT(regs != NULL);
    KASSERT(curproc != NULL);
    KASSERT(curproc->p_state == PROC_RUNNING);
    dbg(DBG_PRINT, "(GRADING3A 7.a)\n");

    proc_t *newproc = create_child_process();

    vmarea_t *childVmArea = NULL;
    list_iterate_begin(&newproc->p_vmmap->vmm_list, childVmArea, vmarea_t, vma_plink)
    {
        vmarea_t *parentVmArea = vmmap_lookup(curproc->p_vmmap, childVmArea->vma_start);
        handle_memory_object(childVmArea, parentVmArea);
        dbg(DBG_PRINT, "(GRADING3A)\n");
    }
    list_iterate_end();

    pt_unmap_range(curproc->p_pagedir, USER_MEM_LOW, USER_MEM_HIGH);
    tlb_flush_all();

    kthread_t *newthr = kthread_clone(curthr);
    KASSERT(newproc->p_state == PROC_RUNNING);
    KASSERT(newproc->p_pagedir != NULL);
    KASSERT(newthr->kt_kstack != NULL);
    dbg(DBG_PRINT, "(GRADING3A 7.a)\n");

    setup_process_context(newthr, newproc, regs);

    copy_parent_files(newproc);

    newthr->kt_proc = newproc;
    list_insert_tail(&newproc->p_threads, &newthr->kt_plink);

    newproc->p_brk = curproc->p_brk;
    newproc->p_start_brk = curproc->p_start_brk;

    sched_make_runnable(newthr);
    dbg(DBG_PRINT, "(GRADING3A)\n");
    return newproc->p_pid;
}

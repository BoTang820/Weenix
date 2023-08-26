#include "globals.h"
#include "errno.h"
#include "util/debug.h"

#include "mm/mm.h"
#include "mm/page.h"
#include "mm/mman.h"

#include "vm/mmap.h"
#include "vm/vmmap.h"

#include "proc/proc.h"

/*
 * This function implements the brk(2) system call.
 *
 * This routine manages the calling process's "break" -- the ending address
 * of the process's "dynamic" region (often also referred to as the "heap").
 * The current value of a process's break is maintained in the 'p_brk' member
 * of the proc_t structure that represents the process in question.
 *
 * The 'p_brk' and 'p_start_brk' members of a proc_t struct are initialized
 * by the loader. 'p_start_brk' is subsequently never modified; it always
 * holds the initial value of the break. Note that the starting break is
 * not necessarily page aligned!
 *
 * 'p_start_brk' is the lower limit of 'p_brk' (that is, setting the break
 * to any value less than 'p_start_brk' should be disallowed).
 *
 * The upper limit of 'p_brk' is defined by the minimum of (1) the
 * starting address of the next occuring mapping or (2) USER_MEM_HIGH.
 * That is, growth of the process break is limited only in that it cannot
 * overlap with/expand into an existing mapping or beyond the region of
 * the address space allocated for use by userland. (note the presence of
 * the 'vmmap_is_range_empty' function).
 *
 * The dynamic region should always be represented by at most ONE vmarea.
 * Note that vmareas only have page granularity, you will need to take this
 * into account when deciding how to set the mappings if p_brk or p_start_brk
 * is not page aligned.
 *
 * You are guaranteed that the process data/bss region is non-empty.
 * That is, if the starting brk is not page-aligned, its page has
 * read/write permissions.
 *
 * If addr is NULL, you should "return" the current break. We use this to
 * implement sbrk(0) without writing a separate syscall. Look in
 * user/libc/syscall.c if you're curious.
 *
 * You should support combined use of brk and mmap in the same process.
 *
 * Note that this function "returns" the new break through the "ret" argument.
 * Return 0 on success, -errno on failure.
 */

// Helper function to validate address
static int validate_brk_addr(void *addr) {
    if (addr < curproc->p_start_brk || (uint32_t)addr > USER_MEM_HIGH) {
        dbg(DBG_PRINT, "(GRADING3D 2)\n");
        return -ENOMEM;
    }
    dbg(DBG_PRINT, "(GRADING3A)\n");
    return 0;
}

// Helper function to handle break expansion
static int expand_brk(vmarea_t *vma, uint32_t prev_end, uint32_t cur_end,
                      uint32_t npages) {
    if (!vmmap_is_range_empty(curproc->p_vmmap, prev_end, npages)) {
        dbg(DBG_PRINT, "(GRADING3D 2)\n");
        return -ENOMEM;
    }
    vma->vma_end = cur_end;
    dbg(DBG_PRINT, "(GRADING3A)\n");
    return 0;
}

int do_brk(void *addr, void **ret) {
        // NOT_YET_IMPLEMENTED("VM: do_brk");
    int err;
    vmarea_t *vma;
    uint32_t prev_end;
    uint32_t cur_end;
    uint32_t num_pages;

    if (addr == NULL) {
        *ret = curproc->p_brk;
        dbg(DBG_PRINT, "(GRADING3A)\n");
        return 0;
    }

    if ((err = validate_brk_addr(addr)) < 0) {
        dbg(DBG_PRINT, "(GRADING3D 2)\n");
        return err;
    }

    vma = vmmap_lookup(curproc->p_vmmap, ADDR_TO_PN(curproc->p_start_brk));

    prev_end = vma->vma_end;
    cur_end = ADDR_TO_PN(PAGE_ALIGN_UP(addr));
    num_pages = cur_end - prev_end;

    if (prev_end < cur_end) {
        if ((err = expand_brk(vma, prev_end, cur_end, num_pages)) < 0) {
            dbg(DBG_PRINT, "(GRADING3D 2)\n");
            return err;
        }
    } else if (prev_end > cur_end) {
        vmmap_remove(curproc->p_vmmap, cur_end, -num_pages);
        dbg(DBG_PRINT, "(GRADING3D 2)\n");
    }

    curproc->p_brk = addr;
    *ret = curproc->p_brk;
    dbg(DBG_PRINT, "(GRADING3A)\n");
    return 0;
}


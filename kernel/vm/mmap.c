#include "globals.h"
#include "errno.h"
#include "types.h"

#include "mm/mm.h"
#include "mm/tlb.h"
#include "mm/mman.h"
#include "mm/page.h"

#include "proc/proc.h"

#include "util/string.h"
#include "util/debug.h"

#include "fs/vnode.h"
#include "fs/vfs.h"
#include "fs/file.h"

#include "vm/vmmap.h"
#include "vm/mmap.h"

// Helper function for argument validation
static int validate_mmap_args(void *addr, size_t len, int prot, int flags,
                              off_t off)
{
    if (!PAGE_ALIGNED(off))
    {
        return -EINVAL;
    }
    if (len == 0)
    {
        return -EINVAL;
    }

    if ((flags & MAP_FIXED) && addr == NULL)
    {
        return -EINVAL;
    }
    if ((addr != NULL && !PAGE_ALIGNED(addr)))
    {
        return -EINVAL;
    }

    if ((uint32_t)addr + len > USER_MEM_HIGH)
    {
        return -EINVAL;
    }

    if (!(flags & MAP_PRIVATE) && !(flags & MAP_SHARED))
    {
        return -EINVAL;
    }

    return 0;
}

// Helper function for handling file-related tasks
static int handle_file_related_tasks(int fd, int flags, int prot, file_t **file,
                                     vnode_t **vn)
{
    if (!(flags & MAP_ANON))
    {
        if (fd == -1 || (*file = fget(fd)) == NULL)
        {
            return -EBADF;
        }
        *vn = (*file)->f_vnode;

        if (!((*file)->f_mode & FMODE_WRITE) && (prot & PROT_WRITE) &&
            !(flags & MAP_PRIVATE))
        {
            fput(*file);
            return -EACCES;
        }
    }
    return 0;
}

/*
 * This function implements the mmap(2) syscall, but only
 * supports the MAP_SHARED, MAP_PRIVATE, MAP_FIXED, and
 * MAP_ANON flags.
 *
 * Add a mapping to the current process's address space.
 * You need to do some error checking; see the ERRORS section
 * of the manpage for the problems you should anticipate.
 * After error checking most of the work of this function is
 * done by vmmap_map(), but remember to clear the TLB.
 */

int do_mmap(void *addr, size_t len, int prot, int flags, int fd, off_t off,
            void **ret)
{
    // NOT_YET_IMPLEMENTED("VM: do_mmap");
    uint32_t lopage, npages;
    vnode_t *vn = NULL;
    file_t *file = NULL;
    vmarea_t *new;
    int err;

err = validate_mmap_args(addr, len, prot, flags, off);
    // if ((err = validate_mmap_args(addr, len, prot, flags, off)) < 0)
    // {
    //     KASSERT(0==1);
    //     return err;
    // }

    lopage = (addr == NULL) ? 0 : ADDR_TO_PN(addr);

err = handle_file_related_tasks(fd, flags, prot, &file, &vn);
    // if ((err = handle_file_related_tasks(fd, flags, prot, &file, &vn)) < 0)
    // {
    //     KASSERT(0==1);
    //     return err;
    // }

    npages = len / PAGE_SIZE + ((len % PAGE_SIZE != 0) ? 1 : 0);

    if ((err = vmmap_map(curproc->p_vmmap, vn, lopage, npages, prot, flags, off,
                         VMMAP_DIR_HILO, &new)) < 0)
    {
        if (file == NULL)
        {
            dbg(DBG_PRINT, "(GRADING3D 2)\n");
            return err;
        }
        // else
        // {
        //     fput(file);
        //     KASSERT(0==1);
        //     return err;
        // }
        // KASSERT(0==1);
    }

    if (file != NULL)
    {
        fput(file);
        dbg(DBG_PRINT, "(GRADING3A)\n");
    }
    *ret = PN_TO_ADDR(new->vma_start);

    tlb_flush_all();

    KASSERT(NULL != curproc->p_pagedir);
    dbg(DBG_PRINT, "(GRADING3A 2.a)\n");
    dbg(DBG_PRINT, "(GRADING3A)\n");
    return 0;
}

// Helper function for argument validation
static int validate_munmap_args(void *addr, size_t len)
{
    if (!PAGE_ALIGNED(addr))
    {
        dbg(DBG_PRINT, "(GRADING3D 5)\n");
        return -EINVAL;
    }
    if ((uint32_t)addr + len > USER_MEM_HIGH)
    {
        dbg(DBG_PRINT, "(GRADING3D 5)\n");
        return -EINVAL;
    }

    if ((uint32_t)addr + len < USER_MEM_LOW)
    {
        dbg(DBG_PRINT, "(GRADING3D 5)\n");
        return -EINVAL;
    }
    // if (len == 0 || len > USER_MEM_HIGH)
    // {
    //     KASSERT(0==1);
    //     return -EINVAL;
    // }
    dbg(DBG_PRINT, "(GRADING3D 2)\n");
    return 0;
}

/*
 * This function implements the munmap(2) syscall.
 *
 * As with do_mmap() it should perform the required error checking,
 * before calling upon vmmap_remove() to do most of the work.
 * Remember to clear the TLB.
 */
int do_munmap(void *addr, size_t len)
{
    // NOT_YET_IMPLEMENTED("VM: do_munmap");
    int err;
    uint32_t num_pages;

    if ((err = validate_munmap_args(addr, len)) < 0)
    {
        dbg(DBG_PRINT, "(GRADING3D 5)\n");
        return err;
    }

    if (len % PAGE_SIZE == 0)
    {
        num_pages = len / PAGE_SIZE;
        dbg(DBG_PRINT, "(GRADING3D 2)\n");
    }
    else
    {
        num_pages = len / PAGE_SIZE + 1;
        dbg(DBG_PRINT, "(GRADING3D 2)\n");
    }

    vmmap_remove(curproc->p_vmmap, ADDR_TO_PN(addr), num_pages);
    tlb_flush_all();
    dbg(DBG_PRINT, "(GRADING3D 2)\n");
    return 0;
}

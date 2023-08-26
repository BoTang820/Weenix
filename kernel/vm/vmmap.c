#include "kernel.h"
#include "errno.h"
#include "globals.h"

#include "vm/vmmap.h"
#include "vm/shadow.h"
#include "vm/anon.h"

#include "proc/proc.h"

#include "util/debug.h"
#include "util/list.h"
#include "util/string.h"
#include "util/printf.h"

#include "fs/vnode.h"
#include "fs/file.h"
#include "fs/fcntl.h"
#include "fs/vfs_syscall.h"

#include "mm/slab.h"
#include "mm/page.h"
#include "mm/mm.h"
#include "mm/mman.h"
#include "mm/mmobj.h"
#include "mm/tlb.h"

static slab_allocator_t *vmmap_allocator;
static slab_allocator_t *vmarea_allocator;

void vmmap_init(void)
{
    vmmap_allocator = slab_allocator_create("vmmap", sizeof(vmmap_t));
    KASSERT(NULL != vmmap_allocator && "failed to create vmmap allocator!");
    vmarea_allocator = slab_allocator_create("vmarea", sizeof(vmarea_t));
    KASSERT(NULL != vmarea_allocator && "failed to create vmarea allocator!");
}

vmarea_t *
vmarea_alloc(void)
{
    vmarea_t *newvma = (vmarea_t *)slab_obj_alloc(vmarea_allocator);
    if (newvma)
    {
        newvma->vma_vmmap = NULL;
    }
    return newvma;
}

void vmarea_free(vmarea_t *vma)
{
    KASSERT(NULL != vma);
    slab_obj_free(vmarea_allocator, vma);
}

/* a debugging routine: dumps the mappings of the given address space. */
size_t
vmmap_mapping_info(const void *vmmap, char *buf, size_t osize)
{
    KASSERT(0 < osize);
    KASSERT(NULL != buf);
    KASSERT(NULL != vmmap);

    vmmap_t *map = (vmmap_t *)vmmap;
    vmarea_t *vma;
    ssize_t size = (ssize_t)osize;

    int len = snprintf(buf, size, "%21s %5s %7s %8s %10s %12s\n",
                       "VADDR RANGE", "PROT", "FLAGS", "MMOBJ", "OFFSET",
                       "VFN RANGE");

    list_iterate_begin(&map->vmm_list, vma, vmarea_t, vma_plink)
    {
        size -= len;
        buf += len;
        if (0 >= size)
        {
            goto end;
        }

        len = snprintf(buf, size,
                       "%#.8x-%#.8x  %c%c%c  %7s 0x%p %#.5x %#.5x-%#.5x\n",
                       vma->vma_start << PAGE_SHIFT,
                       vma->vma_end << PAGE_SHIFT,
                       (vma->vma_prot & PROT_READ ? 'r' : '-'),
                       (vma->vma_prot & PROT_WRITE ? 'w' : '-'),
                       (vma->vma_prot & PROT_EXEC ? 'x' : '-'),
                       (vma->vma_flags & MAP_SHARED ? " SHARED" : "PRIVATE"),
                       vma->vma_obj, vma->vma_off, vma->vma_start, vma->vma_end);
    }
    list_iterate_end();

end:
    if (size <= 0)
    {
        size = osize;
        buf[osize - 1] = '\0';
    }
    /*
    KASSERT(0 <= size);
    if (0 == size) {
            size++;
            buf--;
            buf[0] = '\0';
    }
    */
    return osize - size;
}

/* Create a new vmmap, which has no vmareas and does
 * not refer to a process. */
vmmap_t *
vmmap_create(void)
{
    // NOT_YET_IMPLEMENTED("VM: vmmap_create");
    // return NULL;

    vmmap_t *map;
    map = (vmmap_t *)slab_obj_alloc(vmmap_allocator);

    map->vmm_proc = NULL;
    list_init(&map->vmm_list);
    dbg(DBG_PRINT, "(GRADING3A)\n");

    return map;
}

/* Removes all vmareas from the address space and frees the
 * vmmap struct. */
void vmmap_destroy(vmmap_t *map)
{
    // NOT_YET_IMPLEMENTED("VM: vmmap_destroy");
    KASSERT(NULL != map);
    dbg(DBG_PRINT, "(GRADING3A 3.a)\n");

    vmarea_t *tmp;
    list_iterate_begin(&map->vmm_list, tmp, vmarea_t, vma_plink)
    {

        if (list_link_is_linked(&tmp->vma_plink))
        {
            list_remove(&tmp->vma_plink);
            dbg(DBG_PRINT, "(GRADING3A)\n");
        }

        if (tmp->vma_obj != NULL)
        {
            tmp->vma_obj->mmo_ops->put(tmp->vma_obj);
            dbg(DBG_PRINT, "(GRADING3A)\n");
        }

        if (list_link_is_linked(&tmp->vma_olink))
        {
            list_remove(&tmp->vma_olink);
            dbg(DBG_PRINT, "(GRADING3A)\n");
        }
        dbg(DBG_PRINT, "(GRADING3A)\n");
    }
    list_iterate_end();

    slab_obj_free(vmmap_allocator, map);
    dbg(DBG_PRINT, "(GRADING3A)\n");
}

/* Add a vmarea to an address space. Assumes (i.e. asserts to some extent)
 * the vmarea is valid.  This involves finding where to put it in the list
 * of VM areas, and adding it. Don't forget to set the vma_vmmap for the
 * area. */
void vmmap_insert(vmmap_t *map, vmarea_t *newvma)
{
    // NOT_YET_IMPLEMENTED("VM: vmmap_insert");

    KASSERT(NULL != map && NULL != newvma);
    KASSERT(NULL == newvma->vma_vmmap);
    KASSERT(newvma->vma_start < newvma->vma_end);
    KASSERT(ADDR_TO_PN(USER_MEM_LOW) <= newvma->vma_start && ADDR_TO_PN(USER_MEM_HIGH) >= newvma->vma_end);

    dbg(DBG_PRINT, "(GRADING3A 3.b)\n");

    vmarea_t *vma;

    list_iterate_begin(&map->vmm_list, vma, vmarea_t, vma_plink)
    {
        if (vma->vma_start >= newvma->vma_end)
        {
            newvma->vma_vmmap = map;
            list_insert_before(&vma->vma_plink, &newvma->vma_plink);
            dbg(DBG_PRINT, "(GRADING3A)\n");
            return;
        }
        dbg(DBG_PRINT, "(GRADING3A)\n");
    }
    list_iterate_end();

    newvma->vma_vmmap = map;
    list_insert_tail(&map->vmm_list, &newvma->vma_plink);
    dbg(DBG_PRINT, "(GRADING3A)\n");
    return;
}

/* Find a contiguous range of free virtual pages of length npages in
 * the given address space. Returns starting vfn for the range,
 * without altering the map. Returns -1 if no such range exists.
 *
 * Your algorithm should be first fit. If dir is VMMAP_DIR_HILO, you
 * should find a gap as high in the address space as possible; if dir
 * is VMMAP_DIR_LOHI, the gap should be as low as possible. */
int vmmap_find_range(vmmap_t *map, uint32_t npages, int dir)
{
    // NOT_YET_IMPLEMENTED("VM: vmmap_find_range");
    // return -1;

    uint32_t result;
    int record = -1;
    uint32_t pre_end = ADDR_TO_PN(USER_MEM_LOW);
    uint32_t pre_start = ADDR_TO_PN(USER_MEM_HIGH);

    vmarea_t *tmp;

    // if (dir != VMMAP_DIR_HILO)
    // {
    //     list_iterate_begin(&map->vmm_list, tmp, vmarea_t, vma_plink)
    //     {
    //         if (record == -1 && pre_end > 0 &&
    //             tmp->vma_start - pre_end >= npages)
    //         {
    //             record = 0;
    //             result = pre_end;
    //             KASSERT(0==1);
    //         }
    //         pre_end = tmp->vma_end;
    //         KASSERT(0==1);
    //     }
    //     list_iterate_end();

    //     if (record == -1)
    //     {
    //         vmarea_t *tail_vma = list_tail(&map->vmm_list, vmarea_t, vma_plink);
    //         if (ADDR_TO_PN(USER_MEM_HIGH) - tail_vma->vma_end >= npages)
    //         {
    //             record = 0;
    //             result = tail_vma->vma_end;
    //             KASSERT(0==1);
    //         }
    //         KASSERT(0==1);
    //     }
    //     KASSERT(0==1);
    // }
    // else
    // {

    // }

    list_iterate_reverse(&map->vmm_list, tmp, vmarea_t, vma_plink)
    {
        if (record == -1 && pre_start - tmp->vma_end >= npages)
        {
            record = 0;
            result = pre_start - npages;
            dbg(DBG_PRINT, "(GRADING3A)\n");
        }
        pre_start = tmp->vma_start;
        dbg(DBG_PRINT, "(GRADING3A)\n");
    }
    list_iterate_end();

    if (record == -1)
    {
        vmarea_t *head_vma = list_head(&map->vmm_list, vmarea_t, vma_plink);
        if (head_vma->vma_start - ADDR_TO_PN(USER_MEM_LOW) >= npages)
        {
            record = 0;
            result = head_vma->vma_start - npages;
            dbg(DBG_PRINT, "(GRADING3D 2)\n");
        }
        dbg(DBG_PRINT, "(GRADING3D 2)\n");
    }
    dbg(DBG_PRINT, "(GRADING3A)\n");

    if (record == -1)
    {
        dbg(DBG_PRINT, "(GRADING3D 2)\n");
        return -1;
    }
    dbg(DBG_PRINT, "(GRADING3A)\n");
    return result;
}

/* Find the vm_area that vfn lies in. Simply scan the address space
 * looking for a vma whose range covers vfn. If the page is unmapped,
 * return NULL. */
vmarea_t *vmmap_lookup(vmmap_t *map, uint32_t vfn)
{
    // NOT_YET_IMPLEMENTED("VM: vmmap_lookup");
    // return NULL;
    KASSERT(NULL != map);
    dbg(DBG_PRINT, "(GRADING3A 3.c)\n");

    vmarea_t *vma_recorded = NULL;
    vmarea_t *vma;

    list_iterate_begin(&map->vmm_list, vma, vmarea_t, vma_plink)
    {
        if (vma_recorded == NULL && vma->vma_start <= vfn &&
            vma->vma_end > vfn)
        {
            vma_recorded = vma;
            dbg(DBG_PRINT, "(GRADING3A)\n");
        }
        dbg(DBG_PRINT, "(GRADING3A)\n");
    }
    list_iterate_end();
    dbg(DBG_PRINT, "(GRADING3A)\n");
    return vma_recorded;
}

/* Allocates a new vmmap containing a new vmarea for each area in the
 * given map. The areas should have no mmobjs set yet. Returns pointer
 * to the new vmmap on success, NULL on failure. This function is
 * called when implementing fork(2). */
vmmap_t *vmmap_clone(vmmap_t *map)
{
    // NOT_YET_IMPLEMENTED("VM: vmmap_clone");
    // return NULL;

    vmmap_t *newmap = vmmap_create();
    vmarea_t *tmp;

    list_init(&newmap->vmm_list);
    newmap->vmm_proc = map->vmm_proc;

    list_iterate_begin(&map->vmm_list, tmp, vmarea_t, vma_plink)
    {
        vmarea_t *newobj = vmarea_alloc();

        newobj->vma_obj = NULL;
        newobj->vma_start = tmp->vma_start;
        newobj->vma_end = tmp->vma_end;

        list_init(&newobj->vma_olink);

        newobj->vma_off = tmp->vma_off;
        newobj->vma_flags = tmp->vma_flags;

        list_init(&newobj->vma_plink);

        newobj->vma_prot = tmp->vma_prot;
        newobj->vma_vmmap = newmap;

        list_insert_tail(&newmap->vmm_list, &newobj->vma_plink);
        dbg(DBG_PRINT, "(GRADING3A)\n");
    }
    list_iterate_end();
    dbg(DBG_PRINT, "(GRADING3A)\n");
    return newmap;
}

/* Insert a mapping into the map starting at lopage for npages pages.
 * If lopage is zero, we will find a range of virtual addresses in the
 * process that is big enough, by using vmmap_find_range with the same
 * dir argument.  If lopage is non-zero and the specified region
 * contains another mapping that mapping should be unmapped.
 *
 * If file is NULL an anon mmobj will be used to create a mapping
 * of 0's.  If file is non-null that vnode's file will be mapped in
 * for the given range.  Use the vnode's mmap operation to get the
 * mmobj for the file; do not assume it is file->vn_obj. Make sure all
 * of the area's fields except for vma_obj have been set before
 * calling mmap.
 *
 * If MAP_PRIVATE is specified set up a shadow object for the mmobj.
 *
 * All of the input to this function should be valid (KASSERT!).
 * See mmap(2) for for description of legal input.
 * Note that off should be page aligned.
 *
 * Be very careful about the order operations are performed in here. Some
 * operation are impossible to undo and should be saved until there
 * is no chance of failure.
 *
 * If 'new' is non-NULL a pointer to the new vmarea_t should be stored in it.
 */
int vmmap_map(vmmap_t *map, vnode_t *file, uint32_t lopage, uint32_t npages,
              int prot, int flags, off_t off, int dir, vmarea_t **new)
{
    // NOT_YET_IMPLEMENTED("VM: vmmap_map");
    // return -1;

    KASSERT(NULL != map);
    KASSERT(0 < npages);
    KASSERT((MAP_SHARED & flags) || (MAP_PRIVATE & flags));
    KASSERT((0 == lopage) || (ADDR_TO_PN(USER_MEM_LOW) <= lopage));
    KASSERT((0 == lopage) || (ADDR_TO_PN(USER_MEM_HIGH) >= (lopage + npages)));
    KASSERT(PAGE_ALIGNED(off));
    dbg(DBG_PRINT, "(GRADING3A 3.d)\n");

    int range = 0;

    if (lopage == 0)
    {
        range = vmmap_find_range(map, npages, dir);
        if (range < 0)
        {
            dbg(DBG_PRINT, "(GRADING3D 2)\n");
            return range;
        }
        else
        {
            lopage = range;
            dbg(DBG_PRINT, "(GRADING3A)\n");
        }
        dbg(DBG_PRINT, "(GRADING3A)\n");
    }
    else
    {
        if (!vmmap_is_range_empty(map, lopage, npages))
        {
            vmmap_remove(map, lopage, npages);
            dbg(DBG_PRINT, "(GRADING3A)\n");
        }
        dbg(DBG_PRINT, "(GRADING3A)\n");
    }

    vmarea_t *vma = vmarea_alloc();

    vma->vma_end = lopage + npages;
    vma->vma_start = lopage;

    list_init(&vma->vma_plink);

    vma->vma_off = ADDR_TO_PN(off);
    vma->vma_flags = flags;

    list_init(&vma->vma_olink);

    vma->vma_prot = prot;
    vmmap_insert(map, vma);

    mmobj_t *o;
    // int errnum;
    if (file != NULL)
    {
        file->vn_ops->mmap(file, vma, &o);
        // if (errnum < 0)
        // {
        //     vmarea_free(vma);
        //     KASSERT(0==1);
        //     return errnum;
        // }
        // else
        // {
            
        // }
        list_insert_tail(&o->mmo_un.mmo_vmas, &vma->vma_olink);
            vma->vma_obj = o;

        dbg(DBG_PRINT, "(GRADING3A)\n");
    }
    else
    {
        o = anon_create();
        list_insert_tail(&o->mmo_un.mmo_vmas, &vma->vma_olink);
        vma->vma_obj = o;
        dbg(DBG_PRINT, "(GRADING3A)\n");
    }

    if (vma->vma_flags & MAP_PRIVATE)
    {
        mmobj_t *shadow_obj = shadow_create();
        shadow_obj->mmo_shadowed = o;

        mmobj_t *bottom_obj = mmobj_bottom_obj(o);
        shadow_obj->mmo_un.mmo_bottom_obj = bottom_obj;

        bottom_obj->mmo_ops->ref(bottom_obj);
        vma->vma_obj = shadow_obj;
        dbg(DBG_PRINT, "(GRADING3A)\n");
    }

    if (new == NULL)
    {
        dbg(DBG_PRINT, "(GRADING3A)\n");
        return 0;
    }
    else
    {
        *new = vma;
        dbg(DBG_PRINT, "(GRADING3A)\n");
    }
    dbg(DBG_PRINT, "(GRADING3A)\n");
    return 0;
}

/*
 * We have no guarantee that the region of the address space being
 * unmapped will play nicely with our list of vmareas.
 *
 * You must iterate over each vmarea that is partially or wholly covered
 * by the address range [addr ... addr+len). The vm-area will fall into one
 * of four cases, as illustrated below:
 *
 * key:
 *          [             ]   Existing VM Area
 *        *******             Region to be unmapped
 *
 * Case 1:  [   ******    ]
 * The region to be unmapped lies completely inside the vmarea. We need to
 * split the old vmarea into two vmareas. be sure to increment the
 * reference count to the file associated with the vmarea.
 *
 * Case 2:  [      *******]**
 * The region overlaps the end of the vmarea. Just shorten the length of
 * the mapping.
 *
 * Case 3: *[*****        ]
 * The region overlaps the beginning of the vmarea. Move the beginning of
 * the mapping (remember to update vma_off), and shorten its length.
 *
 * Case 4: *[*************]**
 * The region completely contains the vmarea. Remove the vmarea from the
 * list.
 */
int vmmap_remove(vmmap_t *map, uint32_t lopage, uint32_t npages)
{
    // NOT_YET_IMPLEMENTED("VM: vmmap_remove");
    vmarea_t *tmp;

    list_iterate_begin(&(map->vmm_list), tmp, vmarea_t, vma_plink)
    {
        // case 1
        if ((tmp->vma_start < lopage) && (tmp->vma_end > (lopage + npages)))
        {
            vmarea_t *newvma;
            newvma = vmarea_alloc();

            newvma->vma_start = (lopage + npages);
            newvma->vma_end = tmp->vma_end;

            list_init(&newvma->vma_olink);

            newvma->vma_off = tmp->vma_off + (lopage + npages) - tmp->vma_start;
            newvma->vma_flags = tmp->vma_flags;

            list_init(&newvma->vma_plink);

            newvma->vma_prot = tmp->vma_prot;
            newvma->vma_obj = tmp->vma_obj;

            vmmap_insert(map, newvma);

            newvma->vma_obj->mmo_ops->ref(newvma->vma_obj);
            tmp->vma_end = lopage;
            dbg(DBG_PRINT, "(GRADING3D 2)\n");
        }

        // case 2
        if (tmp->vma_start < lopage && tmp->vma_end > lopage &&
            tmp->vma_end <= (lopage + npages))
        {
            tmp->vma_end = lopage;
            dbg(DBG_PRINT, "(GRADING3D 2)\n");
        }

        // case 3
        if (tmp->vma_end > (lopage + npages) &&
            tmp->vma_start < (lopage + npages) && tmp->vma_start >= lopage)
        {
            tmp->vma_off = tmp->vma_off + (lopage + npages) - (tmp->vma_start);
            tmp->vma_start = (lopage + npages);
            dbg(DBG_PRINT, "(GRADING3D 2)\n");;
        }

        // case 4
        if (tmp->vma_start >= lopage && tmp->vma_end <= (lopage + npages))
        {
            tmp->vma_obj->mmo_ops->put(tmp->vma_obj);

            if (list_link_is_linked(&(tmp->vma_plink)))
            {
                list_remove(&(tmp->vma_plink));
                dbg(DBG_PRINT, "(GRADING3A)\n");
            }

            if (list_link_is_linked(&(tmp->vma_olink)))
            {
                list_remove(&(tmp->vma_olink));
                dbg(DBG_PRINT, "(GRADING3A)\n");
            }

            vmarea_free(tmp);
            dbg(DBG_PRINT, "(GRADING3A)\n");
        }
        dbg(DBG_PRINT, "(GRADING3A)\n");
    }
    list_iterate_end();

    tlb_flush_all();
    pt_unmap_range(curproc->p_pagedir, (uintptr_t)PN_TO_ADDR(lopage),
                   (uintptr_t)PN_TO_ADDR(lopage + npages));
    dbg(DBG_PRINT, "(GRADING3A)\n");
    return 0;
}

/*
 * Returns 1 if the given address space has no mappings for the
 * given range, 0 otherwise.
 */
int vmmap_is_range_empty(vmmap_t *map, uint32_t startvfn, uint32_t npages)
{
    // NOT_YET_IMPLEMENTED("VM: vmmap_is_range_empty");
    // return 0;

    uint32_t endvfn = startvfn + npages;

    KASSERT((startvfn < endvfn) && (ADDR_TO_PN(USER_MEM_LOW) <= startvfn) &&
            (ADDR_TO_PN(USER_MEM_HIGH) >= endvfn));

    dbg(DBG_PRINT, "(GRADING3A 3.e)\n");

    int record = 1;
    vmarea_t *tmp;

    list_iterate_begin(&map->vmm_list, tmp, vmarea_t, vma_plink)
    {
        if (tmp->vma_start < (startvfn + npages) && tmp->vma_end > startvfn)
        {
            record = 0;
            dbg(DBG_PRINT, "(GRADING3A)\n");
        }
        dbg(DBG_PRINT, "(GRADING3A)\n");
    }
    list_iterate_end();
    dbg(DBG_PRINT, "(GRADING3A)\n");

    return record;
}

/* Read into 'buf' from the virtual address space of 'map' starting at
 * 'vaddr' for size 'count'. To do so, you will want to find the vmareas
 * to read from, then find the pframes within those vmareas corresponding
 * to the virtual addresses you want to read, and then read from the
 * physical memory that pframe points to. You should not check permissions
 * of the areas. Assume (KASSERT) that all the areas you are accessing exist.
 * Returns 0 on success, -errno on error.
 */
int vmmap_read(vmmap_t *map, const void *vaddr, void *buf, size_t count)
{
    // NOT_YET_IMPLEMENTED("VM: vmmap_read");
    // return 0;

    uint32_t cur = (uint32_t)vaddr;
    uint32_t end = (uint32_t)vaddr + count;
    uint32_t readedBytes = 0;

    while (end > cur)
    {
        vmarea_t *vma = vmmap_lookup(map, ADDR_TO_PN(cur));

        // if (vma == NULL)
        // {
        //     KASSERT(0==1);
        //     return -curthr->kt_errno;
        // }

        pframe_t *pf;
        mmobj_t *o = vma->vma_obj;
        uint32_t pagenum = ADDR_TO_PN(cur) - vma->vma_start + vma->vma_off;

o->mmo_ops->lookuppage(o, pagenum, 0, &pf);
        // if (o->mmo_ops->lookuppage(o, pagenum, 0, &pf) != 0)
        // {
        //     KASSERT(0==1);
        //     return -curthr->kt_errno;
        // }

        memcpy((char *)buf + readedBytes,
               (char *)pf->pf_addr + PAGE_OFFSET(cur),
               (uint32_t)MIN(count - readedBytes, PAGE_SIZE - PAGE_OFFSET(cur)));

        uint32_t toRead = MIN(count - readedBytes, PAGE_SIZE - PAGE_OFFSET(cur));

        readedBytes += (uint32_t)MIN(count - readedBytes, PAGE_SIZE - PAGE_OFFSET(cur));

        cur += toRead;
        dbg(DBG_PRINT, "(GRADING3A)\n");
    }
    dbg(DBG_PRINT, "(GRADING3A)\n");
    return 0;
}

/* Write from 'buf' into the virtual address space of 'map' starting at
 * 'vaddr' for size 'count'. To do this, you will need to find the correct
 * vmareas to write into, then find the correct pframes within those vmareas,
 * and finally write into the physical addresses that those pframes correspond
 * to. You should not check permissions of the areas you use. Assume (KASSERT)
 * that all the areas you are accessing exist. Remember to dirty pages!
 * Returns 0 on success, -errno on error.
 */
int vmmap_write(vmmap_t *map, void *vaddr, const void *buf, size_t count)
{
    // NOT_YET_IMPLEMENTED("VM: vmmap_write");
    // return 0;

    uint32_t cur = (uint32_t)vaddr;
    uint32_t end = (uint32_t)vaddr + count;
    uint32_t writtenBytes = 0;

    while (end > cur)
    {
        vmarea_t *vma = vmmap_lookup(map, ADDR_TO_PN(cur));

        // if (vma == NULL)
        // {
        //     curthr->kt_errno = 1;
        //     KASSERT(0==1);
        //     return -curthr->kt_errno;
        // }

        pframe_t *pf;
        mmobj_t *o = vma->vma_obj;
        uint32_t pagenum = ADDR_TO_PN(cur) - vma->vma_start + vma->vma_off;

        int err = o->mmo_ops->lookuppage(o, pagenum, 1, &pf);
        // if (err != 0)
        // {
        //     curthr->kt_errno = 1;
        //     KASSERT(0==1);
        //     return -curthr->kt_errno;
        // }

        memcpy((char *)pf->pf_addr + PAGE_OFFSET(cur), (char *)buf + writtenBytes, (uint32_t)MIN(count - writtenBytes, PAGE_SIZE - PAGE_OFFSET(cur)));

        uint32_t toWrite = MIN(count - writtenBytes, PAGE_SIZE - PAGE_OFFSET(cur));

        pframe_dirty(pf);

        writtenBytes += (uint32_t)MIN(count - writtenBytes, PAGE_SIZE - PAGE_OFFSET(cur));
        cur += toWrite;
        dbg(DBG_PRINT, "(GRADING3A)\n");
    }
    dbg(DBG_PRINT, "(GRADING3A)\n");

    return 0;
}

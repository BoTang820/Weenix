#include "globals.h"
#include "errno.h"

#include "util/string.h"
#include "util/debug.h"

#include "mm/mmobj.h"
#include "mm/pframe.h"
#include "mm/mm.h"
#include "mm/page.h"
#include "mm/slab.h"
#include "mm/tlb.h"

#include "vm/vmmap.h"
#include "vm/shadow.h"
#include "vm/shadowd.h"

#define SHADOW_SINGLETON_THRESHOLD 5

int shadow_count = 0; /* for debugging/verification purposes */
#ifdef __SHADOWD__
/*
 * number of shadow objects with a single parent, that is another shadow
 * object in the shadow objects tree(singletons)
 */
static int shadow_singleton_count = 0;
#endif

static slab_allocator_t *shadow_allocator;

static void shadow_ref(mmobj_t *o);
static void shadow_put(mmobj_t *o);
static int shadow_lookuppage(mmobj_t *o, uint32_t pagenum, int forwrite, pframe_t **pf);
static int shadow_fillpage(mmobj_t *o, pframe_t *pf);
static int shadow_dirtypage(mmobj_t *o, pframe_t *pf);
static int shadow_cleanpage(mmobj_t *o, pframe_t *pf);

static mmobj_ops_t shadow_mmobj_ops = {
    .ref = shadow_ref,
    .put = shadow_put,
    .lookuppage = shadow_lookuppage,
    .fillpage = shadow_fillpage,
    .dirtypage = shadow_dirtypage,
    .cleanpage = shadow_cleanpage};

/*
 * This function is called at boot time to initialize the
 * shadow page sub system. Currently it only initializes the
 * shadow_allocator object.
 */
void shadow_init()
{
        // NOT_YET_IMPLEMENTED("VM: shadow_init");
        shadow_allocator = slab_allocator_create("shadow", sizeof(mmobj_t));
        KASSERT(shadow_allocator);
        dbg(DBG_PRINT, "(GRADING3A 6.a)\n");
        dbg(DBG_PRINT, "(GRADING3A)\n");
}

/*
 * You'll want to use the shadow_allocator to allocate the mmobj to
 * return, then then initialize it. Take a look in mm/mmobj.h for
 * macros or functions which can be of use here. Make sure your initial
 * reference count is correct.
 */
mmobj_t *
shadow_create()
{
        // NOT_YET_IMPLEMENTED("VM: shadow_create");
        // return NULL;

        mmobj_t *newobj = NULL;
        newobj = (mmobj_t *)slab_obj_alloc(shadow_allocator);
        mmobj_init(newobj, &shadow_mmobj_ops);

        newobj->mmo_refcount = 1;
        newobj->mmo_un.mmo_bottom_obj = NULL;
        dbg(DBG_PRINT, "(GRADING3A)\n");
        return newobj;
}

/* Implementation of mmobj entry points: */

/*
 * Increment the reference count on the object.
 */
static void
shadow_ref(mmobj_t *o)
{
        // NOT_YET_IMPLEMENTED("VM: shadow_ref");

        KASSERT(o && (0 < o->mmo_refcount) && (&shadow_mmobj_ops == o->mmo_ops));
        dbg(DBG_PRINT, "(GRADING3A 6.b)\n");
        o->mmo_refcount += 1;
        dbg(DBG_PRINT, "(GRADING3A)\n");
}

/*
 * Decrement the reference count on the object. If, however, the
 * reference count on the object reaches the number of resident
 * pages of the object, we can conclude that the object is no
 * longer in use and, since it is a shadow object, it will never
 * be used again. You should unpin and uncache all of the object's
 * pages and then free the object itself.
 */
static void
shadow_put(mmobj_t *o)
{
        // NOT_YET_IMPLEMENTED("VM: shadow_put");
        KASSERT(o && (0 < o->mmo_refcount) && (&shadow_mmobj_ops == o->mmo_ops));
        dbg(DBG_PRINT, "(GRADING3A 6.c)\n");

        if (o->mmo_nrespages != (o->mmo_refcount - 1))
        {
                o->mmo_refcount -= 1;
                dbg(DBG_PRINT, "(GRADING3A)\n");
                return;
        }

        pframe_t *tmp;
        tmp = NULL;
        list_iterate_begin(&o->mmo_respages, tmp, pframe_t, pf_olink)
        {
                if (pframe_is_pinned(tmp))
                {
                        pframe_unpin(tmp);
                        dbg(DBG_PRINT, "(GRADING3A)\n");
                }
                pframe_free(tmp);
                dbg(DBG_PRINT, "(GRADING3A)\n");
        }
        list_iterate_end();
        // remember to put the shadowed and bottom mmobj
        o->mmo_refcount -= 1;

        o->mmo_shadowed->mmo_ops->put(o->mmo_shadowed);
        o->mmo_un.mmo_bottom_obj->mmo_ops->put(o->mmo_un.mmo_bottom_obj);
        // and then free the object itself
        slab_obj_free(shadow_allocator, o);
        dbg(DBG_PRINT, "(GRADING3A)\n");
}

/* This function looks up the given page in this shadow object. The
 * forwrite argument is true if the page is being looked up for
 * writing, false if it is being looked up for reading. This function
 * must handle all do-not-copy-on-not-write magic (i.e. when forwrite
 * is false find the first shadow object in the chain which has the
 * given page resident). copy-on-write magic (necessary when forwrite
 * is true) is handled in shadow_fillpage, not here. It is important to
 * use iteration rather than recursion here as a recursive implementation
 * can overflow the kernel stack when looking down a long shadow chain */
static int
shadow_lookuppage(mmobj_t *o, uint32_t pagenum, int forwrite, pframe_t **pf)
{
        // NOT_YET_IMPLEMENTED("VM: shadow_lookuppage");

        pframe_t *tempPf = NULL;
        int err = 0;
        if (forwrite == 0)
        {
                mmobj_t *cur = o;
                while (cur->mmo_shadowed != NULL)
                {
                        tempPf = pframe_get_resident(cur, pagenum);

                        if (tempPf == NULL)
                        {
                                cur = cur->mmo_shadowed;
                                dbg(DBG_PRINT, "(GRADING3A)\n");
                                continue;
                        }
                        else
                        {
                                *pf = tempPf;
                                dbg(DBG_PRINT, "(GRADING3A)\n");
                                break;
                        }
                }
                if (tempPf == NULL)
                {
                        err = pframe_lookup(o->mmo_un.mmo_bottom_obj, pagenum, 0, pf);
                        if (err < 0)
                        {
                                dbg(DBG_PRINT, "(GRADING3D 2)\n");
                                return err;
                        }
                        dbg(DBG_PRINT, "(GRADING3A)\n");
                }
                dbg(DBG_PRINT, "(GRADING3A)\n");
        }
        else
        {
                tempPf = pframe_get_resident(o, pagenum);
                if (tempPf != NULL)
                {
                        *pf = tempPf;
                        dbg(DBG_PRINT, "(GRADING3A)\n");
                }
                else
                {
                        err = pframe_get(o, pagenum, pf);
                        if (err < 0)
                        {
                                dbg(DBG_PRINT, "(GRADING3D 2)\n");
                                return err;
                        }
                        pframe_dirty(*pf);
                        dbg(DBG_PRINT, "(GRADING3A)\n");
                }
                dbg(DBG_PRINT, "(GRADING3A)\n");
        }

        KASSERT(NULL != (*pf));
        KASSERT((pagenum == (*pf)->pf_pagenum) && (!pframe_is_busy(*pf)));
        dbg(DBG_PRINT, "(GRADING3A 6.d)\n");
        dbg(DBG_PRINT, "(GRADING3A)\n");
        return 0;
}

/* As per the specification in mmobj.h, fill the page frame starting
 * at address pf->pf_addr with the contents of the page identified by
 * pf->pf_obj and pf->pf_pagenum. This function handles all
 * copy-on-write magic (i.e. if there is a shadow object which has
 * data for the pf->pf_pagenum-th page then we should take that data,
 * if no such shadow object exists we need to follow the chain of
 * shadow objects all the way to the bottom object and take the data
 * for the pf->pf_pagenum-th page from the last object in the chain).
 * It is important to use iteration rather than recursion here as a
 * recursive implementation can overflow the kernel stack when
 * looking down a long shadow chain */
static int
shadow_fillpage(mmobj_t *o, pframe_t *pf)
{
        // NOT_YET_IMPLEMENTED("VM: shadow_fillpage");

        KASSERT(pframe_is_busy(pf));
        KASSERT(!pframe_is_pinned(pf));
        dbg(DBG_PRINT, "(GRADING3A 6.e)\n");

        pframe_t *shadowpf;
        shadowpf = NULL;
        mmobj_t *cur = o;

        while (cur != NULL)
        {
                shadowpf = pframe_get_resident(cur->mmo_shadowed, pf->pf_pagenum);
                if (shadowpf == NULL)
                {
                        cur = cur->mmo_shadowed;
                        dbg(DBG_PRINT, "(GRADING3A)\n");
                        continue;
                }
                else
                {
                        memcpy(pf->pf_addr, shadowpf->pf_addr, PAGE_SIZE);
                        pframe_pin(pf);
                        dbg(DBG_PRINT, "(GRADING3A)\n");
                        break;
                }
        }

        if (shadowpf != NULL)
        {
                dbg(DBG_PRINT, "(GRADING3A)\n");
                return 0;
        }

        int err = 0;
        err = pframe_lookup(o->mmo_un.mmo_bottom_obj, pf->pf_pagenum, 0, &shadowpf);
        if (err >= 0)
        {
                memcpy(pf->pf_addr, shadowpf->pf_addr, PAGE_SIZE);
                pframe_pin(pf);
                dbg(DBG_PRINT, "(GRADING3A)\n");
                return 0;
        }
        else
        {
                dbg(DBG_PRINT, "(GRADING3D 2)\n");
                return err;
        }
}

/* These next two functions are not difficult. */

static int
shadow_dirtypage(mmobj_t *o, pframe_t *pf)
{
        // NOT_YET_IMPLEMENTED("VM: shadow_dirtypage");
        // return -1;
        dbg(DBG_PRINT, "(GRADING3A)\n");
        return 0;
}

static int
shadow_cleanpage(mmobj_t *o, pframe_t *pf)
{
        // NOT_YET_IMPLEMENTED("VM: shadow_cleanpage");
        // return -1;
        // KASSERT(0==1);
        return 0;
}

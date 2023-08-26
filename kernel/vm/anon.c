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

int anon_count = 0; /* for debugging/verification purposes */

static slab_allocator_t *anon_allocator;

static void anon_ref(mmobj_t *o);
static void anon_put(mmobj_t *o);
static int anon_lookuppage(mmobj_t *o, uint32_t pagenum, int forwrite, pframe_t **pf);
static int anon_fillpage(mmobj_t *o, pframe_t *pf);
static int anon_dirtypage(mmobj_t *o, pframe_t *pf);
static int anon_cleanpage(mmobj_t *o, pframe_t *pf);

static mmobj_ops_t anon_mmobj_ops = {
    .ref = anon_ref,
    .put = anon_put,
    .lookuppage = anon_lookuppage,
    .fillpage = anon_fillpage,
    .dirtypage = anon_dirtypage,
    .cleanpage = anon_cleanpage};

/*
 * This function is called at boot time to initialize the
 * anonymous page sub system. Currently it only initializes the
 * anon_allocator object.
 */
void anon_init()
{
    // NOT_YET_IMPLEMENTED("VM: anon_init");
    anon_allocator = slab_allocator_create("anon_allocator", sizeof(mmobj_t));
    KASSERT(anon_allocator); /* after initialization, anon_allocator must not be
                                NULL */
    dbg(DBG_PRINT, "(GRADING3A 4.a)\n");
    dbg(DBG_PRINT, "(GRADING3A)\n");
}

/*
 * You'll want to use the anon_allocator to allocate the mmobj to
 * return, then initialize it. Take a look in mm/mmobj.h for
 * definitions which can be of use here. Make sure your initial
 * reference count is correct.
 */
mmobj_t *anon_create()
{
    //     NOT_YET_IMPLEMENTED("VM: anon_create");

    mmobj_t *obj = NULL;
    obj = (mmobj_t *)slab_obj_alloc(anon_allocator);

    if (obj != NULL)
    {
        mmobj_init(obj, &anon_mmobj_ops);
        obj->mmo_refcount = 1;
        dbg(DBG_PRINT, "(GRADING3A)\n");
    }
    dbg(DBG_PRINT, "(GRADING3A)\n");
    return obj;
}

/* Implementation of mmobj entry points: */

/*
 * Increment the reference count on the object.
 */
static void anon_ref(mmobj_t *o)
{
    //     NOT_YET_IMPLEMENTED("VM: anon_ref");

    KASSERT(o && (0 < o->mmo_refcount) && (&anon_mmobj_ops == o->mmo_ops));
    /* the o function argument must be non-NULL, has a positive refcount, and is
     * an anonymous object */
    dbg(DBG_PRINT, "(GRADING3A 4.b)\n");
    o->mmo_refcount += 1;
    dbg(DBG_PRINT, "(GRADING3A)\n");
}

/*
 * Decrement the reference count on the object. If, however, the
 * reference count on the object reaches the number of resident
 * pages of the object, we can conclude that the object is no
 * longer in use and, since it is an anonymous object, it will
 * never be used again. You should unpin and uncache all of the
 * object's pages and then free the object itself.
 */
static void anon_put(mmobj_t *o)
{
    KASSERT(o && (0 < o->mmo_refcount) && (&anon_mmobj_ops == o->mmo_ops));
    dbg(DBG_PRINT, "(GRADING3A 4.c)\n");
    if (o->mmo_refcount == o->mmo_nrespages + 1)
    {
        pframe_t *pframe = NULL;
        list_iterate_begin(&o->mmo_respages, pframe, pframe_t, pf_olink)
        {
            if (pframe_is_pinned(pframe))
            {
                pframe_unpin(pframe);
                dbg(DBG_PRINT, "(GRADING3A)\n");
            }
            pframe_free(pframe);
            dbg(DBG_PRINT, "(GRADING3A)\n");
        }
        list_iterate_end();
        slab_obj_free(anon_allocator, o);
        dbg(DBG_PRINT, "(GRADING3A)\n");
    }
    o->mmo_refcount--;
    dbg(DBG_PRINT, "(GRADING3A)\n");
}

/* Get the corresponding page from the mmobj. No special handling is
 * required. */
static int anon_lookuppage(mmobj_t *o, uint32_t pagenum, int forwrite,
                           pframe_t **pf)
{
    //     NOT_YET_IMPLEMENTED("VM: anon_lookuppage");
    dbg(DBG_PRINT, "(GRADING3A)\n");
    return pframe_get(o, pagenum, pf);
}

/* The following three functions should not be difficult. */

static int anon_fillpage(mmobj_t *o, pframe_t *pf)
{
    KASSERT(pframe_is_busy(pf)); /* can only "fill" a page frame when the page
                                    frame is in the "busy" state */
    KASSERT(!pframe_is_pinned(
        pf)); /* must not fill a page frame that's already pinned */
    dbg(DBG_PRINT, "(GRADING3A 4.d)\n");

    pframe_pin(pf);
    memset(pf->pf_addr, 0, PAGE_SIZE);
    dbg(DBG_PRINT, "(GRADING3A)\n");

    return 0;
}

static int anon_dirtypage(mmobj_t *o, pframe_t *pf)
{
    //     NOT_YET_IMPLEMENTED("VM: anon_dirtypage");

    pframe_set_dirty(pf);
    dbg(DBG_PRINT, "(GRADING3D 1)\n");
    return 0;
}

static int anon_cleanpage(mmobj_t *o, pframe_t *pf)
{
    // NOT_YET_IMPLEMENTED("VM: anon_cleanpage");
    // KASSERT(0==1);
    return 0;
}

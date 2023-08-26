#include "globals.h"
#include "errno.h"
#include "fs/fcntl.h"
#include "util/string.h"
#include "util/printf.h"
#include "fs/vfs.h"
#include "fs/vnode.h"
#include "fs/file.h"
#include "fs/vfs_syscall.h"
#include "fs/open.h"
#include "fs/stat.h"
#include "util/debug.h"

/* find empty index in p->p_files[] */
int get_empty_fd(proc_t *p)
{
        int fd;

        for (fd = 0; fd < NFILES; fd++)
        {
                if (!p->p_files[fd])
                        return fd;
        }

        dbg(DBG_ERROR | DBG_VFS, "ERROR: get_empty_fd: out of file descriptors "
                                 "for pid %d\n",
            curproc->p_pid);
        return -EMFILE;
}

/*
 * There a number of steps to opening a file:
 *      1. Get the next empty file descriptor.
 *      2. Call fget to get a fresh file_t.
 *      3. Save the file_t in curproc's file descriptor table.
 *      4. Set file_t->f_mode to OR of FMODE_(READ|WRITE|APPEND) based on
 *         oflags, which can be O_RDONLY, O_WRONLY or O_RDWR, possibly OR'd with
 *         O_APPEND.
 *      5. Use open_namev() to get the vnode for the file_t.
 *      6. Fill in the fields of the file_t.
 *      7. Return new fd.
 *
 * If anything goes wrong at any point (specifically if the call to open_namev
 * fails), be sure to remove the fd from curproc, fput the file_t and return an
 * error.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EINVAL
 *        oflags is not valid.
 *      o EMFILE
 *        The process already has the maximum number of files open.
 *      o ENOMEM
 *        Insufficient kernel memory was available.
 *      o ENAMETOOLONG
 *        A component of filename was too long.
 *      o ENOENT
 *        O_CREAT is not set and the named file does not exist.  Or, a
 *        directory component in pathname does not exist.
 *      o EISDIR
 *        pathname refers to a directory and the access requested involved
 *        writing (that is, O_WRONLY or O_RDWR is set).
 *      o ENXIO
 *        pathname refers to a device special file and no corresponding device
 *        exists.
 */

int do_open(const char *filename, int oflags)
{
        // NOT_YET_IMPLEMENTED("VFS: do_open");

        // check oflags to pass vfstest( only invalid possible combination is O_WRONLY|O_RDWR. )
        if ((oflags & O_WRONLY) && (oflags & O_RDWR))
        {
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return -EINVAL;
        }
        // Get the next empty file descriptor
        int fd = get_empty_fd(curproc);

        // if (fd < 0)
        // {
        //         dbg(DBG_PRINT, "(GRADING2C)\n");
        //         return -EMFILE; // Maximum number of files open
        // }

        // Call fget to get a fresh file_t
        file_t *file = fget(-1);
        // Save the file_t in curproc's file descriptor table
        curproc->p_files[fd] = file;

        // Set file_t->f_mode based on oflags
        int mode = 0;
        if (oflags == O_RDONLY)
        {

                mode |= FMODE_READ;
                dbg(DBG_PRINT, "(GRADING2B)\n");
        }
        if ((oflags & O_WRONLY) != 0)
        {
                mode |= FMODE_WRITE;
                dbg(DBG_PRINT, "(GRADING2B)\n");
        }
        if ((oflags & O_RDWR) != 0)
        {
                mode |= FMODE_READ | FMODE_WRITE;
                dbg(DBG_PRINT, "(GRADING2B)\n");
        }
        if ((oflags & O_APPEND) != 0)
        {
                mode |= FMODE_APPEND;
                dbg(DBG_PRINT, "(GRADING2B)\n");
        }
        file->f_mode = mode;

        // Use open_namev() to get the vnode for the file_t
        vnode_t *vnode;
        vnode = NULL;
        int res = open_namev(filename, oflags, &vnode, NULL);

        if (res < 0)
        {
                fput(file);
                curproc->p_files[fd] = NULL;
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return res;
        }
        else if (S_ISDIR(vnode->vn_mode) && ((oflags & O_WRONLY) || (oflags & O_RDWR)))
        {
                // decrease resVNode vn_refcount
                vput(vnode);
                // fput the newFile
                fput(file);
                // remove the newfd from curproc
                curproc->p_files[fd] = NULL;
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return -EISDIR;
        }

        // Fill in the fields of the file_t
        file->f_vnode = vnode;
        file->f_pos = 0;

        // Return new fd
        dbg(DBG_PRINT, "(GRADING2B)\n");
        return fd;
}
#include "kernel.h"
#include "errno.h"
#include "globals.h"
#include "fs/vfs.h"
#include "fs/file.h"
#include "fs/vnode.h"
#include "fs/vfs_syscall.h"
#include "fs/open.h"
#include "fs/fcntl.h"
#include "fs/lseek.h"
#include "mm/kmalloc.h"
#include "util/string.h"
#include "util/printf.h"
#include "fs/stat.h"
#include "util/debug.h"

/*
 * Syscalls for vfs. Refer to comments or man pages for implementation.
 * Do note that you don't need to set errno, you should just return the
 * negative error code.
 */

/* To read a file:
 *      o fget(fd)
 *      o call its virtual read vn_op
 *      o update f_pos
 *      o fput() it
 *      o return the number of bytes read, or an error
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        fd is not a valid file descriptor or is not open for reading.
 *      o EISDIR
 *        fd refers to a directory.
 *
 * In all cases, be sure you do not leak file refcounts by returning before
 * you fput() a file that you fget()'ed.
 */
int do_read(int fd, void *buf, size_t nbytes)
{
        // NOT_YET_IMPLEMENTED("VFS: do_read");
        file_t *file = fget(fd);

        // if (nbytes == 0) // if nbytes is 0, return 0
        // {
        //         fput(file);
        //         dbg(DBG_PRINT, "(GRADING2B)\n");
        //         return 0;
        // }
        if (file == NULL) // if file is NULL, return -EBADF
        {
                dbg(DBG_PRINT, "(GRADING2B)\n");

                return -EBADF;
        }
        if (file->f_vnode != NULL && S_ISDIR(file->f_vnode->vn_mode)) // if file is a directory, return -EISDIR
        {
                fput(file);
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return -EISDIR;
        }
        if ((file->f_mode & FMODE_READ) == 0) // if file is not open for reading, return -EBADF
        {
                fput(file);
                dbg(DBG_PRINT, "(GRADING2B)\n");

                return -EBADF;
        }

        // call its virtual read vn_op
        int bytes_read = file->f_vnode->vn_ops->read(file->f_vnode, file->f_pos, buf, nbytes);

        if (bytes_read >= 0) // if bytes_read is greater than or equal to 0, update f_pos
        {
                file->f_pos += bytes_read;
                dbg(DBG_PRINT, "(GRADING2B)\n");
        }
        fput(file); // fput() it
        dbg(DBG_PRINT, "(GRADING2B)\n");
        return bytes_read; // return the number of bytes read, or an error
}

/* Very similar to do_read.  Check f_mode to be sure the file is writable.  If
 * f_mode & FMODE_APPEND, do_lseek() to the end of the file, call the write
 * vn_op, and fput the file.  As always, be mindful of refcount leaks.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        fd is not a valid file descriptor or is not open for writing.
 */
int do_write(int fd, const void *buf, size_t nbytes)
{
        // NOT_YET_IMPLEMENTED("VFS: do_write");
        file_t *file = fget(fd);
        if (file == NULL) // if file is NULL, return -EBADF
        {
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return -EBADF;
        }
        if ((file->f_mode & FMODE_WRITE) == 0) // if file is not open for writing, return -EBADF
        {

                fput(file);
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return -EBADF;
        }

        // check
        if (file->f_mode & FMODE_APPEND) // if f_mode & FMODE_APPEND, do_lseek() to the end of the file
        {
                dbg(DBG_PRINT, "(GRADING2B)\n");
                do_lseek(fd, 0, SEEK_END);
        }
        int bytes_written = file->f_vnode->vn_ops->write(file->f_vnode, file->f_pos, buf, nbytes);
        if (bytes_written >= 0) // if bytes_written is greater than or equal to 0, update f_pos
        {

                file->f_pos += bytes_written;
                KASSERT((S_ISCHR(file->f_vnode->vn_mode)) ||
                        (S_ISBLK(file->f_vnode->vn_mode)) ||
                        ((S_ISREG(file->f_vnode->vn_mode)) && (file->f_pos <= file->f_vnode->vn_len)));
                dbg(DBG_PRINT, "(GRADING2A 3.a)\n");
                dbg(DBG_PRINT, "(GRADING2B)\n");
        }
        fput(file); // fput() it
        dbg(DBG_PRINT, "(GRADING2B)\n");
        return bytes_written;
}

/*
 * Zero curproc->p_files[fd], and fput() the file. Return 0 on success
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        fd isn't a valid open file descriptor.
 */
int do_close(int fd)
{

        // NOT_YET_IMPLEMENTED("VFS: do_close");
        if (fd < 0 || fd >= NFILES) // if fd is less than 0 or greater than or equal to NFILES, return -EBADF
        {
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return -EBADF;
        }
        if (NULL == curproc->p_files[fd]) // if curproc->p_files[fd] is NULL, return -EBADF
        {
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return -EBADF;
        }

        fput(curproc->p_files[fd]); // fput() the file
        // Zero curproc->p_files[fd]
        curproc->p_files[fd] = NULL; // set curproc->p_files[fd] to NULL
        dbg(DBG_PRINT, "(GRADING2B)\n");
        return 0;
}

/* To dup a file:
 *      o fget(fd) to up fd's refcount
 *      o get_empty_fd()
 *      o point the new fd to the same file_t* as the given fd
 *      o return the new file descriptor
 *
 * Don't fput() the fd unless something goes wrong.  Since we are creating
 * another reference to the file_t*, we want to up the refcount.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        fd isn't an open file descriptor.
 *      o EMFILE
 *        The process already has the maximum number of file descriptors open
 *        and tried to open a new one.
 */
int do_dup(int fd)
{

        // NOT_YET_IMPLEMENTED("VFS: do_dup");
        if (fd < 0 || fd >= NFILES) // if fd is less than 0 or greater than or equal to NFILES, return -EBADF
        {
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return -EBADF;
        }
        file_t *file = fget(fd);
        if (file == NULL) // if file is NULL, return -EBADF
        {
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return -EBADF;
        }
        int new_fd = get_empty_fd(curproc);
        // if (new_fd < 0) // if new_fd is less than 0, return -EMFILE
        // {
        //         dbg(DBG_PRINT, "(GRADING2B)\n");
        //         fput(file);
        //         return -EMFILE;
        // }
        curproc->p_files[new_fd] = file; // point the new fd to the same file_t* as the given fd
        dbg(DBG_PRINT, "(GRADING2B)\n");
        return new_fd; // return the new file descriptor
}

/* Same as do_dup, but insted of using get_empty_fd() to get the new fd,
 * they give it to us in 'nfd'.  If nfd is in use (and not the same as ofd)
 * do_close() it first.  Then return the new file descriptor.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        ofd isn't an open file descriptor, or nfd is out of the allowed
 *        range for file descriptors.
 */
int do_dup2(int ofd, int nfd)
{

        // NOT_YET_IMPLEMENTED("VFS: do_dup2");
        if (ofd < 0 || ofd >= NFILES) // if ofd is less than 0 or greater than or equal to NFILES, return -EBADF
        {
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return -EBADF;
        }
        if (nfd < 0 || nfd >= NFILES) // ifnfd is less than 0 or greater than or equal to NFILES, return -EBADF
        {
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return -EBADF;
        }
        file_t *file = fget(ofd);
        if (file == NULL) // if file is NULL, return -EBADF
        {
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return -EBADF;
        }
        // if (curproc->p_files[nfd] != NULL && nfd != ofd) // if curproc->p_files[nfd] is not NULL and not the same as ofd, do_close() it first
        // {
        //         dbg(DBG_PRINT, "(GRADING2B)\n");
        //         do_close(nfd);
        // }
        curproc->p_files[nfd] = file; // point the new fd to the same file_t* as the given fd
        if (nfd == ofd)
        {
                dbg(DBG_PRINT, "(GRADING2B)\n");
                fput(file);
        }
        dbg(DBG_PRINT, "(GRADING2B)\n");
        return nfd; // return the new file descriptor
}

/*
 * This routine creates a special file of the type specified by 'mode' at
 * the location specified by 'path'. 'mode' should be one of S_IFCHR or
 * S_IFBLK (you might note that mknod(2) normally allows one to create
 * regular files as well-- for simplicity this is not the case in Weenix).
 * 'devid', as you might expect, is the device identifier of the device
 * that the new special file should represent.
 *
 * You might use a combination of dir_namev, lookup, and the fs-specific
 * mknod (that is, the containing directory's 'mknod' vnode operation).
 * Return the result of the fs-specific mknod, or an error.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EINVAL
 *        mode requested creation of something other than a device special
 *        file.
 *      o EEXIST
 *        path already exists.
 *      o ENOENT
 *        A directory component in path does not exist.
 *      o ENOTDIR
 *        A component used as a directory in path is not, in fact, a directory.
 *      o ENAMETOOLONG
 *        A component of path was too long.
 */
int do_mknod(const char *path, int mode, unsigned devid)
{
        // NOT_YET_IMPLEMENTED("VFS: do_mknod");
        // if (mode != S_IFCHR && mode != S_IFBLK) // if mode is not S_IFCHR or S_IFBLK, return -EINVAL
        // {

        //         dbg(DBG_PRINT, "(GRADING2B)\n");
        //         return -EINVAL;
        // }
        KASSERT(S_ISCHR(mode) || S_ISBLK(mode));
        size_t namelen = 0;
        const char *name = NULL;
        vnode_t *dir_vnode = NULL;
        int res = dir_namev(path, &namelen, &name, NULL, &dir_vnode); // if success, refcount of dir_vnode will be incremented
        // if (res != 0)                                                 // if res is not 0, dir_namev failed
        // {
        //         dbg(DBG_PRINT, "(GRADING2B)\n");
        //         return res;
        // }
        vnode_t *vnode = NULL;
        KASSERT(NULL != dir_vnode);
        res = lookup(dir_vnode, name, namelen, &vnode);
        KASSERT(NULL != dir_vnode->vn_ops->mknod);
        dbg(DBG_PRINT, "(GRADING2A 3.b)\n");
        res = dir_vnode->vn_ops->mknod(dir_vnode, name, namelen, mode, devid);
        vput(dir_vnode); // vput() the dir_vnode, decrement its refcount

        dbg(DBG_PRINT, "(GRADING2B)\n");

        return res;
}

/* Use dir_namev() to find the vnode of the dir we want to make the new
 * directory in.  Then use lookup() to make sure it doesn't already exist.
 * Finally call the dir's mkdir vn_ops. Return what it returns.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EEXIST
 *        path already exists.
 *      o ENOENT
 *        A directory component in path does not exist.
 *      o ENOTDIR
 *        A component used as a directory in path is not, in fact, a directory.
 *      o ENAMETOOLONG
 *        A component of path was too long.
 */
int do_mkdir(const char *path)
{

        // NOT_YET_IMPLEMENTED("VFS: do_mkdir");
        size_t namelen = 0;
        const char *name = NULL;
        vnode_t *dir_vnode = NULL;
        int res = dir_namev(path, &namelen, &name, NULL, &dir_vnode); // if success, refcount of dir_vnode will be incremented
        // if path is "/", return -EEXIST
        if (namelen == 0 && dir_vnode == vfs_root_vn)
        {
                vput(dir_vnode);
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return -EEXIST;
        }
        if (res != 0) // if res is not 0, dir_namev failed
        {
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return res;
        }
        vnode_t *vnode = NULL;
        KASSERT(NULL != dir_vnode);
        KASSERT(NULL != name);
        res = lookup(dir_vnode, name, namelen, &vnode);
        // if success, refcount of vnode will be incremented
        if (res == 0) // if res is 0, the directory already exists
        {
                vput(dir_vnode); // vput() the dir_vnode, decrement its refcount
                vput(vnode);     // vput() the vnode, decrement its refcount
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return -EEXIST;
        }
        KASSERT(NULL != dir_vnode->vn_ops->mkdir);
        dbg(DBG_PRINT, "(GRADING2A 3.c)\n");
        res = dir_vnode->vn_ops->mkdir(dir_vnode, name, namelen);
        vput(dir_vnode); // vput() the dir_vnode, decrement its refcount

        dbg(DBG_PRINT, "(GRADING2B)\n");
        return res;
}

/* Use dir_namev() to find the vnode of the directory containing the dir to be
 * removed. Then call the containing dir's rmdir v_op.  The rmdir v_op will
 * return an error if the dir to be removed does not exist or is not empty, so
 * you don't need to worry about that here. Return the value of the v_op,
 * or an error.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EINVAL
 *        path has "." as its final component.
 *      o ENOTEMPTY
 *        path has ".." as its final component.
 *      o ENOENT
 *        A directory component in path does not exist.
 *      o ENOTDIR
 *        A component used as a directory in path is not, in fact, a directory.
 *      o ENAMETOOLONG
 *        A component of path was too long.
 */
int do_rmdir(const char *path)
{

        // NOT_YET_IMPLEMENTED("VFS: do_rmdir");
        size_t namelen = 0;
        const char *name = NULL;
        vnode_t *dir_vnode = NULL;
        int res = dir_namev(path, &namelen, &name, NULL, &dir_vnode); // if success, refcount of dir_vnode will be incremented
        if (res != 0)                                                 // if res is not 0, dir_namev failed
        {
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return res;
        }
        // vnode_t *vnode = NULL;
        // res = lookup(dir_vnode, name, namelen, &vnode); // if success, refcount of vnode will be incremented

        if (strcmp(name, ".") == 0) // if name is ".", return -EINVAL
        {
                vput(dir_vnode); // vput() the dir_vnode, decrement its refcount
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return -EINVAL;
        }
        if (strcmp(name, "..") == 0) // if name is "..", return -ENOTEMPTY
        {
                vput(dir_vnode); // vput() the dir_vnode, decrement its refcount
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return -ENOTEMPTY;
        }
        KASSERT(NULL != dir_vnode->vn_ops->rmdir);
        dbg(DBG_PRINT, "(GRADING2A 3.d)\n");
        res = dir_vnode->vn_ops->rmdir(dir_vnode, name, namelen);
        vput(dir_vnode); // vput() the dir_vnode, decrement its refcount
        dbg(DBG_PRINT, "(GRADING2B)\n");
        return res;
}

/*
 * Similar to do_rmdir, but for files.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EPERM
 *        path refers to a directory.
 *      o ENOENT
 *        Any component in path does not exist, including the element at the
 *        very end.
 *      o ENOTDIR
 *        A component used as a directory in path is not, in fact, a directory.
 *      o ENAMETOOLONG
 *        A component of path was too long.
 */
int do_unlink(const char *path)
{
        // NOT_YET_IMPLEMENTED("VFS: do_unlink");
        size_t namelen = 0;
        const char *name = NULL;
        vnode_t *dir_vnode = NULL;
        int res = dir_namev(path, &namelen, &name, NULL, &dir_vnode); // if success, refcount of dir_vnode will be incremented
        if (res != 0)                                                 // if res is not 0, dir_namev failed
        {
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return res;
        }

        vnode_t *vnode = NULL;
        KASSERT(NULL != dir_vnode);
        res = lookup(dir_vnode, name, namelen, &vnode); // if success, refcount of vnode will be incremented
        if (res != 0)
        {
                vput(dir_vnode); // vput() the dir_vnode, decrement its refcount
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return res;
        }
        if (S_ISDIR(vnode->vn_mode))
        {
                vput(dir_vnode); // vput() the dir_vnode, decrement its refcount
                vput(vnode);     // vput() the vnode, decrement its refcount
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return -EPERM;
        }
        KASSERT(NULL != dir_vnode->vn_ops->unlink);
        dbg(DBG_PRINT, "(GRADING2A 3.e)\n");
        res = dir_vnode->vn_ops->unlink(dir_vnode, name, namelen);

        vput(dir_vnode); // vput() the dir_vnode, decrement its refcount
        vput(vnode);     // vput() the vnode, decrement its refcount
        dbg(DBG_PRINT, "(GRADING2B)\n");
        return res;
}

/* To link:
 *      o open_namev(from)
 *      o dir_namev(to)
 *      o call the destination dir's (to) link vn_ops.
 *      o return the result of link, or an error
 *
 * Remember to vput the vnodes returned from open_namev and dir_namev.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EEXIST
 *        to already exists.
 *      o ENOENT
 *        A directory component in from or to does not exist.
 *      o ENOTDIR
 *        A component used as a directory in from or to is not, in fact, a
 *        directory.
 *      o ENAMETOOLONG
 *        A component of from or to was too long.
 *      o EPERM
 *        from is a directory.
 */
int do_link(const char *from, const char *to)
{
        // NOT_YET_IMPLEMENTED("VFS: do_link");
        vnode_t *from_vnode = NULL;
        int res = open_namev(from, 0, &from_vnode, NULL); // if success, refcount of from_vnode will be incremented
        if (res != 0)
        {
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return res;
        }

        vnode_t *to_parent_vnode = NULL;
        size_t namelen = 0;
        const char *name = NULL;
        res = dir_namev(to, &namelen, &name, NULL, &to_parent_vnode); // if success, refcount of to_parent_vnode will be incremented
        if (res != 0)
        {
                vput(from_vnode); // vput() the from_vnode, decrement its refcount
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return res;
        }
        vnode_t *to_vnode = NULL;
        res = lookup(to_parent_vnode, name, namelen, &to_vnode); // if success, refcount of to_vnode will be incremented
        res = to_parent_vnode->vn_ops->link(from_vnode, to_parent_vnode, name, namelen);
        vput(from_vnode);      // vput() the from_vnode, decrement its refcount
        vput(to_parent_vnode); // vput() the to_parent_vnode, decrement its refcount
        dbg(DBG_PRINT, "(GRADING2B)\n");
        return res;
}

/*      o link newname to oldname
 *      o unlink oldname
 *      o return the value of unlink, or an error
 *
 * Note that this does not provide the same behavior as the
 * Linux system call (if unlink fails then two links to the
 * file could exist).
 */
int do_rename(const char *oldname, const char *newname)
{

        // NOT_YET_IMPLEMENTED("VFS: do_rename");
        int res = do_link(oldname, newname);
        if (res != 0)
        {
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return res;
        }
        res = do_unlink(oldname);
        dbg(DBG_PRINT, "(GRADING2B)\n");
        return res;
}

/* Make the named directory the current process's cwd (current working
 * directory).  Don't forget to down the refcount to the old cwd (vput()) and
 * up the refcount to the new cwd (open_namev() or vget()). Return 0 on
 * success.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o ENOENT
 *        path does not exist.
 *      o ENAMETOOLONG
 *        A component of path was too long.
 *      o ENOTDIR
 *        A component of path is not a directory.
 */
int do_chdir(const char *path)
{
        // NOT_YET_IMPLEMENTED("VFS: do_chdir");
        vnode_t *vnode = NULL;
        int res = open_namev(path, 0, &vnode, NULL); // if success, refcount of vnode will be incremented
        if (res != 0)                                // if res is not 0, open_namev failed
        {
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return res;
        }
        if (!S_ISDIR(vnode->vn_mode)) // if vnode is not a directory, return -ENOTDIR
        {
                vput(vnode); // vput() the vnode, decrement its refcount
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return -ENOTDIR;
        }
        vput(curproc->p_cwd); // vput() the old cwd, decrement its refcount
        curproc->p_cwd = vnode;
        dbg(DBG_PRINT, "(GRADING2B)\n");
        return 0;
}

/* Call the readdir vn_op on the given fd, filling in the given dirent_t*.
 * If the readdir vn_op is successful, it will return a positive value which
 * is the number of bytes copied to the dirent_t.  You need to increment the
 * file_t's f_pos by this amount.  As always, be aware of refcounts, check
 * the return value of the fget and the virtual function, and be sure the
 * virtual function exists (is not null) before calling it.
 *
 * Return either 0 or sizeof(dirent_t), or -errno.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        Invalid file descriptor fd.
 *      o ENOTDIR
 *        File descriptor does not refer to a directory.
 */
int do_getdent(int fd, struct dirent *dirp)
{
        // NOT_YET_IMPLEMENTED("VFS: do_getdent");
        if (fd < 0 || fd >= NFILES) // if fd is less than 0 or greater than or equal to NFILES, return -EBADF
        {
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return -EBADF;
        }
        if (NULL == curproc->p_files[fd]) // if curproc->p_files[fd] is NULL, return -EBADF
        {
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return -EBADF;
        }
        file_t *file = fget(fd); // if success, refcount of file will be incremented

        if (file->f_vnode->vn_ops->readdir == NULL) // if file->f_vnode->vn_ops->readdir is NULL, return -ENOTDIR
        {

                fput(file);
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return -ENOTDIR;
        }
        int bytes_read = file->f_vnode->vn_ops->readdir(file->f_vnode, file->f_pos, dirp);
        if (bytes_read == 0)
        {
                fput(file);
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return 0;
        }
        if (bytes_read > 0) // if bytes_read is greater than 0, increment the file_t's f_pos by this amount
        {

                file->f_pos += bytes_read;
                dbg(DBG_PRINT, "(GRADING2B)\n");
        }
        fput(file); // fput() the file, decrement its refcount
        // if readByteSize == 0, f_pos has reached RAMFS_MAX_DIRENT
        dbg(DBG_PRINT, "(GRADING2B)\n");

        return sizeof(*dirp);
}

/*
 * Modify f_pos according to offset and whence.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        fd is not an open file descriptor.
 *      o EINVAL
 *        whence is not one of SEEK_SET, SEEK_CUR, SEEK_END; or the resulting
 *        file offset would be negative.
 */
int do_lseek(int fd, int offset, int whence)
{

        // NOT_YET_IMPLEMENTED("VFS: do_lseek");
        if (fd < 0 || fd >= NFILES) // if fd is less than 0 or greater than or equal to NFILES, return -EBADF
        {
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return -EBADF;
        }
        file_t *file = fget(fd); // if success, refcount of file will be incremented
        if (file == NULL)        // if file is NULL, return -EBADF
        {
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return -EBADF;
        }
        int new_pos = 0;
        switch (whence)
        {
        case SEEK_SET:
                new_pos = offset;
                dbg(DBG_PRINT, "(GRADING2B)\n");
                break;
        case SEEK_CUR:
                new_pos = file->f_pos + offset;
                dbg(DBG_PRINT, "(GRADING2B)\n");
                break;
        case SEEK_END:
                new_pos = file->f_vnode->vn_len + offset;
                dbg(DBG_PRINT, "(GRADING2B)\n");
                break;
        default:
                fput(file); // fput() the file, decrement its refcount
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return -EINVAL;
        }
        if (new_pos < 0) // if new_pos is less than 0, return -EINVAL
        {
                fput(file); // fput() the file, decrement its refcount
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return -EINVAL;
        }
        file->f_pos = new_pos;
        fput(file); // fput() the file, decrement its refcount
        dbg(DBG_PRINT, "(GRADING2B)\n");
        return new_pos;
}

/*
 * Find the vnode associated with the path, and call the stat() vnode operation.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o ENOENT
 *        A component of path does not exist.
 *      o ENOTDIR
 *        A component of the path prefix of path is not a directory.
 *      o ENAMETOOLONG
 *        A component of path was too long.
 *      o EINVAL
 *        path is an empty string.
 */
int do_stat(const char *path, struct stat *buf)
{

        // NOT_YET_IMPLEMENTED("VFS: do_stat");
        if (strlen(path) == 0) // if path is an empty string, return -EINVAL
        {
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return -EINVAL;
        }
        vnode_t *vnode = NULL;
        int res = open_namev(path, 0, &vnode, NULL); // if success, refcount of vnode will be incremented
        if (res != 0)                                // if res is not 0, open_namev failed
        {
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return res;
        }
        KASSERT(NULL != vnode->vn_ops->stat);
        dbg(DBG_PRINT, "(GRADING2A 3.f)\n");
        res = vnode->vn_ops->stat(vnode, buf);
        vput(vnode); // vput() the vnode, decrement its refcount
        dbg(DBG_PRINT, "(GRADING2B)\n");
        return res;
}

#ifdef __MOUNTING__
/*
 * Implementing this function is not required and strongly discouraged unless
 * you are absolutely sure your Weenix is perfect.
 *
 * This is the syscall entry point into vfs for mounting. You will need to
 * create the fs_t struct and populate its fs_dev and fs_type fields before
 * calling vfs's mountfunc(). mountfunc() will use the fields you populated
 * in order to determine which underlying filesystem's mount function should
 * be run, then it will finish setting up the fs_t struct. At this point you
 * have a fully functioning file system, however it is not mounted on the
 * virtual file system, you will need to call vfs_mount to do this.
 *
 * There are lots of things which can go wrong here. Make sure you have good
 * error handling. Remember the fs_dev and fs_type buffers have limited size
 * so you should not write arbitrary length strings to them.
 */
int do_mount(const char *source, const char *target, const char *type)
{
        NOT_YET_IMPLEMENTED("MOUNTING: do_mount");
        return -EINVAL;
}

/*
 * Implementing this function is not required and strongly discouraged unless
 * you are absolutley sure your Weenix is perfect.
 *
 * This function delegates all of the real work to vfs_umount. You should not worry
 * about freeing the fs_t struct here, that is done in vfs_umount. All this function
 * does is figure out which file system to pass to vfs_umount and do good error
 * checking.
 */
int do_umount(const char *target)
{
        NOT_YET_IMPLEMENTED("MOUNTING: do_umount");
        return -EINVAL;
}
#endif

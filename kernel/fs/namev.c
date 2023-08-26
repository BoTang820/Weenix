#include "kernel.h"
#include "globals.h"
#include "types.h"
#include "errno.h"

#include "util/string.h"
#include "util/printf.h"
#include "util/debug.h"

#include "fs/dirent.h"
#include "fs/fcntl.h"
#include "fs/stat.h"
#include "fs/vfs.h"
#include "fs/vnode.h"

/* This takes a base 'dir', a 'name', its 'len', and a result vnode.
 * Most of the work should be done by the vnode's implementation
 * specific lookup() function.
 *
 * If dir has no lookup(), return -ENOTDIR.
 *
 * Note: returns with the vnode refcount on *result incremented.
 */
int lookup(vnode_t *dir, const char *name, size_t len, vnode_t **result)
{

        // NOT_YET_IMPLEMENTED("VFS: lookup");
        KASSERT(NULL != dir);
        KASSERT(NULL != name);
        KASSERT(NULL != result);
        dbg(DBG_PRINT, "(GRADING2A 2.a)\n");

        // if dir has no lookup(), return -ENOTDIR.
        // if (dir->vn_ops->lookup == NULL)
        // {
        //         dbg(DBG_PRINT, "(GRADING2B)\n");
        //         return -ENOTDIR;
        // }

        // if dir has lookup(), call it. The vnode_t returned by lookup() should be stored in result.
        int res = dir->vn_ops->lookup(dir, name, len, result);
        dbg(DBG_PRINT, "(GRADING2B)\n");
        return res;
}

/* When successful this function returns data in the following "out"-arguments:
 *  o res_vnode: the vnode of the parent directory of "name"
 *  o name: the `basename' (the element of the pathname)
 *  o name_len: the length of the basename
 *
 * For example: dir_namev("/s5fs/bin/ls", &name_len, &name, NULL,
 * &res_vnode) would put 2 in name_len, "ls" in name, and a pointer to the
 * vnode corresponding to "/s5fs/bin" in res_vnode.
 *
 * The "base" argument defines where we start resolving the path from:
 * A base value of NULL means to use the process's current working directory,
 * curproc->p_cwd.  If pathname[0] == '/', ignore base and start with
 * vfs_root_vn.  dir_namev() should call lookup() to take care of resolving each
 * piece of the pathname.
 *
 * Note: A successful call to this causes vnode refcount on *res_vnode to
 * be incremented.
 */

int dir_namev(const char *pathname, size_t *name_len, const char **name,
              vnode_t *base, vnode_t **res_vnode)
{
        // NOT_YET_IMPLEMENTED("VFS: dir_namev");
        vnode_t *cur_base = base;

        KASSERT(NULL != pathname);
        KASSERT(NULL != name_len);
        KASSERT(NULL != name);
        KASSERT(NULL != res_vnode);
        dbg(DBG_PRINT, "(GRADING2A 2.b)\n");
        const char *cur_path = pathname;

        // change cur_base according to different cases --check pathname first

        if (base == NULL)
        {
                cur_base = curproc->p_cwd;
                dbg(DBG_PRINT, "(GRADING2B)\n");
        }
        if ('/' == cur_path[0])
        {
                cur_base = vfs_root_vn;
                dbg(DBG_PRINT, "(GRADING2B)\n");
        }

        // find index limit of res_vnode parent directory
        // exclude last file of pathname
        int dir_index_limit = strlen(cur_path) - 1;
        while (dir_index_limit >= 0 && '/' != cur_path[dir_index_limit])
        {
                dir_index_limit--;
                dbg(DBG_PRINT, "(GRADING2B)\n");
        }
        // get the name of the file
        *name_len = strlen(cur_path) - (dir_index_limit + 1);
        // if pathname has last file
        if (*name_len > 0)
        {
                // check last file name length
                if (*name_len > NAME_LEN)
                {
                        dbg(DBG_PRINT, "(GRADING2B)\n");
                        return -ENAMETOOLONG;
                }
                // if pathname has at least 1 slash
                if (dir_index_limit >= 0)
                {
                        *name = &pathname[dir_index_limit] + 1;
                        dbg(DBG_PRINT, "(GRADING2B)\n");
                }
                // if pathname has no slash
                else
                {
                        *name = &pathname[0];
                        dbg(DBG_PRINT, "(GRADING2B)\n");
                }
        }

        int cur_pos = 0;
        int next_pos = 0;

        // skip slash
        while (next_pos < dir_index_limit && '/' == pathname[next_pos])
        {
                next_pos++;
                dbg(DBG_PRINT, "(GRADING2B)\n");
        }

        vnode_t *cur_node = NULL;
        vnode_t *prev_node = NULL;
        // try to search each file in pathname until index limit
        while (next_pos < dir_index_limit)
        {
                // first non-slash char
                cur_pos = next_pos;
                // put next_pos to next slash or end
                while (next_pos < dir_index_limit && '/' != pathname[next_pos])
                {

                        next_pos++;
                        dbg(DBG_PRINT, "(GRADING2B)\n");
                }
                int len = next_pos - cur_pos;

                // lookup next file from cur_base
                int res = lookup(cur_base, &pathname[cur_pos], len, &cur_node); // if lookup succeeds, cur_node refcount is incremented

                if (prev_node != NULL)
                {
                        // decrement the refcount of prev_node
                        vput(prev_node);
                        dbg(DBG_PRINT, "(GRADING2B)\n");
                }
                if (res < 0)
                {
                        dbg(DBG_PRINT, "(GRADING2B)\n");
                        return res;
                }

                // change cur_base to next file, prev_node to cur_node
                prev_node = cur_node;
                cur_base = cur_node;
                // skip slash
                while (next_pos < dir_index_limit && '/' == pathname[next_pos])
                {
                        next_pos++;
                        dbg(DBG_PRINT, "(GRADING2B)\n");
                }
                dbg(DBG_PRINT, "(GRADING2B)\n");
        }

        // if pathname >= 2 layer
        if (NULL != cur_node && cur_base->vn_ops->lookup == NULL)
        {
                vput(cur_node);
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return -ENOTDIR;
        }

        if (NULL == cur_node)
        {
                *res_vnode = cur_base;
                next_pos++;
                vref(*res_vnode);
                dbg(DBG_PRINT, "(GRADING2B)\n");
        }
        else
        {
                *res_vnode = cur_node;
                dbg(DBG_PRINT, "(GRADING2B)\n");
        }
        dbg(DBG_PRINT, "(GRADING2B)\n");
        return 0;
}

/* This returns in res_vnode the vnode requested by the other parameters.
 * It makes use of dir_namev and lookup to find the specified vnode (if it
 * exists).  flag is right out of the parameters to open(2); see
 * <weenix/fcntl.h>.  If the O_CREAT flag is specified and the file does
 * not exist, call create() in the parent directory vnode. However, if the
 * parent directory itself does not exist, this function should fail - in all
 * cases, no files or directories other than the one at the very end of the path
 * should be created.
 *
 * Note: Increments vnode refcount on *res_vnode.
 */

int open_namev(const char *pathname, int flag, vnode_t **res_vnode, vnode_t *base)
{
        // NOT_YET_IMPLEMENTED("VFS: open_namev");

        size_t name_len = 0;
        const char *name = NULL;
        vnode_t *dir_node = NULL;

        // resolve pathname
        int res = dir_namev(pathname, &name_len, &name, base, &dir_node);

        // if res < 0, parent directory does not exist
        if (res < 0)
        {
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return res;
        }
        // if name_len == 0, pathname has no last file
        else if (name_len == 0)
        {
                // set res_vnode to parent directory of res_vnode
                *res_vnode = dir_node;
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return 0;
        }
        // if pathname has last file
        res = lookup(dir_node, name, name_len, res_vnode);

        // check if pathname is a directory
        // if (pathname[strlen(pathname) - 1] == '/' && !S_ISDIR((*res_vnode)->vn_mode))
        // {
        //         vput((*res_vnode));
        //         vput(dir_node);
        //         dbg(DBG_PRINT, "(GRADING2B)\n");
        //         return -ENOTDIR;
        // }
        // if res < 0, lookup() fails
        if (res < 0)
        {
                // if O_CREAT flag is specified
                if (flag & O_CREAT)
                {

                        KASSERT(NULL != dir_node->vn_ops->create);
                        dbg(DBG_PRINT, "(GRADING2A 2.c)\n");
                        // create last file
                        res = dir_node->vn_ops->create(dir_node, name, name_len, res_vnode); // if create succeeds, res_vnode refcount is incremented
                        vput(dir_node);                                                      // decrement the refcount of dir_node
                        dbg(DBG_PRINT, "(GRADING2B)\n");
                        return res;
                }
                vput(dir_node);
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return res;
        }
        // lookup() has set res_vnode VNode and increased res_vnode vn_refcount
        vput(dir_node);
        dbg(DBG_PRINT, "(GRADING2B)\n");
        return 0;
}

#ifdef __GETCWD__
/* Finds the name of 'entry' in the directory 'dir'. The name is writen
 * to the given buffer. On success 0 is returned. If 'dir' does not
 * contain 'entry' then -ENOENT is returned. If the given buffer cannot
 * hold the result then it is filled with as many characters as possible
 * and a null terminator, -ERANGE is returned.
 *
 * Files can be uniquely identified within a file system by their
 * inode numbers. */
int lookup_name(vnode_t *dir, vnode_t *entry, char *buf, size_t size)
{
        NOT_YET_IMPLEMENTED("GETCWD: lookup_name");
        return -ENOENT;
}

/* Used to find the absolute path of the directory 'dir'. Since
 * directories cannot have more than one link there is always
 * a unique solution. The path is writen to the given buffer.
 * On success 0 is returned. On error this function returns a
 * negative error res. See the man page for getcwd(3) for
 * possible errors. Even if an error res is returned the buffer
 * will be filled with a valid string which has some partial
 * information about the wanted path. */
ssize_t
lookup_dirpath(vnode_t *dir, char *buf, size_t osize)
{
        NOT_YET_IMPLEMENTED("GETCWD: lookup_dirpath");

        return -ENOENT;
}
#endif /* __GETCWD__ */

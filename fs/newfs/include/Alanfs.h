#ifndef _ALANFS_H_
#define _ALANFS_H_

#define FUSE_USE_VERSION 26
#include "stdio.h"
#include "stdlib.h"
#include <unistd.h>
#include "fcntl.h"
#include "string.h"
#include "fuse.h"
#include <stddef.h>
#include "ddriver.h"
#include "errno.h"
#include "types.h"

#define ALANFS_MAGIC                  /* TODO: Define by yourself */
#define ALANFS_DEFAULT_PERM    0777   /* 全权限打开 */

/******************************************************************************
* SECTION: global region
*******************************************************************************/
struct Alanfs_super      Alanfs_super; 
struct custom_options Alanfs_options;
/******************************************************************************
* SECTION: macro debug
*******************************************************************************/
#define ALANFS_DBG(fmt, ...) do { printf("ALANFS_DBG: " fmt, ##__VA_ARGS__); } while(0) 
/******************************************************************************
* SECTION: Alanfs_utils.c
*******************************************************************************/
char* 			   Alanfs_get_fname(const char* path);
int 			   Alanfs_calc_lvl(const char * path);
int 			   Alanfs_driver_read(int offset, uint8_t *out_content, int size);
int 			   Alanfs_driver_write(int offset, uint8_t *in_content, int size);


int 			   Alanfs_mount(struct custom_options options);
int 			   Alanfs_umount();

int 			   Alanfs_alloc_dentry(struct Alanfs_inode * inode, struct Alanfs_dentry * dentry);
int 			   Alanfs_drop_dentry(struct Alanfs_inode * inode, struct Alanfs_dentry * dentry);
struct Alanfs_inode*  Alanfs_alloc_inode(struct Alanfs_dentry * dentry);
int 			   Alanfs_sync_inode(struct Alanfs_inode * inode);
int 			   Alanfs_drop_inode(struct Alanfs_inode * inode);
struct Alanfs_inode*  Alanfs_read_inode(struct Alanfs_dentry * dentry, int ino);
struct Alanfs_dentry* Alanfs_get_dentry(struct Alanfs_inode * inode, int dir);

struct Alanfs_dentry* Alanfs_lookup(const char * path, boolean * is_find, boolean* is_root);

/******************************************************************************
* SECTION: Alanfs.c
*******************************************************************************/
void* 			   Alanfs_init(struct fuse_conn_info *);
void  			   Alanfs_destroy(void *);
int   			   Alanfs_mkdir(const char *, mode_t);
int   			   Alanfs_getattr(const char *, struct stat *);
int   			   Alanfs_readdir(const char *, void *, fuse_fill_dir_t, off_t,
						                struct fuse_file_info *);
int   			   Alanfs_mknod(const char *, mode_t, dev_t);
int   			   Alanfs_write(const char *, const char *, size_t, off_t,
					                  struct fuse_file_info *);
int   			   Alanfs_read(const char *, char *, size_t, off_t,
					                 struct fuse_file_info *);
int   			   Alanfs_access(const char *, int);
int   			   Alanfs_unlink(const char *);
int   			   Alanfs_rmdir(const char *);
int   			   Alanfs_rename(const char *, const char *);
int   			   Alanfs_utimens(const char *, const struct timespec tv[2]);
int   			   Alanfs_truncate(const char *, off_t);
			
int   			   Alanfs_open(const char *, struct fuse_file_info *);
int   			   Alanfs_opendir(const char *, struct fuse_file_info *);

/******************************************************************************
* SECTION: Alanfs_debug.c
*******************************************************************************/
void 			   Alanfs_dump_map();

#endif  /* _Alanfs_H_ */
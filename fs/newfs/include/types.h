#ifndef _TYPES_H_
#define _TYPES_H_

#define MAX_NAME_LEN    128     

// struct custom_options {
// 	const char*        device;
// };

// struct Alanfs_super {
//     uint32_t magic;
//     int      fd;
//     /* TODO: Define yourself */
// };

// struct Alanfs_inode {
//     uint32_t ino;
//     /* TODO: Define yourself */
// };

// struct Alanfs_dentry {
//     char     name[MAX_NAME_LEN];
//     uint32_t ino;
//     /* TODO: Define yourself */
// };

/******************************************************************************
* SECTION: Type def
*******************************************************************************/
typedef int          boolean;
typedef uint16_t     flag16;

typedef enum Alanfs_file_type {
    ALANFS_REG_FILE,
    ALANFS_DIR
} ALANFS_FILE_TYPE;
/******************************************************************************
* SECTION: Macro
*******************************************************************************/
#define TRUE                    1
#define FALSE                   0
#define UINT32_BITS             32
#define UINT8_BITS              8

#define ALANFS_MAGIC_NUM           0x52415453  
#define ALANFS_SUPER_OFS           0
#define ALANFS_ROOT_INO            0



#define ALANFS_ERROR_NONE          0
#define ALANFS_ERROR_ACCESS        EACCES
#define ALANFS_ERROR_SEEK          ESPIPE     
#define ALANFS_ERROR_ISDIR         EISDIR
#define ALANFS_ERROR_NOSPACE       ENOSPC
#define ALANFS_ERROR_EXISTS        EEXIST
#define ALANFS_ERROR_NOTFOUND      ENOENT
#define ALANFS_ERROR_UNSUPPORTED   ENXIO
#define ALANFS_ERROR_IO            EIO     /* Error Input/Output */
#define ALANFS_ERROR_INVAL         EINVAL  /* Invalid Args */

#define ALANFS_MAX_FILE_NAME       128
#define ALANFS_INODE_PER_FILE      1
#define ALANFS_DATA_PER_FILE       16
#define ALANFS_DEFAULT_PERM        0777

#define ALANFS_IOC_MAGIC           'S'
#define ALANFS_IOC_SEEK            _IO(ALANFS_IOC_MAGIC, 0)

#define ALANFS_FLAG_BUF_DIRTY      0x1
#define ALANFS_FLAG_BUF_OCCUPY     0x2
/******************************************************************************
* SECTION: Macro Function
*******************************************************************************/
#define ALANFS_IO_SZ()                     (Alanfs_super.sz_io)
#define ALANFS_DISK_SZ()                   (Alanfs_super.sz_disk)
#define ALANFS_DRIVER()                    (Alanfs_super.driver_fd)

#define ALANFS_ROUND_DOWN(value, round)    (value % round == 0 ? value : (value / round) * round)
#define ALANFS_ROUND_UP(value, round)      (value % round == 0 ? value : (value / round + 1) * round)

#define ALANFS_BLKS_SZ(blks)               (blks * ALANFS_IO_SZ())
#define ALANFS_ASSIGN_FNAME(pAlanfs_dentry, _fname)\ 
                                        memcpy(pAlanfs_dentry->fname, _fname, strlen(_fname))
#define ALANFS_INO_OFS(ino)                (Alanfs_super.data_offset + ino * ALANFS_BLKS_SZ((\
                                        ALANFS_INODE_PER_FILE + ALANFS_DATA_PER_FILE)))
#define ALANFS_DATA_OFS(ino)               (ALANFS_INO_OFS(ino) + ALANFS_BLKS_SZ(ALANFS_INODE_PER_FILE))

#define ALANFS_IS_DIR(pinode)              (pinode->dentry->ftype == ALANFS_DIR)
#define ALANFS_IS_REG(pinode)              (pinode->dentry->ftype == ALANFS_REG_FILE)
/******************************************************************************
* SECTION: FS Specific Structure - In memory structure
*******************************************************************************/
struct Alanfs_dentry;
struct Alanfs_inode;
struct Alanfs_super;

struct custom_options {
	const char*        device;
	boolean            show_help;
};

struct Alanfs_inode
{
    int                ino;                           /* 在inode位图中的下标 */
    int                size;                          /* 文件已占用空间 */
    int                dir_cnt;
    struct Alanfs_dentry* dentry;                        /* 指向该inode的dentry */
    struct Alanfs_dentry* dentrys;                       /* 所有目录项 */
    uint8_t*           data;           
};  

struct Alanfs_dentry
{
    char               fname[ALANFS_MAX_FILE_NAME];
    struct Alanfs_dentry* parent;                        /* 父亲Inode的dentry */
    struct Alanfs_dentry* brother;                       /* 兄弟 */
    int                ino;
    struct Alanfs_inode*  inode;                         /* 指向inode */
    ALANFS_FILE_TYPE      ftype;
};

struct Alanfs_super
{
    int                driver_fd;
    
    int                sz_io;
    int                sz_disk;
    int                sz_usage;
    
    int                max_ino;
    uint8_t*           map_inode;
    int                map_inode_blks;
    int                map_inode_offset;
    
    int                data_offset;

    boolean            is_mounted;

    struct Alanfs_dentry* root_dentry;
};

static inline struct Alanfs_dentry* new_dentry(char * fname, ALANFS_FILE_TYPE ftype) {
    struct Alanfs_dentry * dentry = (struct Alanfs_dentry *)malloc(sizeof(struct Alanfs_dentry));
    memset(dentry, 0, sizeof(struct Alanfs_dentry));
    ALANFS_ASSIGN_FNAME(dentry, fname);
    dentry->ftype   = ftype;
    dentry->ino     = -1;
    dentry->inode   = NULL;
    dentry->parent  = NULL;
    dentry->brother = NULL;                                            
}
/******************************************************************************
* SECTION: FS Specific Structure - Disk structure
*******************************************************************************/
struct Alanfs_super_d
{
    uint32_t           magic_num;
    int                sz_usage;
    
    int                max_ino;
    int                map_inode_blks;
    int                map_inode_offset;
    int                data_offset;
};

struct Alanfs_inode_d
{
    int                ino;                           /* 在inode位图中的下标 */
    int                size;                          /* 文件已占用空间 */
    int                dir_cnt;
    ALANFS_FILE_TYPE      ftype;   
};  

struct Alanfs_dentry_d
{
    char               fname[ALANFS_MAX_FILE_NAME];
    ALANFS_FILE_TYPE      ftype;
    int                ino;                           /* 指向的ino号 */
};  

#endif /* _TYPES_H_ */
#include "../include/Alanfs.h"
/**
 * @brief 获取文件名
 * 
 * @param path 
 * @return char* 
 */
char* Alanfs_get_fname(const char* path) {
    char ch = '/';
    char *q = strrchr(path, ch) + 1;
    return q;
}
/**
 * @brief 计算路径的层级
 * exm: /av/c/d/f
 * -> lvl = 4
 * @param path 
 * @return int 
 */
int Alanfs_calc_lvl(const char * path) {
    // char* path_cpy = (char *)malloc(strlen(path));
    // strcpy(path_cpy, path);
    char* str = path;
    int   lvl = 0;
    if (strcmp(path, "/") == 0) {
        return lvl;
    }
    while (*str != NULL) {
        if (*str == '/') {
            lvl++;
        }
        str++;
    }
    return lvl;
}
/**
 * @brief 驱动读
 * 
 * @param offset 
 * @param out_content 
 * @param size 
 * @return int 
 */
int Alanfs_driver_read(int offset, uint8_t *out_content, int size) {
    int      offset_aligned = ALANFS_ROUND_DOWN(offset, ALANFS_IO_SZ());
    int      bias           = offset - offset_aligned;
    int      size_aligned   = ALANFS_ROUND_UP((size + bias), ALANFS_IO_SZ());
    uint8_t* temp_content   = (uint8_t*)malloc(size_aligned);
    uint8_t* cur            = temp_content;
    // lseek(ALANFS_DRIVER(), offset_aligned, SEEK_SET);
    ddriver_seek(ALANFS_DRIVER(), offset_aligned, SEEK_SET);
    while (size_aligned != 0)
    {
        // read(ALANFS_DRIVER(), cur, ALANFS_IO_SZ());
        ddriver_read(ALANFS_DRIVER(), cur, ALANFS_IO_SZ());
        cur          += ALANFS_IO_SZ();
        size_aligned -= ALANFS_IO_SZ();   
    }
    memcpy(out_content, temp_content + bias, size);
    free(temp_content);
    return ALANFS_ERROR_NONE;
}
/**
 * @brief 驱动写
 * 
 * @param offset 
 * @param in_content 
 * @param size 
 * @return int 
 */
int Alanfs_driver_write(int offset, uint8_t *in_content, int size) {
    int      offset_aligned = ALANFS_ROUND_DOWN(offset, ALANFS_IO_SZ());
    int      bias           = offset - offset_aligned;
    int      size_aligned   = ALANFS_ROUND_UP((size + bias), ALANFS_IO_SZ());
    uint8_t* temp_content   = (uint8_t*)malloc(size_aligned);
    uint8_t* cur            = temp_content;
    Alanfs_driver_read(offset_aligned, temp_content, size_aligned);
    memcpy(temp_content + bias, in_content, size);
    
    // lseek(ALANFS_DRIVER(), offset_aligned, SEEK_SET);
    ddriver_seek(ALANFS_DRIVER(), offset_aligned, SEEK_SET);
    while (size_aligned != 0)
    {
        // write(ALANFS_DRIVER(), cur, ALANFS_IO_SZ());
        ddriver_write(ALANFS_DRIVER(), cur, ALANFS_IO_SZ());
        cur          += ALANFS_IO_SZ();
        size_aligned -= ALANFS_IO_SZ();   
    }

    free(temp_content);
    return ALANFS_ERROR_NONE;
}
/**
 * @brief 为一个inode分配dentry，采用头插法
 * 
 * @param inode 
 * @param dentry 
 * @return int 
 */
int Alanfs_alloc_dentry(struct Alanfs_inode* inode, struct Alanfs_dentry* dentry) {
    if (inode->dentrys == NULL) {
        inode->dentrys = dentry;
    }
    else {
        dentry->brother = inode->dentrys;
        inode->dentrys = dentry;
    }
    inode->dir_cnt++;
    return inode->dir_cnt;
}
/**
 * @brief 将dentry从inode的dentrys中取出
 * 
 * @param inode 
 * @param dentry 
 * @return int 
 */
int Alanfs_drop_dentry(struct Alanfs_inode * inode, struct Alanfs_dentry * dentry) {
    boolean is_find = FALSE;
    struct Alanfs_dentry* dentry_cursor;
    dentry_cursor = inode->dentrys;
    
    if (dentry_cursor == dentry) {
        inode->dentrys = dentry->brother;
        is_find = TRUE;
    }
    else {
        while (dentry_cursor)
        {
            if (dentry_cursor->brother == dentry) {
                dentry_cursor->brother = dentry->brother;
                is_find = TRUE;
                break;
            }
            dentry_cursor = dentry_cursor->brother;
        }
    }
    if (!is_find) {
        return -ALANFS_ERROR_NOTFOUND;
    }
    inode->dir_cnt--;
    return inode->dir_cnt;
}
/**
 * @brief 分配一个inode，占用位图
 * 
 * @param dentry 该dentry指向分配的inode
 * @return Alanfs_inode
 */
struct Alanfs_inode* Alanfs_alloc_inode(struct Alanfs_dentry * dentry) {
    struct Alanfs_inode* inode;
    int byte_cursor = 0; 
    int bit_cursor  = 0; 
    int ino_cursor  = 0;
    boolean is_find_free_entry = FALSE;

    for (byte_cursor = 0; byte_cursor < ALANFS_BLKS_SZ(Alanfs_super.map_inode_blks); 
         byte_cursor++)
    {
        for (bit_cursor = 0; bit_cursor < UINT8_BITS; bit_cursor++) {
            if((Alanfs_super.map_inode[byte_cursor] & (0x1 << bit_cursor)) == 0) {    
                                                      /* 当前ino_cursor位置空闲 */
                Alanfs_super.map_inode[byte_cursor] |= (0x1 << bit_cursor);
                is_find_free_entry = TRUE;           
                break;
            }
            ino_cursor++;
        }
        if (is_find_free_entry) {
            break;
        }
    }

    if (!is_find_free_entry || ino_cursor == Alanfs_super.max_ino)
        return -ALANFS_ERROR_NOSPACE;

    inode = (struct Alanfs_inode*)malloc(sizeof(struct Alanfs_inode));
    inode->ino  = ino_cursor; 
    inode->size = 0;
                                                      /* dentry指向inode */
    dentry->inode = inode;
    dentry->ino   = inode->ino;
                                                      /* inode指回dentry */
    inode->dentry = dentry;
    
    inode->dir_cnt = 0;
    inode->dentrys = NULL;
    
    if (ALANFS_IS_REG(inode)) {
        inode->data = (uint8_t *)malloc(ALANFS_BLKS_SZ(ALANFS_DATA_PER_FILE));
    }

    return inode;
}
/**
 * @brief 将内存inode及其下方结构全部刷回磁盘
 * 
 * @param inode 
 * @return int 
 */
int Alanfs_sync_inode(struct Alanfs_inode * inode) {
    struct Alanfs_inode_d  inode_d;
    struct Alanfs_dentry*  dentry_cursor;
    struct Alanfs_dentry_d dentry_d;
    int ino             = inode->ino;
    inode_d.ino         = ino;
    inode_d.size        = inode->size;
    inode_d.ftype       = inode->dentry->ftype;
    inode_d.dir_cnt     = inode->dir_cnt;
    int offset;
    
    if (Alanfs_driver_write(ALANFS_INO_OFS(ino), (uint8_t *)&inode_d, 
                     sizeof(struct Alanfs_inode_d)) != ALANFS_ERROR_NONE) {
        ALANFS_DBG("[%s] io error\n", __func__);
        return -ALANFS_ERROR_IO;
    }
                                                      /* Cycle 1: 写 INODE */
                                                      /* Cycle 2: 写 数据 */
    if (ALANFS_IS_DIR(inode)) {                          
        dentry_cursor = inode->dentrys;
        offset        = ALANFS_DATA_OFS(ino);
        while (dentry_cursor != NULL)
        {
            memcpy(dentry_d.fname, dentry_cursor->fname, ALANFS_MAX_FILE_NAME);
            dentry_d.ftype = dentry_cursor->ftype;
            dentry_d.ino = dentry_cursor->ino;
            if (Alanfs_driver_write(offset, (uint8_t *)&dentry_d, 
                                 sizeof(struct Alanfs_dentry_d)) != ALANFS_ERROR_NONE) {
                ALANFS_DBG("[%s] io error\n", __func__);
                return -ALANFS_ERROR_IO;                     
            }
            
            if (dentry_cursor->inode != NULL) {
                Alanfs_sync_inode(dentry_cursor->inode);
            }

            dentry_cursor = dentry_cursor->brother;
            offset += sizeof(struct Alanfs_dentry_d);
        }
    }
    else if (ALANFS_IS_REG(inode)) {
        if (Alanfs_driver_write(ALANFS_DATA_OFS(ino), inode->data, 
                             ALANFS_BLKS_SZ(ALANFS_DATA_PER_FILE)) != ALANFS_ERROR_NONE) {
            ALANFS_DBG("[%s] io error\n", __func__);
            return -ALANFS_ERROR_IO;
        }
    }
    return ALANFS_ERROR_NONE;
}
/**
 * @brief 删除内存中的一个inode， 暂时不释放
 * Case 1: Reg File
 * 
 *                  Inode
 *                /      \
 *            Dentry -> Dentry (Reg Dentry)
 *                       |
 *                      Inode  (Reg File)
 * 
 *  1) Step 1. Erase Bitmap     
 *  2) Step 2. Free Inode                      (Function of Alanfs_drop_inode)
 * ------------------------------------------------------------------------
 *  3) *Setp 3. Free Dentry belonging to Inode (Outsider)
 * ========================================================================
 * Case 2: Dir
 *                  Inode
 *                /      \
 *            Dentry -> Dentry (Dir Dentry)
 *                       |
 *                      Inode  (Dir)
 *                    /     \
 *                Dentry -> Dentry
 * 
 *   Recursive
 * @param inode 
 * @return int 
 */
int Alanfs_drop_inode(struct Alanfs_inode * inode) {
    struct Alanfs_dentry*  dentry_cursor;
    struct Alanfs_dentry*  dentry_to_free;
    struct Alanfs_inode*   inode_cursor;

    int byte_cursor = 0; 
    int bit_cursor  = 0; 
    int ino_cursor  = 0;
    boolean is_find = FALSE;

    if (inode == Alanfs_super.root_dentry->inode) {
        return ALANFS_ERROR_INVAL;
    }

    if (ALANFS_IS_DIR(inode)) {
        dentry_cursor = inode->dentrys;
                                                      /* 递归向下drop */
        while (dentry_cursor)
        {   
            inode_cursor = dentry_cursor->inode;
            Alanfs_drop_inode(inode_cursor);
            Alanfs_drop_dentry(inode, dentry_cursor);
            dentry_to_free = dentry_cursor;
            dentry_cursor = dentry_cursor->brother;
            free(dentry_to_free);
        }
    }
    else if (ALANFS_IS_REG(inode)) {
        for (byte_cursor = 0; byte_cursor < ALANFS_BLKS_SZ(Alanfs_super.map_inode_blks); 
            byte_cursor++)                            /* 调整inodemap */
        {
            for (bit_cursor = 0; bit_cursor < UINT8_BITS; bit_cursor++) {
                if (ino_cursor == inode->ino) {
                     Alanfs_super.map_inode[byte_cursor] &= (uint8_t)(~(0x1 << bit_cursor));
                     is_find = TRUE;
                     break;
                }
                ino_cursor++;
            }
            if (is_find == TRUE) {
                break;
            }
        }
        if (inode->data)
            free(inode->data);
        free(inode);
    }
    return ALANFS_ERROR_NONE;
}
/**
 * @brief 
 * 
 * @param dentry dentry指向ino，读取该inode
 * @param ino inode唯一编号
 * @return struct Alanfs_inode* 
 */
struct Alanfs_inode* Alanfs_read_inode(struct Alanfs_dentry * dentry, int ino) {
    struct Alanfs_inode* inode = (struct Alanfs_inode*)malloc(sizeof(struct Alanfs_inode));
    struct Alanfs_inode_d inode_d;
    struct Alanfs_dentry* sub_dentry;
    struct Alanfs_dentry_d dentry_d;
    int    dir_cnt = 0, i;
    if (Alanfs_driver_read(ALANFS_INO_OFS(ino), (uint8_t *)&inode_d, 
                        sizeof(struct Alanfs_inode_d)) != ALANFS_ERROR_NONE) {
        ALANFS_DBG("[%s] io error\n", __func__);
        return NULL;                    
    }
    inode->dir_cnt = 0;
    inode->ino = inode_d.ino;
    inode->size = inode_d.size;
    inode->dentry = dentry;
    inode->dentrys = NULL;
    if (ALANFS_IS_DIR(inode)) {
        dir_cnt = inode_d.dir_cnt;
        for (i = 0; i < dir_cnt; i++)
        {
            if (Alanfs_driver_read(ALANFS_DATA_OFS(ino) + i * sizeof(struct Alanfs_dentry_d), 
                                (uint8_t *)&dentry_d, 
                                sizeof(struct Alanfs_dentry_d)) != ALANFS_ERROR_NONE) {
                ALANFS_DBG("[%s] io error\n", __func__);
                return NULL;                    
            }
            sub_dentry = new_dentry(dentry_d.fname, dentry_d.ftype);
            sub_dentry->parent = inode->dentry;
            sub_dentry->ino    = dentry_d.ino; 
            Alanfs_alloc_dentry(inode, sub_dentry);
        }
    }
    else if (ALANFS_IS_REG(inode)) {
        inode->data = (uint8_t *)malloc(ALANFS_BLKS_SZ(ALANFS_DATA_PER_FILE));
        if (Alanfs_driver_read(ALANFS_DATA_OFS(ino), (uint8_t *)inode->data, 
                            ALANFS_BLKS_SZ(ALANFS_DATA_PER_FILE)) != ALANFS_ERROR_NONE) {
            ALANFS_DBG("[%s] io error\n", __func__);
            return NULL;                    
        }
    }
    return inode;
}
/**
 * @brief 
 * 
 * @param inode 
 * @param dir [0...]
 * @return struct Alanfs_dentry* 
 */
struct Alanfs_dentry* Alanfs_get_dentry(struct Alanfs_inode * inode, int dir) {
    struct Alanfs_dentry* dentry_cursor = inode->dentrys;
    int    cnt = 0;
    while (dentry_cursor)
    {
        if (dir == cnt) {
            return dentry_cursor;
        }
        cnt++;
        dentry_cursor = dentry_cursor->brother;
    }
    return NULL;
}
/**
 * @brief 
 * path: /qwe/ad  total_lvl = 2,
 *      1) find /'s inode       lvl = 1
 *      2) find qwe's dentry 
 *      3) find qwe's inode     lvl = 2
 *      4) find ad's dentry
 *
 * path: /qwe     total_lvl = 1,
 *      1) find /'s inode       lvl = 1
 *      2) find qwe's dentry
 * 
 * @param path 
 * @return struct Alanfs_inode* 
 */
struct Alanfs_dentry* Alanfs_lookup(const char * path, boolean* is_find, boolean* is_root) {
    struct Alanfs_dentry* dentry_cursor = Alanfs_super.root_dentry;
    struct Alanfs_dentry* dentry_ret = NULL;
    struct Alanfs_inode*  inode; 
    int   total_lvl = Alanfs_calc_lvl(path);
    int   lvl = 0;
    boolean is_hit;
    char* fname = NULL;
    char* path_cpy = (char*)malloc(sizeof(path));
    *is_root = FALSE;
    strcpy(path_cpy, path);

    if (total_lvl == 0) {                           /* 根目录 */
        *is_find = TRUE;
        *is_root = TRUE;
        dentry_ret = Alanfs_super.root_dentry;
    }
    fname = strtok(path_cpy, "/");       
    while (fname)
    {   
        lvl++;
        if (dentry_cursor->inode == NULL) {           /* Cache机制 */
            Alanfs_read_inode(dentry_cursor, dentry_cursor->ino);
        }

        inode = dentry_cursor->inode;

        if (ALANFS_IS_REG(inode) && lvl < total_lvl) {
            ALANFS_DBG("[%s] not a dir\n", __func__);
            dentry_ret = inode->dentry;
            break;
        }
        if (ALANFS_IS_DIR(inode)) {
            dentry_cursor = inode->dentrys;
            is_hit        = FALSE;

            while (dentry_cursor)
            {
                if (memcmp(dentry_cursor->fname, fname, strlen(fname)) == 0) {
                    is_hit = TRUE;
                    break;
                }
                dentry_cursor = dentry_cursor->brother;
            }
            
            if (!is_hit) {
                *is_find = FALSE;
                ALANFS_DBG("[%s] not found %s\n", __func__, fname);
                dentry_ret = inode->dentry;
                break;
            }

            if (is_hit && lvl == total_lvl) {
                *is_find = TRUE;
                dentry_ret = dentry_cursor;
                break;
            }
        }
        fname = strtok(NULL, "/"); 
    }

    if (dentry_ret->inode == NULL) {
        dentry_ret->inode = Alanfs_read_inode(dentry_ret, dentry_ret->ino);
    }
    
    return dentry_ret;
}
/**
 * @brief 挂载Alanfs, Layout 如下
 * 
 * Layout
 * | Super | Inode Map | Data |
 * 
 * IO_SZ = BLK_SZ
 * 
 * 每个Inode占用一个Blk
 * @param options 
 * @return int 
 */
int Alanfs_mount(struct custom_options options){
    int                 ret = ALANFS_ERROR_NONE;
    int                 driver_fd;
    struct Alanfs_super_d  Alanfs_super_d; 
    struct Alanfs_dentry*  root_dentry;
    struct Alanfs_inode*   root_inode;

    int                 inode_num;
    int                 map_inode_blks;
    
    int                 super_blks;
    boolean             is_init = FALSE;

    Alanfs_super.is_mounted = FALSE;

    // driver_fd = open(options.device, O_RDWR);
    driver_fd = ddriver_open(options.device);

    if (driver_fd < 0) {
        return driver_fd;
    }

    Alanfs_super.driver_fd = driver_fd;
    ddriver_ioctl(ALANFS_DRIVER(), IOC_REQ_DEVICE_SIZE,  &Alanfs_super.sz_disk);
    ddriver_ioctl(ALANFS_DRIVER(), IOC_REQ_DEVICE_IO_SZ, &Alanfs_super.sz_io);
    
    root_dentry = new_dentry("/", ALANFS_DIR);

    if (Alanfs_driver_read(ALANFS_SUPER_OFS, (uint8_t *)(&Alanfs_super_d), 
                        sizeof(struct Alanfs_super_d)) != ALANFS_ERROR_NONE) {
        return -ALANFS_ERROR_IO;
    }   
                                                      /* 读取super */
    if (Alanfs_super_d.magic_num != ALANFS_MAGIC_NUM) {     /* 幻数无 */
                                                      /* 估算各部分大小 */
        super_blks = ALANFS_ROUND_UP(sizeof(struct Alanfs_super_d), ALANFS_IO_SZ()) / ALANFS_IO_SZ();

        inode_num  =  ALANFS_DISK_SZ() / ((ALANFS_DATA_PER_FILE + ALANFS_INODE_PER_FILE) * ALANFS_IO_SZ());

        map_inode_blks = ALANFS_ROUND_UP(ALANFS_ROUND_UP(inode_num, UINT32_BITS), ALANFS_IO_SZ()) 
                         / ALANFS_IO_SZ();
        
                                                      /* 布局layout */
        Alanfs_super.max_ino = (inode_num - super_blks - map_inode_blks); 
        Alanfs_super_d.map_inode_offset = ALANFS_SUPER_OFS + ALANFS_BLKS_SZ(super_blks);
        Alanfs_super_d.data_offset = Alanfs_super_d.map_inode_offset + ALANFS_BLKS_SZ(map_inode_blks);
        Alanfs_super_d.map_inode_blks  = map_inode_blks;
        Alanfs_super_d.sz_usage    = 0;

        is_init = TRUE;
    }
    Alanfs_super.sz_usage   = Alanfs_super_d.sz_usage;      /* 建立 in-memory 结构 */
    
    Alanfs_super.map_inode = (uint8_t *)malloc(ALANFS_BLKS_SZ(Alanfs_super_d.map_inode_blks));
    Alanfs_super.map_inode_blks = Alanfs_super_d.map_inode_blks;
    Alanfs_super.map_inode_offset = Alanfs_super_d.map_inode_offset;
    Alanfs_super.data_offset = Alanfs_super_d.data_offset;

    if (Alanfs_driver_read(Alanfs_super_d.map_inode_offset, (uint8_t *)(Alanfs_super.map_inode), 
                        ALANFS_BLKS_SZ(Alanfs_super_d.map_inode_blks)) != ALANFS_ERROR_NONE) {
        return -ALANFS_ERROR_IO;
    }

    if (is_init) {                                    /* 分配根节点 */
        root_inode = Alanfs_alloc_inode(root_dentry);
        Alanfs_sync_inode(root_inode);
    }
    
    root_inode            = Alanfs_read_inode(root_dentry, ALANFS_ROOT_INO);
    root_dentry->inode    = root_inode;
    Alanfs_super.root_dentry = root_dentry;
    Alanfs_super.is_mounted  = TRUE;

    Alanfs_dump_map();
    return ret;
}
/**
 * @brief 
 * 
 * @return int 
 */
int Alanfs_umount() {
    struct Alanfs_super_d  Alanfs_super_d; 

    if (!Alanfs_super.is_mounted) {
        return ALANFS_ERROR_NONE;
    }

    Alanfs_sync_inode(Alanfs_super.root_dentry->inode);     /* 从根节点向下刷写节点 */
                                                    
    Alanfs_super_d.magic_num           = ALANFS_MAGIC_NUM;
    Alanfs_super_d.map_inode_blks      = Alanfs_super.map_inode_blks;
    Alanfs_super_d.map_inode_offset    = Alanfs_super.map_inode_offset;
    Alanfs_super_d.data_offset         = Alanfs_super.data_offset;
    Alanfs_super_d.sz_usage            = Alanfs_super.sz_usage;

    if (Alanfs_driver_write(ALANFS_SUPER_OFS, (uint8_t *)&Alanfs_super_d, 
                     sizeof(struct Alanfs_super_d)) != ALANFS_ERROR_NONE) {
        return -ALANFS_ERROR_IO;
    }

    if (Alanfs_driver_write(Alanfs_super_d.map_inode_offset, (uint8_t *)(Alanfs_super.map_inode), 
                         ALANFS_BLKS_SZ(Alanfs_super_d.map_inode_blks)) != ALANFS_ERROR_NONE) {
        return -ALANFS_ERROR_IO;
    }

    free(Alanfs_super.map_inode);
    ddriver_close(ALANFS_DRIVER());

    return ALANFS_ERROR_NONE;
}

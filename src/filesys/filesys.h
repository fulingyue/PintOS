#ifndef FILESYS_FILESYS_H
#define FILESYS_FILESYS_H

#include <stdbool.h>
#include "filesys/off_t.h"
#include "filesys/directory.h"

/* Sectors of system file inodes. */
#define FREE_MAP_SECTOR 0       /* Free map file inode sector. */
#define ROOT_DIR_SECTOR 1       /* Root directory file inode sector. */

/* Block device that contains the file system. */
struct block *fs_device;

void filesys_init (bool format);
void filesys_done (void);
bool filesys_create (const char *name, off_t initial_size);
struct file *filesys_open (const char *name);
bool filesys_remove (const char *name);

/* Implementation by ymt Started */
bool check_filedir_name(const char *name);
bool is_rootpath(const char *name);
bool path_paser(const char *target_path, struct dir **prev_dir, char **pure_name, bool *is_dir);
/* Implementation by ymt Ended */

#endif /* filesys/filesys.h */
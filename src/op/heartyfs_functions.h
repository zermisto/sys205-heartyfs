/**
 * @file heartyfs_functions.h
 * @author your name (you@domain.com)
 * @brief Header file for functions handling directory and file operations in the HeartyFS file system.
 * @version 0.1
 * @date 2024-10-03
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#ifndef HEARTYFS_FUNCTIONS_H
#define HEARTYFS_FUNCTIONS_H

#include "../heartyfs.h"

int find_free_block(unsigned char *bitmap);
void set_block_used(unsigned char *bitmap, int block_num);
void set_block_free(unsigned char *bitmap, int block_num);
// int find_file(void *buffer, const char *path, struct heartyfs_inode **inode);
int find_inode_by_path(void *buffer, const char *path, struct heartyfs_inode **inode);

 int find_parent_directory_and_file_index(void *buffer, const char *path, struct heartyfs_directory **parent_dir, int *file_index);

#endif // HEARTYFS_FUNCTIONS_H

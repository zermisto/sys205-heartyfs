/**
 * @file heartyfs_functions.c
 * @author Panupong Dangkajitpetch (King)
 * @brief This file contains functions for handling directory and file operations in the HeartyFS file system.
 * @version 0.1
 * @date 2024-10-03
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#include "../heartyfs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <libgen.h>
#include <stdlib.h>
#include <sys/mman.h>

/**
 * @brief Find the first free block in the bitmap. 
 * 
 * @param bitmap - The bitmap containing the block allocation information
 * @return int - The block number of the first free block, -1 if no free block is found
 */
int find_free_block(unsigned char *bitmap) {
    for (int i = 2; i < NUM_BLOCK; i++) {
        if (bitmap[i/8] & (1 << (7 - i%8))) { 
            return i;
        }   
    }
    return -1;
}

/**
 * @brief Set the block used object
 * 
 * @param bitmap - The bitmap containing the block allocation information
 * @param block_num - The block number to set as used
 */
void set_block_used(unsigned char *bitmap, int block_num) {
    bitmap[block_num/8] &= ~(1 << (7 - block_num%8));
}

/**
 * @brief Set the block free object
 * 
 * @param bitmap - The bitmap containing the block allocation information
 * @param block_num - The block number to set as free
 */
void set_block_free(unsigned char *bitmap, int block_num) {
    //
    bitmap[block_num/8] |= (1 << (7 - block_num%8));
}

/**
 * @brief Find an inode by its path
 * 
 * @param buffer - The buffer containing the disk image
 * @param path - The path of the inode
 * @param inode - The inode object to be returned
 * @return int - The block number of the inode, -1 if not found
 */
int find_inode_by_path(void *buffer, const char *path, struct heartyfs_inode **inode) {
    char *path_copy = strdup(path);
    char *parent_path = dirname(strdup(path_copy));
    char *file_name = basename(path_copy);

    struct heartyfs_directory *current = buffer;
    int current_block_id = 0;

    char *token = strtok(parent_path, "/");
    while (token != NULL) {
        int found = 0;
        for (int i = 0; i < current->size; i++) {
            if (strcmp(current->entries[i].file_name, token) == 0) {
                current_block_id = current->entries[i].block_id;
                current = (struct heartyfs_directory *)((char *)buffer + current_block_id * BLOCK_SIZE);
                found = 1;
                break;
            }
        }
        if (!found) {
            free(path_copy);
            free(parent_path);
            return -1;
        }
        token = strtok(NULL, "/");
    }

    for (int i = 0; i < current->size; i++) {
        if (strcmp(current->entries[i].file_name, file_name) == 0) {
            *inode = (struct heartyfs_inode *)((char *)buffer + current->entries[i].block_id * BLOCK_SIZE);
            free(path_copy);
            free(parent_path);
            return current->entries[i].block_id;
        }
    }

    free(path_copy);
    free(parent_path);
    return -1;
}

/**
 * @brief Find the parent directory and file index by its path
 * 
 * @param buffer - The buffer containing the disk image
 * @param path - The path of the file
 * @param parent_dir - The parent directory object to be returned
 * @param file_index - The index of the file in the parent directory
 * @return int - The block number of the file, -1 if not found
 */
int find_parent_directory_and_file_index(void *buffer, const char *path, struct heartyfs_directory **parent_dir, int *file_index) {
    char *path_copy = strdup(path);
    char *parent_path = dirname(strdup(path_copy));
    char *file_name = basename(path_copy);

    struct heartyfs_directory *current = buffer;
    int current_block_id = 0;

    char *token = strtok(parent_path, "/");
    while (token != NULL) {
        int found = 0;
        for (int i = 0; i < current->size; i++) {
            if (strcmp(current->entries[i].file_name, token) == 0) {
                current_block_id = current->entries[i].block_id;
                current = (struct heartyfs_directory *)((char *)buffer + current_block_id * BLOCK_SIZE);
                found = 1;
                break;
            }
        }
        if (!found) {
            free(path_copy);
            free(parent_path);
            return -1;
        }
        token = strtok(NULL, "/");
    }

    for (int i = 0; i < current->size; i++) {
        if (strcmp(current->entries[i].file_name, file_name) == 0) {
            *parent_dir = current;
            *file_index = i;
            free(path_copy);
            free(parent_path);
            return current->entries[i].block_id;
        }
    }

    free(path_copy);
    free(parent_path);
    return -1;
}
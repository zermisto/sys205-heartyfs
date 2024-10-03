/**
 * @file heartyfs_creat.c
 * @author Panupong Dangkajitpetch (King)
 * @brief This file creates a file in the heartyfs file system.
 * @version 0.1
 * @date 2024-10-03
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#include "../heartyfs.h"
#include "heartyfs_functions.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <libgen.h>

/**
 * @brief Find the first free block in the bitmap.
 * 
 * @param bitmap - The bitmap containing the block allocation information  
 * @return int - The block number of the first free block, -1 if no free block is found
 */
// int find_free_block(unsigned char *bitmap) {
//     for (int i = 2; i < NUM_BLOCK; i++) {
//         if (bitmap[i/8] & (1 << (7 - i%8))) {
//             return i;
//         }
//     }
//     return -1;
// }

// void set_block_used(unsigned char *bitmap, int block_num) {
//     bitmap[block_num/8] &= ~(1 << (7 - block_num%8));
// }

int find_directory(void *buffer, const char *path, struct heartyfs_directory **dir) {
    char *path_copy = strdup(path);
    char *parent_path = dirname(strdup(path_copy));

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

    *dir = current;
    free(path_copy);
    free(parent_path);
    return current_block_id;
}

int create_file(void *buffer, unsigned char *bitmap, const char *path) {
    char *path_copy = strdup(path);
    char *file_name = basename(path_copy);
    struct heartyfs_directory *parent_dir;

    int parent_block_id = find_directory(buffer, path, &parent_dir);
    if (parent_block_id == -1) {
        fprintf(stderr, "Error: Parent directory does not exist\n");
        free(path_copy);
        return -1;
    }

    // Check if file already exists
    for (int i = 0; i < parent_dir->size; i++) {
        if (strcmp(parent_dir->entries[i].file_name, file_name) == 0) {
            fprintf(stderr, "Error: File %s already exists\n", file_name);
            free(path_copy);
            return -1;
        }
    }

    // Check if parent directory is full
    if (parent_dir->size >= 14) {
        fprintf(stderr, "Error: Parent directory is full\n");
        free(path_copy);
        return -1;
    }

    // Find a free block for the inode
    int inode_block_id = find_free_block(bitmap);
    if (inode_block_id == -1) {
        fprintf(stderr, "Error: No free blocks available\n");
        free(path_copy);
        return -1;
    }

    // Mark the inode block as used
    set_block_used(bitmap, inode_block_id);

    // Initialize the inode
    struct heartyfs_inode *inode = (struct heartyfs_inode *)((char *)buffer + inode_block_id * BLOCK_SIZE);
    inode->type = 0;  // Regular file
    strncpy(inode->name, file_name, sizeof(inode->name) - 1);
    inode->size = 0;
    memset(inode->data_blocks, -1, sizeof(inode->data_blocks));

    // Add new entry to parent directory
    parent_dir->entries[parent_dir->size].block_id = inode_block_id;
    strncpy(parent_dir->entries[parent_dir->size].file_name, file_name, sizeof(parent_dir->entries[parent_dir->size].file_name) - 1);
    parent_dir->size++;

    free(path_copy);
    return 0;
}

int main(int argc, char *argv[]) {
    printf("heartyfs_creat\n");
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file_path>\n", argv[0]);
        return 1;
    }

    int fd = open(DISK_FILE_PATH, O_RDWR);
    if (fd < 0) {
        perror("Error: Cannot open the disk file");
        return 1;
    }

    void *buffer = mmap(NULL, DISK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (buffer == MAP_FAILED) {
        perror("Error: Cannot map the disk file onto memory");
        close(fd);
        return 1;
    }

    unsigned char *bitmap = (unsigned char *)buffer + BLOCK_SIZE;

    if (create_file(buffer, bitmap, argv[1]) == 0) {
        printf("Success: File %s created successfully\n", argv[1]);
    } else {
        fprintf(stderr, "Error: Failed to create file %s\n", argv[1]);
    }

    if (msync(buffer, DISK_SIZE, MS_SYNC) == -1) {
        perror("Error: Failed to sync changes to disk");
    }

    if (munmap(buffer, DISK_SIZE) == -1) {
        perror("Error: Failed to unmap file");
    }
    close(fd);

    return 0;
}
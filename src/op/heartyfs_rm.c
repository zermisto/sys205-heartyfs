/**
 * @file heartyfs_rm.c
 * @author Panupong Dangkajitpetch (King)
 * @brief This file removes a file from the heartyfs file system.
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

// void set_block_free(unsigned char *bitmap, int block_num) {
//     bitmap[block_num/8] |= (1 << (7 - block_num%8));
// }

// int find_file(void *buffer, const char *path, struct heartyfs_directory **parent_dir, int *file_index) {
//     char *path_copy = strdup(path);
//     char *parent_path = dirname(strdup(path_copy));
//     char *file_name = basename(path_copy);

//     struct heartyfs_directory *current = buffer;
//     int current_block_id = 0;

//     char *token = strtok(parent_path, "/");
//     while (token != NULL) {
//         int found = 0;
//         for (int i = 0; i < current->size; i++) {
//             if (strcmp(current->entries[i].file_name, token) == 0) {
//                 current_block_id = current->entries[i].block_id;
//                 current = (struct heartyfs_directory *)((char *)buffer + current_block_id * BLOCK_SIZE);
//                 found = 1;
//                 break;
//             }
//         }
//         if (!found) {
//             free(path_copy);
//             free(parent_path);
//             return -1;
//         }
//         token = strtok(NULL, "/");
//     }

//     for (int i = 0; i < current->size; i++) {
//         if (strcmp(current->entries[i].file_name, file_name) == 0) {
//             *parent_dir = current;
//             *file_index = i;
//             free(path_copy);
//             free(parent_path);
//             return current->entries[i].block_id;
//         }
//     }

//     free(path_copy);
//     free(parent_path);
//     return -1;
// }

int remove_file(void *buffer, unsigned char *bitmap, const char *path) {
    struct heartyfs_directory *parent_dir;
    int file_index;
    int inode_block_id = find_parent_directory_and_file_index(buffer, path, &parent_dir, &file_index);

    if (inode_block_id == -1) {
        fprintf(stderr, "Error: File %s does not exist\n", path);
        return -1;
    }

    struct heartyfs_inode *inode = (struct heartyfs_inode *)((char *)buffer + inode_block_id * BLOCK_SIZE);

    if (inode->type != 0) {
        fprintf(stderr, "Error: %s is not a regular file\n", path);
        return -1;
    }

    // Free data blocks
    for (int i = 0; i < 119 && inode->data_blocks[i] != -1; i++) {
        set_block_free(bitmap, inode->data_blocks[i]);
    }

    // Free inode block
    set_block_free(bitmap, inode_block_id);

    // Remove file entry from parent directory
    for (int i = file_index; i < parent_dir->size - 1; i++) {
        parent_dir->entries[i] = parent_dir->entries[i + 1];
    }
    parent_dir->size--;

    // Clear the inode block
    memset(inode, 0, BLOCK_SIZE);

    return 0;
}

int main(int argc, char *argv[]) {
    printf("heartyfs_rm\n");
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

    if (remove_file(buffer, bitmap, argv[1]) == 0) {
        printf("Success: File %s removed successfully\n", argv[1]);
    } else {
        fprintf(stderr, "Error: Failed to remove file %s\n", argv[1]);
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
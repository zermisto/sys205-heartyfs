/**
 * @file heartyfs_read.c
 * @author your name (you@domain.com)
 * @brief 
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

// int find_file(void *buffer, const char *path, struct heartyfs_inode **inode) {
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
//             *inode = (struct heartyfs_inode *)((char *)buffer + current->entries[i].block_id * BLOCK_SIZE);
//             free(path_copy);
//             free(parent_path);
//             return current->entries[i].block_id;
//         }
//     }

//     free(path_copy);
//     free(parent_path);
//     return -1;
// }

int read_file(void *buffer, const char *path) {
    struct heartyfs_inode *inode;
    int inode_block_id = find_inode_by_path(buffer, path, &inode);

    if (inode_block_id == -1) {
        fprintf(stderr, "Error: File %s does not exist\n", path);
        return -1;
    }

    if (inode->type != 0) {
        fprintf(stderr, "Error: %s is not a regular file\n", path);
        return -1;
    }

    int remaining = inode->size;
    int block_index = 0;

    while (remaining > 0 && block_index < 119) {
        int block_id = inode->data_blocks[block_index];
        if (block_id == -1) {
            break;
        }

        struct heartyfs_data_block *data_block = (struct heartyfs_data_block *)((char *)buffer + block_id * BLOCK_SIZE);
        int to_read = (remaining > data_block->size) ? data_block->size : remaining;

        if (write(STDOUT_FILENO, data_block->name, to_read) != to_read) {
            perror("Error: Failed to write to stdout");
            return -1;
        }

        remaining -= to_read;
        block_index++;
    }

    return 0;
}

int main(int argc, char *argv[]) {
    printf("heartyfs_read\n");
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file_path>\n", argv[0]);
        return 1;
    }

    int fd = open(DISK_FILE_PATH, O_RDONLY);
    if (fd < 0) {
        perror("Error: Cannot open the disk file");
        return 1;
    }

    void *buffer = mmap(NULL, DISK_SIZE, PROT_READ, MAP_PRIVATE, fd, 0);
    if (buffer == MAP_FAILED) {
        perror("Error: Cannot map the disk file onto memory");
        close(fd);
        return 1;
    }

    if (read_file(buffer, argv[1]) != 0) {
        fprintf(stderr, "Error: Failed to read file %s\n", argv[1]);
    }

    if (munmap(buffer, DISK_SIZE) == -1) {
        perror("Error: Failed to unmap file");
    }
    close(fd);

    return 0;
}
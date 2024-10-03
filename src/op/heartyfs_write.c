/**
 * @file heartyfs_write.c
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
#include <sys/stat.h>
#include <libgen.h>

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



int write_file(void *buffer, unsigned char *bitmap, const char *heartyfs_path, const char *external_path) {
    struct heartyfs_inode *inode;
    int inode_block_id = find_inode_by_path(buffer, heartyfs_path, &inode);

    if (inode_block_id == -1) {
        fprintf(stderr, "Error: File %s does not exist in heartyfs\n", heartyfs_path);
        return -1;
    }

    if (inode->type != 0) {
        fprintf(stderr, "Error: %s is not a regular file\n", heartyfs_path);
        return -1;
    }

    // Open the external file
    int ext_fd = open(external_path, O_RDONLY);
    if (ext_fd < 0) {
        perror("Error: Cannot open the external file");
        return -1;
    }

    // Get the size of the external file
    struct stat st;
    if (fstat(ext_fd, &st) < 0) {
        perror("Error: Cannot get file size");
        close(ext_fd);
        return -1;
    }

    // Check if the file size exceeds the heartyfs limit
    if (st.st_size > 119 * 508) {
        fprintf(stderr, "Error: File size exceeds heartyfs limit\n");
        close(ext_fd);
        return -1;
    }

    // Clear existing data blocks
    for (int i = 0; i < 119 && inode->data_blocks[i] != -1; i++) {
        set_block_used(bitmap, inode->data_blocks[i]);
        memset((char *)buffer + inode->data_blocks[i] * BLOCK_SIZE, 0, BLOCK_SIZE);
        inode->data_blocks[i] = -1;
    }

    // Write the file content
    int remaining = st.st_size;
    int block_index = 0;
    while (remaining > 0) {
        int block_id = find_free_block(bitmap);
        if (block_id == -1) {
            fprintf(stderr, "Error: No free blocks available\n");
            close(ext_fd);
            return -1;
        }

        set_block_used(bitmap, block_id);
        inode->data_blocks[block_index] = block_id;

        struct heartyfs_data_block *data_block = (struct heartyfs_data_block *)((char *)buffer + block_id * BLOCK_SIZE);
        int to_read = (remaining > 508) ? 508 : remaining;
        data_block->size = read(ext_fd, data_block->name, to_read);

        if (data_block->size < 0) {
            perror("Error: Failed to read from external file");
            close(ext_fd);
            return -1;
        }

        remaining -= data_block->size;
        block_index++;
    }

    inode->size = st.st_size;
    close(ext_fd);
    return 0;
}

int main(int argc, char *argv[]) {
    printf("heartyfs_write\n");
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <heartyfs_file_path> <external_file_path>\n", argv[0]);
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

    if (write_file(buffer, bitmap, argv[1], argv[2]) == 0) {
        printf("Success: File %s written to %s successfully\n", argv[2], argv[1]);
    } else {
        fprintf(stderr, "Error: Failed to write file %s to %s\n", argv[2], argv[1]);
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
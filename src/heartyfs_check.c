/**
 * @file heartyfs_check.c
 * @author Panupong Dangkajitpetch (King)
 * @brief This file checks the heartyfs file system by printing the superblock and bitmap.
 * @version 0.1
 * @date 2024-10-03
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#include "heartyfs.h"
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

/**
 * @brief Print the contents of the superblock (root directory).
 * 
 * @param buffer - the buffer containing the superblock
 */
void print_superblock(void *buffer) {
    struct heartyfs_directory *root = (struct heartyfs_directory *)buffer;
    
    printf("Superblock (Root Directory) Contents:\n");
    printf("Type: %d\n", root->type);
    printf("Name: %s\n", root->name);
    printf("Size: %d\n", root->size);
    
    printf("Entries:\n");
    for (int i = 0; i < root->size; i++) {
        printf("  Entry %d:\n", i);
        printf("    Block ID: %d\n", root->entries[i].block_id);
        printf("    File Name: %s\n", root->entries[i].file_name);
    }
}

/**
 * @brief Print the contents of the bitmap.
 * 
 * @param buffer - the buffer containing the bitmap
 */
void print_bitmap(void *buffer) {
    unsigned char *bitmap = (unsigned char *)buffer + BLOCK_SIZE;  // Start of Block 1
    int bitmap_size = (NUM_BLOCK - 2) / 8;  // in bytes, excluding superblock and bitmap block
    
    printf("\nBitmap Contents:\n");
    for (int i = 0; i < bitmap_size; i++) {
        for (int j = 7; j >= 0; j--) {
            printf("%d", (bitmap[i] >> j) & 1);
        }
        if ((i + 1) % 8 == 0) printf("\n");
        else printf(" ");
    }
    printf("\n");
}

int main() {
    printf("heartyfs_check\n");
    int fd = open(DISK_FILE_PATH, O_RDONLY);
    if (fd < 0) {
        perror("Cannot open the disk file");
        exit(1);
    }

    void *buffer = mmap(NULL, DISK_SIZE, PROT_READ, MAP_PRIVATE, fd, 0);
    if (buffer == MAP_FAILED) {
        perror("Cannot map the disk file onto memory");
        close(fd);
        exit(1);
    }

    print_superblock(buffer);
    print_bitmap(buffer);

    if (munmap(buffer, DISK_SIZE) == -1) {
        perror("Error unmapping file");
    }
    close(fd);

    return 0;
}
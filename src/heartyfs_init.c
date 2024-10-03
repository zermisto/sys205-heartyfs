/**
 * @file heartyfs_init.c
 * @author Panupong Dangkajitpetch (King)
 * @brief This file initializes the heartyfs file system with superblock and bitmap.
 * The superblock is stored at block 0 and acts as the metadata of the entire file system.
 * The bitmap will keep track of all the free blocks in the heartyfs
 * @version 0.1
 * @date 2024-10-03
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#include "heartyfs.h"
#include <string.h>
#include <unistd.h>

/**
 * @brief Initialize the superblock with the root directory.
 * 
 * @param buffer - The buffer containing the disk image
 */
void init_superblock(void *buffer) {
    struct heartyfs_directory *root = (struct heartyfs_directory *)buffer;
    
    root->type = 1;  // Directory type
    strncpy(root->name, "/", sizeof(root->name));
    root->size = 2;  // . and ..

    // Initialize . (current directory)
    root->entries[0].block_id = 0;
    strncpy(root->entries[0].file_name, ".", sizeof(root->entries[0].file_name));

    // Initialize .. (parent directory, same as . for root)
    root->entries[1].block_id = 0;
    strncpy(root->entries[1].file_name, "..", sizeof(root->entries[1].file_name));

    // Clear the rest of the entries
    for (int i = 2; i < 14; i++) {
        root->entries[i].block_id = -1;
        memset(root->entries[i].file_name, 0, sizeof(root->entries[i].file_name));
    }
}

/**
 * @brief Initialize the bitmap with all blocks marked as free.
 * 
 * @param buffer - The buffer containing the disk image
 */
void init_bitmap(void *buffer) {
    unsigned char *bitmap = (unsigned char *)buffer + BLOCK_SIZE;  // Start of Block 1
    int bitmap_size = (NUM_BLOCK - 2) / 8;  // in bytes, excluding superblock and bitmap block (block 0 and 1). 1 byte = 8 blocks
    
    memset(bitmap, 0xFF, bitmap_size);  // Set all bits to 1 (free)

    // Set first byte of bitmap to 0xFC (11111100) to mark the first two blocks as used (superblock and bitmap)
    bitmap[0] = 0xFC;  // 11111100 in binary
}   

int main() {
    printf("heartyfs_innit\n");
    // Open the disk file
    int fd = open(DISK_FILE_PATH, O_RDWR);
    if (fd < 0) {
        perror("Cannot open the disk file\n");
        exit(1);
    }

    // Map the disk file onto memory
    void *buffer = mmap(NULL, DISK_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if (buffer == MAP_FAILED) {
        perror("Cannot map the disk file onto memory\n");
        exit(1);
    }

    // TODO:
    printf("Disk file mapped to memory successfully.\n");

    // Initialize superblock and bitmap
    init_superblock(buffer);
    init_bitmap(buffer);

    printf("Superblock and bitmap initialized.\n");

    // Sync changes to disk
    if (msync(buffer, DISK_SIZE, MS_SYNC) == -1) {
        perror("Error syncing changes to disk\n");
    }

    // Unmap the file and close
    if (munmap(buffer, DISK_SIZE) == -1) {
        perror("Error unmapping file\n");
    }
    close(fd);

    printf("heartyfs initialized successfully.\n");
    return 0;
}

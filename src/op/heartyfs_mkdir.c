/**
 * @file heartyfs_rmdir.c
 * @author Panupong Dangkajitpetch (King)
 * @brief This file makes a directory from the heartyfs file system.
 * @version 0.1
 * @date 2024-10-03
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#include "../heartyfs.h"
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <libgen.h>
#include <stdlib.h>

/**
 * @brief Check if the heartyfs is initialized. 
 * The root directory should have type 1 and name "/".
 * @param buffer - The buffer containing the disk image
 * @return int - 1 if initialized, 0 otherwise
 */
int is_initialized(void *buffer) {
    struct heartyfs_directory *root = (struct heartyfs_directory *)buffer;
    return (root->type == 1 && strcmp(root->name, "/") == 0);
}

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
 * @brief Find a directory by its path
 * 
 * @param buffer - The buffer containing the disk image
 * @param path - The path of the directory
 * @param dir - The directory object to be returned
 * @return int - The block number of the directory, -1 if not found
 */
int find_directory(void *buffer, const char *path, struct heartyfs_directory **dir) {
    char *path_copy = strdup(path); // Copy the path to avoid modifying the original
    char *token = strtok(path_copy, "/"); // Tokenize the path by '/'
    struct heartyfs_directory *current = buffer; // Start from the root directory
    int block_id = 0; // Start from the root directory block

    // Traverse the path to find the directory
    while (token != NULL) {
        int found = 0;
        for (int i = 0; i < current->size; i++) {
            if (strcmp(current->entries[i].file_name, token) == 0) {
                block_id = current->entries[i].block_id;
                current = (struct heartyfs_directory *)((char *)buffer + block_id * BLOCK_SIZE);
                found = 1; // Found the directory
                break;
            }
        }
        if (!found) { // Directory not found
            free(path_copy);
            return -1;
        }
        token = strtok(NULL, "/"); // Move to the next token
    }

    *dir = current;
    free(path_copy);
    return block_id;
}

/**
 * @brief Create a directory object
 * 
 * @param buffer - The buffer containing the disk image
 * @param bitmap - The bitmap containing the block allocation information
 * @param path - The path of the directory to create
 * @return int - 0 if successful, -1 if failed
 */
int create_directory(void *buffer, unsigned char *bitmap, const char *path) {
    char *path_copy = strdup(path); // Copy the path to avoid modifying the original
    char *parent_path = dirname(strdup(path_copy)); // Get the parent path
    char *dir_name = basename(path_copy); // Get the directory name

    struct heartyfs_directory *parent_dir; // Parent directory object
    int parent_block_id = find_directory(buffer, parent_path, &parent_dir); // Find the parent directory

    if (parent_block_id == -1) {
        // Parent directory doesn't exist, try to create it recursively
        if (create_directory(buffer, bitmap, parent_path) != 0) {
            fprintf(stderr, "Error: Failed to create parent directory %s\n", parent_path);
            free(path_copy);
            free(parent_path);
            return -1;
        }
        // After creating parent, find it again
        parent_block_id = find_directory(buffer, parent_path, &parent_dir);
    }

    // Check if the directory already exists
    for (int i = 0; i < parent_dir->size; i++) {
        if (strcmp(parent_dir->entries[i].file_name, dir_name) == 0) {
            fprintf(stderr, "Error: Directory %s already exists\n", dir_name);
            free(path_copy);
            free(parent_path);
            return -1;
        }
    }

    // Check if the parent directory is full
    if (parent_dir->size >= 14) {
        fprintf(stderr, "Error: Parent directory %s is full\n", parent_path);
        free(path_copy);
        free(parent_path);
        return -1;
    }

    // Get a free block
    int new_block_id = find_free_block(bitmap);
    if (new_block_id == -1) {
        fprintf(stderr, "Error: No free blocks available\n");
        free(path_copy);
        free(parent_path);
        return -1;
    }

    // Mark the block as used
    set_block_used(bitmap, new_block_id);

    // Initialize the new directory block
    struct heartyfs_directory *new_dir = (struct heartyfs_directory *)((char *)buffer + new_block_id * BLOCK_SIZE);
    new_dir->type = 1;
    strncpy(new_dir->name, dir_name, sizeof(new_dir->name) - 1);
    new_dir->size = 2;

    // Set . entry
    new_dir->entries[0].block_id = new_block_id;
    strcpy(new_dir->entries[0].file_name, ".");

    // Set .. entry
    new_dir->entries[1].block_id = parent_block_id;
    strcpy(new_dir->entries[1].file_name, "..");

    // Add new entry to parent directory
    parent_dir->entries[parent_dir->size].block_id = new_block_id;
    strncpy(parent_dir->entries[parent_dir->size].file_name, dir_name, sizeof(parent_dir->entries[parent_dir->size].file_name) - 1);
    parent_dir->size++;

    free(path_copy);
    free(parent_path);
    return 0;
}

int main(int argc, char *argv[]) {
    printf("heartyfs_mkdir\n");
    if (argc != 2) {
        //ex: bin/heartyfs_mkdir /dir1/dir2/dir3/
        fprintf(stderr, "Usage: %s <directory_path>\n", argv[0]);
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

    // Check if heartyfs is initialized
    if (!is_initialized(buffer)) {
        fprintf(stderr, "Error: heartyfs is not initialized\n");
        munmap(buffer, DISK_SIZE);
        close(fd);
        return 1;
    }

    unsigned char *bitmap = (unsigned char *)buffer + BLOCK_SIZE; // Start of Block 1

    // Create the directory
    if (create_directory(buffer, bitmap, argv[1]) == 0) {
        printf("Success: Directory %s created successfully\n", argv[1]);
    } else {
        fprintf(stderr, "Error: Failed to create directory %s\n", argv[1]);
    }
    // Sync changes to disk
    if (msync(buffer, DISK_SIZE, MS_SYNC) == -1) {
        perror("Error: Failed to sync changes to disk");
    }
    // Unmap the file and close
    if (munmap(buffer, DISK_SIZE) == -1) {
        perror("Error: Failed to unmap file");
    }
    close(fd);

    return 0;
}
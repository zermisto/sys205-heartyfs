#include "../heartyfs.h"
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <libgen.h>
#include <stdlib.h>

int is_initialized(void *buffer) {
    struct heartyfs_directory *root = (struct heartyfs_directory *)buffer;
    return (root->type == 1 && strcmp(root->name, "/") == 0);
}

int find_free_block(unsigned char *bitmap) {
    for (int i = 2; i < NUM_BLOCK; i++) {
        if (bitmap[i/8] & (1 << (7 - i%8))) {
            return i;
        }
    }
    return -1;
}

void set_block_used(unsigned char *bitmap, int block_num) {
    bitmap[block_num/8] &= ~(1 << (7 - block_num%8));
}

int find_directory(void *buffer, const char *path, struct heartyfs_directory **dir) {
    char *path_copy = strdup(path);
    char *token = strtok(path_copy, "/");
    struct heartyfs_directory *current = buffer;
    int block_id = 0;

    while (token != NULL) {
        int found = 0;
        for (int i = 0; i < current->size; i++) {
            if (strcmp(current->entries[i].file_name, token) == 0) {
                block_id = current->entries[i].block_id;
                current = (struct heartyfs_directory *)((char *)buffer + block_id * BLOCK_SIZE);
                found = 1;
                break;
            }
        }
        if (!found) {
            free(path_copy);
            return -1;
        }
        token = strtok(NULL, "/");
    }

    *dir = current;
    free(path_copy);
    return block_id;
}

int create_directory(void *buffer, unsigned char *bitmap, const char *path) {
    char *path_copy = strdup(path);
    char *parent_path = dirname(strdup(path_copy));
    char *dir_name = basename(path_copy);

    struct heartyfs_directory *parent_dir;
    int parent_block_id = find_directory(buffer, parent_path, &parent_dir);

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
    if (argc != 2) {
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

    unsigned char *bitmap = (unsigned char *)buffer + BLOCK_SIZE;

    if (create_directory(buffer, bitmap, argv[1]) == 0) {
        printf("Success: Directory %s created successfully\n", argv[1]);
    } else {
        fprintf(stderr, "Error: Failed to create directory %s\n", argv[1]);
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
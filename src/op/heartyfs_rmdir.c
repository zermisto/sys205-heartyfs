#include "../heartyfs.h"
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <libgen.h>
#include <stdlib.h>

void set_block_free(unsigned char *bitmap, int block_num) {
    bitmap[block_num/8] |= (1 << (7 - block_num%8));
}

int find_directory(void *buffer, const char *path, struct heartyfs_directory **dir, struct heartyfs_directory **parent_dir, int *dir_index) {
    char *path_copy = strdup(path);
    char *parent_path = dirname(strdup(path_copy));
    char *dir_name = basename(path_copy);

    struct heartyfs_directory *current = buffer;
    struct heartyfs_directory *parent = NULL;
    int current_block_id = 0;
    int found_index = -1;

    char *token = strtok(parent_path, "/");
    while (token != NULL) {
        int found = 0;
        for (int i = 0; i < current->size; i++) {
            if (strcmp(current->entries[i].file_name, token) == 0) {
                parent = current;
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
        if (strcmp(current->entries[i].file_name, dir_name) == 0) {
            *dir = (struct heartyfs_directory *)((char *)buffer + current->entries[i].block_id * BLOCK_SIZE);
            *parent_dir = current;
            *dir_index = i;
            found_index = current->entries[i].block_id;
            break;
        }
    }

    free(path_copy);
    free(parent_path);
    return found_index;
}

int remove_directory(void *buffer, unsigned char *bitmap, const char *path) {
    struct heartyfs_directory *dir;
    struct heartyfs_directory *parent_dir;
    int dir_index;
    int dir_block_id = find_directory(buffer, path, &dir, &parent_dir, &dir_index);

    if (dir_block_id == -1) {
        fprintf(stderr, "Error: Directory %s does not exist\n", path);
        return -1;
    }

    if (dir->type != 1) {
        fprintf(stderr, "Error: %s is not a directory\n", path);
        return -1;
    }

    if (dir->size > 2) {
        fprintf(stderr, "Error: Directory %s is not empty\n", path);
        return -1;
    }

    // Remove the directory entry from its parent
    for (int i = dir_index; i < parent_dir->size - 1; i++) {
        parent_dir->entries[i] = parent_dir->entries[i + 1];
    }
    parent_dir->size--;

    // Mark the block as free in the bitmap
    set_block_free(bitmap, dir_block_id);

    // Clear the directory block
    memset(dir, 0, BLOCK_SIZE);

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

    unsigned char *bitmap = (unsigned char *)buffer + BLOCK_SIZE;

    if (remove_directory(buffer, bitmap, argv[1]) == 0) {
        printf("Success: Directory %s removed successfully\n", argv[1]);
    } else {
        fprintf(stderr, "Error: Failed to remove directory %s\n", argv[1]);
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
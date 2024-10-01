#include "../heartyfs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

void list_directory(void *buffer, const char *path) {
    struct heartyfs_directory *current_dir = (struct heartyfs_directory *)buffer; // Start at root

    // If path is not root, find the correct directory
    if (strcmp(path, "/") != 0) {
        char *path_copy = strdup(path);
        char *token = strtok(path_copy, "/");
        
        while (token != NULL) {
            int found = 0;
            for (int i = 0; i < current_dir->size; i++) {
                if (strcmp(current_dir->entries[i].file_name, token) == 0) {
                    current_dir = (struct heartyfs_directory *)((char *)buffer + current_dir->entries[i].block_id * BLOCK_SIZE);
                    found = 1;
                    break;
                }
            }
            if (!found) {
                fprintf(stderr, "Error: Directory %s not found\n", path);
                free(path_copy);
                return;
            }
            token = strtok(NULL, "/");
        }
        free(path_copy);
    }

    // List contents of the directory
    printf("Contents of directory %s:\n", path);
    for (int i = 0; i < current_dir->size; i++) {
        struct heartyfs_directory *entry_dir = (struct heartyfs_directory *)((char *)buffer + current_dir->entries[i].block_id * BLOCK_SIZE);
        struct heartyfs_inode *entry_inode = (struct heartyfs_inode *)((char *)buffer + current_dir->entries[i].block_id * BLOCK_SIZE);
        
        if (entry_dir->type == 1) { // Directory
            printf("d %s\n", current_dir->entries[i].file_name);
        } else if (entry_inode->type == 0) { // File
            printf("f %s\n", current_dir->entries[i].file_name);
        } else {
            printf("? %s\n", current_dir->entries[i].file_name);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <directory_path>\n", argv[0]);
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

    list_directory(buffer, argv[1]);

    if (munmap(buffer, DISK_SIZE) == -1) {
        perror("Error: Failed to unmap file");
    }
    close(fd);

    return 0;
}
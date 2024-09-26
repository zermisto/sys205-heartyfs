#include "../heartyfs.h"
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

struct heartyfs_dir_entry {
    int block_id;
    char file_name[28];
};

struct heartyfs_directory {
    int type;
    char name[28];
    int size;
    struct heartyfs_dir_entry entries[14];
};

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
    char *parent_path = dirname(path_copy);
    char *dir_name = basename(strcpy(malloc(strlen(path) + 1), path));

    struct heartyfs_directory *parent_dir;
    int parent_block_id = find_directory(buffer, parent_path, &parent_dir);

    if (parent_block_id == -1) {
        fprintf(stderr, "Parent directory does not exist\n");
        free(path_copy);
        free(dir_name);
        return -1;
    }

    if (parent_dir->size >= 14) {
        fprintf(stderr, "Parent directory is full\n");
        free(path_copy);
        free(dir_name);
        return -1;
    }

    int new_block_id = find_free_block(bitmap);
    if (new_block_id == -1) {
        fprintf(stderr, "No free blocks available\n");
        free(path_copy);
        free(dir_name);
        return -1;
    }

    set_block_used(bitmap, new_block_id);

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
    free(dir_name);
    return 0;
}

int main(int argc, char *argv[]) {
    printf("heartyfs_mkdir\n");
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <directory_path>\n", argv[0]);
        return 1;
    }

    int fd = open(DISK_FILE_PATH, O_RDWR);
    if (fd < 0) {
        perror("Cannot open the disk file");
        return 1;
    }

    void *buffer = mmap(NULL, DISK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (buffer == MAP_FAILED) {
        perror("Cannot map the disk file onto memory");
        close(fd);
        return 1;
    }

    if (!is_initialized(buffer)) {
        fprintf(stderr, "heartyfs is not initialized\n");
        munmap(buffer, DISK_SIZE);
        close(fd);
        return 1;
    }

    unsigned char *bitmap = (unsigned char *)buffer + BLOCK_SIZE;

    if (create_directory(buffer, bitmap, argv[1]) == 0) {
        printf("Directory created successfully\n");
    } else {
        fprintf(stderr, "Failed to create directory\n");
    }

    if (msync(buffer, DISK_SIZE, MS_SYNC) == -1) {
        perror("Error syncing changes to disk");
    }

    if (munmap(buffer, DISK_SIZE) == -1) {
        perror("Error unmapping file");
    }
    close(fd);

    return 0;
}
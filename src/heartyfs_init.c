#include "heartyfs.h"
#include <string.h>
#include <unistd.h>

struct heartyfs_dir_entry {
    int block_id;   //4 bytes
    char file_name[28]; //28 bytes
};   //Overall: 32 bytes

struct heartyfs_directory {
    int type;
    char name[28];
    int size;
    struct heartyfs_dir_entry entries[14];
};


struct heartyfs_inode {
    int type;               // 4 bytes
    char name[28];          // 28 bytes
    int size;               // 4 bytes
    int data_blocks[119];   // 476 bytes
};  // Overall: 512 bytes

struct heartyfs_data_block {
    int size;               // 4 bytes
    char name[508];         // 508 bytes
};  // Overall: 512 bytes

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

void init_bitmap(void *buffer) {
    unsigned char *bitmap = (unsigned char *)buffer + BLOCK_SIZE;  // Start of Block 1
    int bitmap_size = (NUM_BLOCK - 2) / 8;  // in bytes, excluding superblock and bitmap block
    
    memset(bitmap, 0xFF, bitmap_size);  // Set all bits to 1 (free)

    // Mark superblock and bitmap block as used
    bitmap[0] = 0xFC;  // 11111100 in binary
}   

int main() {
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
    printf("before init superblock\n");
    init_superblock(buffer);
    printf("after init superblock\n");
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

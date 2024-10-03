#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

#define DISK_FILE_PATH "/tmp/heartyfs"
#define BLOCK_SIZE (1 << 9) // 512 bytes
#define DISK_SIZE (1 << 20) // 1 MB
#define NUM_BLOCK (DISK_SIZE / BLOCK_SIZE) // 2048 blocks (1,048,576 bytes/512 bytes)


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
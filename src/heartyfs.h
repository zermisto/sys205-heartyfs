#ifndef HEARTYFS_H
#define HEARTYFS_H

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

#define DISK_FILE_PATH "/tmp/heartyfs"
#define BLOCK_SIZE (1 << 9) // 512 bytes
#define DISK_SIZE (1 << 20) // 1 MB
#define NUM_BLOCK (DISK_SIZE / BLOCK_SIZE) // 2048 blocks (1,048,576 bytes/512 bytes)
#define FILENAME_MAXLEN 28     // Maximum length for file/directory names
#define DIR_MAX_ENTRIES 14     // Maximum number of directory entries
#define INODE_BLOCKS 119       // Maximum number of data blocks an inode can reference
#define DATA_BLOCK_NAME_SIZE 508  // Space for data within a data block


    struct heartyfs_dir_entry {
        int block_id;   //4 bytes
        char file_name[FILENAME_MAXLEN]; //28 bytes
    };   //Overall: 32 bytes

    struct heartyfs_directory {
        int type;
        char name[FILENAME_MAXLEN];
        int size;
        struct heartyfs_dir_entry entries[DIR_MAX_ENTRIES];
    };


    struct heartyfs_inode {
        int type;   // 4 bytes
        char name[FILENAME_MAXLEN]; // 28 bytes
        int size;   // 4 bytes
        int data_blocks[INODE_BLOCKS];   // 476 bytes
    };  // Overall: 512 bytes

    struct heartyfs_data_block {
        int size;   // 4 bytes
        char name[DATA_BLOCK_NAME_SIZE];    // 508 bytes
    };  // Overall: 512 bytes

#endif // HEARTYFS_H
/*
 * lab3a.c
 * Copyright (C) 2019 Qingwei Zeng <zenn@ucla.edu>
 * ID: 005181694
 *
 * Distributed under terms of the MIT license.
 */

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stats.h>
#include <time.h>
#include <unistd.h>

#ifndef EXT2_FS_H
#define EXT2_FS_H
#include "ext2_fs.h"
#endif

#include "lab3a.h"

#define PROGRAM_NAME "lab3a"
#define AUTHORS proper_name("Qingwei Zeng")

char const program_name[] = PROGRAM_NAME;

/* global variables */
int fs_fd;
superblock_t super_block;
__u32 block_size, descriptor_block_index, group_count;

unsigned long _offset(__u32 base, __u32 index, __u32 scale) {
  return base + index * scale;
}

/**
 * system call error checker
 *
 * Check if the system call is successful. If not, prints out the given error
 * message as well as the error message related to the errno. Then, exits with
 * EXIT_FAILURE status.
 */
void _c(int ret, char *errmsg);

/**
 * memory allocation error checker
 *
 * Check if the memory allocation is successful. If not, prints out the given
 * error message. Then exits with EXIT_FAILURE status.
 */
void _m(void *ret, char *name);

unsigned int b_to_B(unsigned int num_bits) { return num_bits / 8 + 1; }

unsigned long block_offset(__u32 block_index) {
  return _offset(BOOT_BLOCK_SIZE, block_index - 1, block_size);
}

/* store the time in the format mm/dd/yy hh:mm:ss, GMT
 * 'raw_time' is a 32 bit value representing the number of
 * seconds since January 1st, 1970
 */
void get_time(time_t raw_time, char *buf) {
  time_t epoch = raw_time;
  struct tm ts = *gmtime(&epoch);
  strftime(buf, 80, "%m/%d/%y %H:%M:%S", &ts);
}

/* given location of directory entry block, produce directory entry summary */
void read_dir_entry(unsigned int parent_inode, unsigned int block_num) {
  struct ext2_dir_entry dir_entry;
  unsigned long offset = block_offset(block_num);
  unsigned int num_bytes = 0;

  while (num_bytes < block_size) {
    memset(dir_entry.name, 0, 256);
    pread(fs_fd, &dir_entry, sizeof(dir_entry), offset + num_bytes);
    if (dir_entry.inode != 0) {  // entry is not empty
      memset(&dir_entry.name[dir_entry.name_len], 0, 256 - dir_entry.name_len);
      printf("DIRENT,%d,%d,%d,%d,%d,'%s'\n",
             parent_inode,        // parent inode number
             num_bytes,           // logical byte offset
             dir_entry.inode,     // inode number of the referenced file
             dir_entry.rec_len,   // entry length
             dir_entry.name_len,  // name length
             dir_entry.name       // name, string, surrounded by single-quotes
      );
    }
    num_bytes += dir_entry.rec_len;
  }
}

/* for an allocated inode, print its summary */
void read_inode(unsigned int inode_table_id, unsigned int index,
                unsigned int inode_num) {
  struct ext2_inode inode;

  unsigned long offset = block_offset(inode_table_id) + index * sizeof(inode);
  _c(pread(fs_fd, &inode, sizeof(inode), offset), "Failed to read inode");

  // Check if inode is actually freed
  if (inode.i_mode == 0 || inode.i_links_count == 0) return;

  char filetype = '?';
  if (S_ISLNK(inode.i_mode)) filetype = 's';
  if (S_ISREG(inode.i_mode)) filetype = 'f';
  if (S_ISDIR(inode.i_mode)) filetype = 'd';

  unsigned int num_blocks =
      2 * (inode.i_blocks / (2 << super_block.s_log_block_size));

  printf("INODE,%d,%c,%o,%d,%d,%d,",
         inode_num,             // inode number
         filetype,              // filetype
         inode.i_mode & 0xFFF,  // mode, low order 12-bits
         inode.i_uid,           // owner
         inode.i_gid,           // group
         inode.i_links_count    // link count
  );

  char ctime[20], mtime[20], atime[20];
  get_time(inode.i_ctime, ctime);  // creation time
  get_time(inode.i_mtime, mtime);  // modification time
  get_time(inode.i_atime, atime);  // access time
  printf("%s,%s,%s,", ctime, mtime, atime);

  printf("%d,%d",
         inode.i_size,  // file size
         num_blocks     // number of blocks
  );

  unsigned int i;
  for (i = 0; i < 15; i++) {  // block addresses
    printf(",%d", inode.i_block[i]);
  }
  printf("\n");

  // if the filetype is a directory, need to create a directory entry
  for (i = 0; i < 12; i++) {  // direct entries
    if (inode.i_block[i] != 0 && filetype == 'd') {
      read_dir_entry(inode_num, inode.i_block[i]);
    }
  }

  // indirect entry
  if (inode.i_block[12] != 0) {
    uint32_t *block_ptrs = malloc(block_size);
    uint32_t num_ptrs = block_size / sizeof(uint32_t);

    unsigned long indir_offset = block_offset(inode.i_block[12]);
    pread(fs_fd, block_ptrs, block_size, indir_offset);

    unsigned int j;
    for (j = 0; j < num_ptrs; j++) {
      if (block_ptrs[j] != 0) {
        if (filetype == 'd') {
          read_dir_entry(inode_num, block_ptrs[j]);
        }
        printf(
            "INDIRECT,%d,%d,%d,%d,%d\n",
            inode_num,          // inode number
            1,                  // level of indirection
            12 + j,             // logical block offset
            inode.i_block[12],  // block number of indirect block being scanned
            block_ptrs[j]       // block number of reference block
        );
      }
    }
    free(block_ptrs);
  }

  // doubly indirect entry
  if (inode.i_block[13] != 0) {
    uint32_t *indir_block_ptrs = malloc(block_size);
    uint32_t num_ptrs = block_size / sizeof(uint32_t);

    unsigned long indir2_offset = block_offset(inode.i_block[13]);
    pread(fs_fd, indir_block_ptrs, block_size, indir2_offset);

    unsigned int j;
    for (j = 0; j < num_ptrs; j++) {
      if (indir_block_ptrs[j] != 0) {
        printf(
            "INDIRECT,%d,%d,%d,%d,%d\n",
            inode_num,           // inode number
            2,                   // level of indirection
            256 + 12 + j,        // logical block offset
            inode.i_block[13],   // block number of indirect block being scanned
            indir_block_ptrs[j]  // block number of reference block
        );

        // search through this indirect block to find its directory entries
        uint32_t *block_ptrs = malloc(block_size);
        unsigned long indir_offset = block_offset(indir_block_ptrs[j]);
        pread(fs_fd, block_ptrs, block_size, indir_offset);

        unsigned int k;
        for (k = 0; k < num_ptrs; k++) {
          if (block_ptrs[k] != 0) {
            if (filetype == 'd') {
              read_dir_entry(inode_num, block_ptrs[k]);
            }
            printf("INDIRECT,%d,%d,%d,%d,%d\n",
                   inode_num,            // inode number
                   1,                    // level of indirection
                   256 + 12 + k,         // logical block offset
                   indir_block_ptrs[j],  // block number of indirect block
                   // being scanned
                   block_ptrs[k]  // block number of reference block
            );
          }
        }
        free(block_ptrs);
      }
    }
    free(indir_block_ptrs);
  }

  // triply indirect entry
  if (inode.i_block[14] != 0) {
    uint32_t *indir2_block_ptrs = malloc(block_size);
    uint32_t num_ptrs = block_size / sizeof(uint32_t);

    unsigned long indir3_offset = block_offset(inode.i_block[14]);
    pread(fs_fd, indir2_block_ptrs, block_size, indir3_offset);

    unsigned int j;
    for (j = 0; j < num_ptrs; j++) {
      if (indir2_block_ptrs[j] != 0) {
        printf(
            "INDIRECT,%d,%d,%d,%d,%d\n",
            inode_num,             // inode number
            3,                     // level of indirection
            65536 + 256 + 12 + j,  // logical block offset
            inode.i_block[14],  // block number of indirect block being scanned
            indir2_block_ptrs[j]  // block number of reference block
        );

        uint32_t *indir_block_ptrs = malloc(block_size);
        unsigned long indir2_offset = block_offset(indir2_block_ptrs[j]);
        pread(fs_fd, indir_block_ptrs, block_size, indir2_offset);

        unsigned int k;
        for (k = 0; k < num_ptrs; k++) {
          if (indir_block_ptrs[k] != 0) {
            printf("INDIRECT,%d,%d,%d,%d,%d\n",
                   inode_num,             // inode number
                   2,                     // level of indirection
                   65536 + 256 + 12 + k,  // logical block offset
                   indir2_block_ptrs[j],  // block number of indirect block
                   // being scanned
                   indir_block_ptrs[k]  // block number of reference block
            );
            uint32_t *block_ptrs = malloc(block_size);
            unsigned long indir_offset = block_offset(indir_block_ptrs[k]);
            pread(fs_fd, block_ptrs, block_size, indir_offset);

            unsigned int l;
            for (l = 0; l < num_ptrs; l++) {
              if (block_ptrs[l] != 0) {
                if (filetype == 'd') {
                  read_dir_entry(inode_num, block_ptrs[l]);
                }
                printf("INDIRECT,%d,%d,%d,%d,%d\n",
                       inode_num,             // inode number
                       1,                     // level of indirection
                       65536 + 256 + 12 + l,  // logical block offset
                       indir_block_ptrs[k],   // block number of indirect block
                       // being scanned
                       block_ptrs[l]  // block number of reference block
                );
              }
            }
            free(block_ptrs);
          }
        }
        free(indir_block_ptrs);
      }
    }
    free(indir2_block_ptrs);
  }
}

/**
 * usage
 *
 * prints out the usage of this function. Then exits with EXIT_FAILURE status.
 */
void usage();

void analyze_group(__u32 group_index) {
  char *block_bitmap, *inode_bitmap;
  group_descriptor_t group_descriptor;
  __u32 num_inodes_in_group, num_blocks_in_group;
  __u32 inode_bitmap_size = b_to_B(super_block.s_inodes_per_group);

  _m(inode_bitmap = (char *)malloc(inode_bitmap_size), "inode bitmap");
  _m(block_bitmap = (char *)malloc(block_size), "block bitmap");
  // Initialize the group descriptor for this group
  _c(pread(fs_fd, &group_descriptor, sizeof(group_descriptor),
           block_size * descriptor_block_index +
               group_index * sizeof(group_descriptor)),
     "Failed to read the group descriptor");

  // Calculate number of block in this group
  if (group_index == group_count - 1) {
    num_blocks_in_group = super_block.s_blocks_count -
                          super_block.s_blocks_per_group * (group_count - 1);
    num_inodes_in_group = super_block.s_inodes_count -
                          super_block.s_inodes_per_group * (group_count - 1);
  } else {
    num_blocks_in_group = super_block.s_blocks_per_group;
    num_inodes_in_group = super_block.s_inodes_per_group;
  }

  // Print group summary
  printf(
      "GROUP,%d,%d,%d,%d,%d,%d,%d,%d\n",
      group_index,          // group number (decimal, starting from zero)
      num_blocks_in_group,  // total number of blocks in this group (decimal)
      num_inodes_in_group,  // total number of inodes (decimal)
      group_descriptor.bg_free_blocks_count,  // number of free blocks (decimal)
      group_descriptor.bg_free_inodes_count,  // number of free inodes (decimal)
      // block number of free block bitmap for this group (decimal)
      group_descriptor.bg_block_bitmap,
      // block number of free inode bitmap for this group (decimal)
      group_descriptor.bg_inode_bitmap,
      // block number of first block of i-nodes in this group (decimal)
      group_descriptor.bg_inode_table);

  // Read the block bitmap for this group
  _c(pread(fs_fd, block_bitmap, block_size,
           block_offset(group_descriptor.bg_block_bitmap)),
     "Failed to read the block bitmap");

  // Set block_index to the index of the first block in this group
  __u32 block_index = _offset(super_block.s_first_data_block, group_index,
                              super_block.s_blocks_per_group);
  // Scan through the block bitmap bit by bit
  for (__u32 i = 0; i < block_size; i++) {
    char byte = block_bitmap[i];
    for (short j = 0; j < BIT_PER_BYTE; j++) {
      bool allocated = byte & 1;
      if (!allocated) printf("BFREE,%d\n", block_index);
      byte >>= 1;
      block_index++;
    }
  }

  __u32 group_inode_start =
      _offset(1, group_index, super_block.s_inodes_per_group);
  __u32 inode_index = group_inode_start;
  _c(pread(fs_fd, inode_bitmap, inode_bitmap_size,
           block_offset(group_descriptor.bg_inode_bitmap)),
     "Failed to read the inode bitmap");

  for (__u32 i = 0; i < inode_bitmap_size; i++) {
    char byte = inode_bitmap[i];
    for (int j = 0; j < BIT_PER_BYTE; j++) {
      bool allocated = byte & 1;
      if (allocated) {
        read_inode(group_descriptor.bg_inode_table,
                   inode_index - group_inode_start, inode_index);
      } else
        printf("IFREE,%d\n", inode_index);
      byte >>= 1;
      inode_index++;
    }
  }

  // Free allocated memory
  free(block_bitmap);
  free(inode_bitmap);
};

int main(int argc, char *argv[]) {
  // Check the number of arguments; should be exactly 1
  if (argc != 2) {
    fprintf(stderr, "The program takes exactly 1 argument, %d were given.\n",
            argc - 1);
    usage();
  }
  // Open the file system image
  _c(fs_fd = open(argv[1], O_RDONLY),
     "Failed to open the given file system image file");
  // Intialize super_block structure
  _c(pread(fs_fd, &super_block, sizeof(super_block), BOOT_BLOCK_SIZE),
     "Failed to read the super block");
  block_size = EXT2_MIN_BLOCK_SIZE << super_block.s_log_block_size;
  // Print super block summary
  printf("SUPERBLOCK,%d,%d,%d,%d,%d,%d,%d\n",
         super_block.s_blocks_count,      // total number of blocks (decimal)
         super_block.s_inodes_count,      // total number of inodes (decimal)
         block_size,                      // block size (in bytes, decimal)
         super_block.s_inode_size,        // i-node size (in bytes, decimal)
         super_block.s_blocks_per_group,  // blocks per group (decimal)
         super_block.s_inodes_per_group,  // inodes per group (decimal)
         super_block.s_first_ino          // first non-reserved inode (decimal)
  );

  // Calculate number of block groups on the disk
  group_count =
      1 + (super_block.s_blocks_count - 1) / super_block.s_blocks_per_group;
  // Locate the group descriptors
  if (block_size < 1024) {
    fprintf(stderr, "Block size can't be less than 1024. Something's wrong.\n");
    exit(EXIT_FAILURE);
  }
  descriptor_block_index = block_size > 1024 ? 1 : 2;
  // Analyze each block group
  for (__u32 group_index = 0; group_index < group_count; group_index++)
    analyze_group(group_index);

  exit(EXIT_SUCCESS);
}

void usage() {
  fprintf(stderr,
          "\n\
Usage: %s DISKIMAGE\n\
\n\
",
          program_name);
  fputs(
      "\
DISKIMAGE                   the file system image to analyze\n\
",
      stderr);
  exit(EXIT_FAILURE);
}

void _c(int ret, char *errmsg) {
  if (ret != -1) return;  // syscall suceeded
  fprintf(stderr, "%s: %s. errno %d\n", errmsg, strerror(errno), errno);
  exit(EXIT_FAILURE);
}

void _m(void *ret, char *name) {
  if (ret != NULL) return;  // memory allocation suceeded
  fprintf(stderr, "Failed to allocate memory for %s.\n", name);
  exit(EXIT_FAILURE);
}

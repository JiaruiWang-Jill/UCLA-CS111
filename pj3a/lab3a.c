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
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
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

__u32 b_to_B(__u32 num_bits) { return num_bits / 8 + 1; }

unsigned long block_offset(__u32 block_index) {
  return _offset(BOOT_BLOCK_SIZE, block_index - 1, block_size);
}

void parse_time(time_t time, char *buf) {
  strftime(buf, TIME_STRING_SIZE, "%m/%d/%y %H:%M:%S", gmtime(&time));
}

void analyze_group(__u32 group_index);

void analyze_inode(__u32 inode_index, unsigned long inode_offset);

void analyze_direct_entry(__u32 inode_index, __u32 block_index);

void analyze_indirect_entry(__u32 inode_index, __u32 indir_block_ptr,
                            char filetype, short depth, short current_depth);

/**
 * usage
 *
 * prints out the usage of this function. Then exits with EXIT_FAILURE status.
 */
void usage();

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
         super_block.s_blocks_count,      // total number of blocks
         super_block.s_inodes_count,      // total number of inodes
         block_size,                      // block size
         super_block.s_inode_size,        // i-node size
         super_block.s_blocks_per_group,  // blocks per group
         super_block.s_inodes_per_group,  // inodes per group
         super_block.s_first_ino          // first non-reserved inode
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

void analyze_group(__u32 group_index) {
  group_descriptor_t group_descriptor;
  __u32 num_inodes_in_group, block_count_in_group;
  __u32 inode_bitmap_size = b_to_B(super_block.s_inodes_per_group);

  // Initialize the group descriptor for this group
  _c(pread(fs_fd, &group_descriptor, sizeof(group_descriptor),
           block_size * descriptor_block_index +
               group_index * sizeof(group_descriptor)),
     "Failed to read the group descriptor");

  // Calculate number of block in this group
  if (group_index == group_count - 1) {
    block_count_in_group = super_block.s_blocks_count -
                           super_block.s_blocks_per_group * (group_count - 1);
    num_inodes_in_group = super_block.s_inodes_count -
                          super_block.s_inodes_per_group * (group_count - 1);
  } else {
    block_count_in_group = super_block.s_blocks_per_group;
    num_inodes_in_group = super_block.s_inodes_per_group;
  }

  // Print group summary
  printf("GROUP,%d,%d,%d,%d,%d,%d,%d,%d\n",
         group_index,           // group number
         block_count_in_group,  // total number of blocks in this group
         num_inodes_in_group,   // total number of inodes
         group_descriptor.bg_free_blocks_count,  // number of free blocks
         group_descriptor.bg_free_inodes_count,  // number of free inodes
         // block number of free block bitmap for this group
         group_descriptor.bg_block_bitmap,
         // block number of free inode bitmap for this group
         group_descriptor.bg_inode_bitmap,
         // block number of first block of i-nodes in this group
         group_descriptor.bg_inode_table);

  char *block_bitmap;
  _m(block_bitmap = (char *)malloc(block_size), "block bitmap");
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

  char *inode_bitmap;
  _m(inode_bitmap = (char *)malloc(inode_bitmap_size), "inode bitmap");
  __u32 first_inode_index =
      _offset(1, group_index, super_block.s_inodes_per_group);
  __u32 inode_index = first_inode_index;
  // Read the inode bitmap for this group
  _c(pread(fs_fd, inode_bitmap, inode_bitmap_size,
           block_offset(group_descriptor.bg_inode_bitmap)),
     "Failed to read the inode bitmap");

  for (__u32 i = 0; i < inode_bitmap_size; i++) {
    char byte = inode_bitmap[i];
    unsigned long inode_offset;
    for (short j = 0; j < BIT_PER_BYTE; j++) {
      bool allocated = byte & 1;
      if (!allocated)
        printf("IFREE,%d\n", inode_index);
      else {
        inode_offset =
            _offset(block_offset(group_descriptor.bg_inode_table),
                    (inode_index - first_inode_index), sizeof(inode_t));
        analyze_inode(inode_index, inode_offset);
      }
      byte >>= 1;
      inode_index++;
    }
  }

  // Free allocated memory
  free(block_bitmap);
  free(inode_bitmap);
};

void analyze_inode(__u32 inode_index, unsigned long inode_offset) {
  inode_t inode;

  _c(pread(fs_fd, &inode, sizeof(inode), inode_offset), "Failed to read inode");

  // Check if inode is actually freed
  if (inode.i_mode == 0 || inode.i_links_count == 0) return;

  char filetype = '?';
  if (S_ISLNK(inode.i_mode)) filetype = 's';  // symlink
  if (S_ISREG(inode.i_mode)) filetype = 'f';  // regular file
  if (S_ISDIR(inode.i_mode)) filetype = 'd';  // directory

  __u32 block_count =
      2 * (inode.i_blocks / (2 << super_block.s_log_block_size));

  char ctime[TIME_STRING_SIZE];
  char mtime[TIME_STRING_SIZE];
  char atime[TIME_STRING_SIZE];
  parse_time(inode.i_ctime, ctime);  // creation time
  parse_time(inode.i_mtime, mtime);  // modification time
  parse_time(inode.i_atime, atime);  // access time
  // Print inode summary
  printf("INODE,%d,%c,%o,%d,%d,%d,%s,%s,%s,%d,%d",
         inode_index,            // inode number
         filetype,               // filetype
         inode.i_mode & 0x0FFF,  // mode, low order 12-bits
         inode.i_uid,            // owner
         inode.i_gid,            // group
         inode.i_links_count,    // link count
         ctime, mtime, atime,
         inode.i_size,  // file size
         block_count    // number of blocks
  );
  for (short i = 0; i < 15; i++) printf(",%d", inode.i_block[i]);
  printf("\n");

  if (filetype == 'd')
    // direct entries
    for (short i = 0; i < 12; i++)
      if (inode.i_block[i] != 0)
        analyze_direct_entry(inode_index, inode.i_block[i]);

  // Analyze indirect entry
  analyze_indirect_entry(inode_index, inode.i_block[12], filetype, 1, 1);
  // Analyze doubly indirect entry
  analyze_indirect_entry(inode_index, inode.i_block[13], filetype, 2, 1);
  // Analyze triply indirect entry
  analyze_indirect_entry(inode_index, inode.i_block[14], filetype, 3, 1);
}

void analyze_direct_entry(__u32 inode_index, __u32 block_index) {
  dir_entry_t dir_entry;
  for (__u32 logical_byte_offset = 0; logical_byte_offset < block_size;
       logical_byte_offset += dir_entry.rec_len) {
    _c(pread(fs_fd, &dir_entry, sizeof(dir_entry),
             block_offset(block_index) + logical_byte_offset),
       "Failed to read the directory entry");
    if (dir_entry.inode != 0) {
      dir_entry.name[dir_entry.name_len] = '\0';
      printf("DIRENT,%d,%d,%d,%d,%d,'%s'\n",
             inode_index,          // parent inode number
             logical_byte_offset,  // logical byte offset
             dir_entry.inode,      // inode number of the referenced file
             dir_entry.rec_len,    // entry length
             dir_entry.name_len,   // name length
             dir_entry.name        // name, string, surrounded by single-quotes
      );
    }
  }
}

void analyze_indirect_entry(__u32 inode_index, __u32 indir_block_ptr,
                            char filetype, short depth, short current_depth) {
  __u32 *block_ptrs;
  _m((block_ptrs = malloc(block_size)), "block pointers");
  _c(pread(fs_fd, block_ptrs, block_size, block_offset(indir_block_ptr)),
     "Failed to read the block pointers");
  // Calculate offset in this depth
  int depth_offset = 0;
  for (short i = 1; i < depth; i++) {
    int power = 1;
    for (short j = 0; j < i; j++) power *= 256;
    depth_offset += power;
  }
  __u32 num_pointers = block_size / sizeof(__u32);
  for (__u32 j = 0; j < num_pointers; j++) {
    if (block_ptrs[j] != 0) {
      if (depth == current_depth && filetype == 'd')
        analyze_direct_entry(inode_index, block_ptrs[j]);
      printf("INDIRECT,%d,%d,%d,%d,%d\n",
             inode_index,                // inode number
             depth - current_depth + 1,  // level of indirection
             depth_offset + 12 + j,      // logical block offset
             indir_block_ptr,  // block number of indirect block being scanned
             block_ptrs[j]     // block number of reference block
      );
      if (current_depth < depth)
        analyze_indirect_entry(inode_index, block_ptrs[j], filetype, depth,
                               current_depth + 1);
    }
  }
  free(block_ptrs);
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
  if (ret != -1) return;  // Syscall suceeded
  fprintf(stderr, "%s: %s. errno %d\n", errmsg, strerror(errno), errno);
  exit(EXIT_FAILURE);
}

void _m(void *ret, char *name) {
  if (ret != NULL) return;  // Memory allocation suceeded
  fprintf(stderr, "Failed to allocate memory for %s.\n", name);
  exit(EXIT_FAILURE);
}

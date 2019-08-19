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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "ext2_fs.h"
#include "lab3a.h"

#define PROGRAM_NAME "lab3a"
#define AUTHORS proper_name("Qingwei Zeng")

char const program_name[] = PROGRAM_NAME;

/* returns the offset for a block number */
unsigned long block_offset(unsigned int block) {
  return BOOT_BLOCK_SIZE + (block - 1) * block_size;
}

/* for each free block, print the number of the free block */
void free_blocks(int group, unsigned int block) {
  // 1 means 'used', 0 means 'free'
  char *bytes = (char *)malloc(block_size);
  unsigned long offset = block_offset(block);
  unsigned int curr =
      superblock.s_first_data_block + group * superblock.s_blocks_per_group;
  pread(fs_fd, bytes, block_size, offset);

  unsigned int i, j;
  for (i = 0; i < block_size; i++) {
    char x = bytes[i];
    for (j = 0; j < 8; j++) {
      int used = 1 & x;
      if (!used) {
        fprintf(stdout, "BFREE,%d\n", curr);
      }
      x >>= 1;
      curr++;
    }
  }
  free(bytes);
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
      fprintf(stdout, "DIRENT,%d,%d,%d,%d,%d,'%s'\n",
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
  pread(fs_fd, &inode, sizeof(inode), offset);

  if (inode.i_mode == 0 || inode.i_links_count == 0) {
    return;
  }

  char filetype = '?';
  // get bits that determine the file type
  uint16_t file_val = (inode.i_mode >> 12) << 12;
  if (file_val == 0xa000) {  // symbolic link
    filetype = 's';
  } else if (file_val == 0x8000) {  // regular file
    filetype = 'f';
  } else if (file_val == 0x4000) {  // directory
    filetype = 'd';
  }

  unsigned int num_blocks =
      2 * (inode.i_blocks / (2 << superblock.s_log_block_size));

  fprintf(stdout, "INODE,%d,%c,%o,%d,%d,%d,",
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
  fprintf(stdout, "%s,%s,%s,", ctime, mtime, atime);

  fprintf(stdout, "%d,%d",
          inode.i_size,  // file size
          num_blocks     // number of blocks
  );

  unsigned int i;
  for (i = 0; i < 15; i++) {  // block addresses
    fprintf(stdout, ",%d", inode.i_block[i]);
  }
  fprintf(stdout, "\n");

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
        fprintf(
            stdout, "INDIRECT,%d,%d,%d,%d,%d\n",
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
        fprintf(
            stdout, "INDIRECT,%d,%d,%d,%d,%d\n",
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
            fprintf(stdout, "INDIRECT,%d,%d,%d,%d,%d\n",
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
        fprintf(
            stdout, "INDIRECT,%d,%d,%d,%d,%d\n",
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
            fprintf(stdout, "INDIRECT,%d,%d,%d,%d,%d\n",
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
                fprintf(stdout, "INDIRECT,%d,%d,%d,%d,%d\n",
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

/* for each inode, print if free; if not, print inode summary */
void read_inode_bitmap(int group, int block, int inode_table_id) {
  int num_bytes = superblock.s_inodes_per_group / 8;
  char *bytes = (char *)malloc(num_bytes);

  unsigned long offset = block_offset(block);
  unsigned int curr = group * superblock.s_inodes_per_group + 1;
  unsigned int start = curr;
  pread(fs_fd, bytes, num_bytes, offset);

  int i, j;
  for (i = 0; i < num_bytes; i++) {
    char x = bytes[i];
    for (j = 0; j < 8; j++) {
      int used = 1 & x;
      if (used) {  // inode is allocated
        read_inode(inode_table_id, curr - start, curr);
      } else {  // free inode
        fprintf(stdout, "IFREE,%d\n", curr);
      }
      x >>= 1;
      curr++;
    }
  }
  free(bytes);
}

/* for each group, read bitmaps for blocks and inodes */
void read_group(int group, int total_groups) {
  struct ext2_group_desc group_desc;
  uint32_t descblock = 0;
  if (block_size == 1024) {
    descblock = 2;
  } else if (block_size > 1024) {
    descblock = 1;
  } else {
    fprintf(stderr,
            "Block size cannot be less than 1024, yet here it is. Something's "
            "wrong.\n");
    exit(EXIT_FAILURE);
  }

  unsigned long offset = block_size * descblock + 32 * group;
  pread(fs_fd, &group_desc, sizeof(group_desc), offset);

  unsigned int num_blocks_in_group = superblock.s_blocks_per_group;
  if (group == total_groups - 1) {
    num_blocks_in_group = superblock.s_blocks_count -
                          (superblock.s_blocks_per_group * (total_groups - 1));
  }

  unsigned int num_inodes_in_group = superblock.s_inodes_per_group;
  if (group == total_groups - 1) {
    num_inodes_in_group = superblock.s_inodes_count -
                          (superblock.s_inodes_per_group * (total_groups - 1));
  }

  // TODO: fix comments
  printf(
      GROUP_SUMMARY,
      group,                            // group number
      num_blocks_in_group,              // total number of blocks in this group
      num_inodes_in_group,              // total number of inodes
      group_desc.bg_free_blocks_count,  // number of free blocks
      group_desc.bg_free_inodes_count,  // number of free inodes
      group_desc
          .bg_block_bitmap,  // block number of free block bitmap for this group
      group_desc
          .bg_inode_bitmap,  // block number of free inode bitmap for this group
      group_desc.bg_inode_table  // block number of first block of i-nodes in
                                 // this group
  );

  unsigned int block_bitmap = group_desc.bg_block_bitmap;
  free_blocks(group, block_bitmap);

  unsigned int inode_bitmap = group_desc.bg_inode_bitmap;
  unsigned int inode_table = group_desc.bg_inode_table;
  read_inode_bitmap(group, inode_bitmap, inode_table);
}

/** usage
 * prints out the usage of this function. Then exits with EXIT_FAILURE status.
 */
void usage();

/** system call error checker
 * prints out the given error message and the error message related to the
 * errno. Then exits with EXIT_FAILURE status.
 */
void _c(int ret, char *errmsg);

int main(int argc, char *argv[]) {
  // Check the number of arguments; should be exactly 1
  if (argc != 2) {
    fprintf(stderr, "The program takes exactly one argument, %d were given.\n",
            argc - 1);
    usage();
  }
  // Open the file system image
  _c(fs_fd = open(argv[1], O_RDONLY),
     "Failed to open the given file system image file");
  // Intialize superblock structure
  _c(pread(fs_fd, &superblock, sizeof(superblock), BOOT_BLOCK_SIZE),
     "Failed to read the super block");
  block_size = EXT2_MIN_BLOCK_SIZE << superblock.s_log_block_size;
  // TODO: replace with print super block summary function
  // Print out superblock
  printf(SUPERBLOCK_SUMMARY,
         superblock.s_blocks_count,      // total number of blocks (decimal)
         superblock.s_inodes_count,      // total number of inodes (decimal)
         block_size,                     // block size (in bytes, decimal)
         superblock.s_inode_size,        // i-node size (in bytes, decimal)
         superblock.s_blocks_per_group,  // blocks per group (decimal)
         superblock.s_inodes_per_group,  // inodes per group (decimal)
         superblock.s_first_ino          // first non-reserved inode (decimal)
  );

  __u32 group_num =
      superblock.s_blocks_count % superblock.s_blocks_per_group == 0
          ? superblock.s_blocks_count / superblock.s_blocks_per_group
          : superblock.s_blocks_count / superblock.s_blocks_per_group + 1;

  for (__u32 i = 0; i < group_num; i++) read_group(i, group_num);

  return 0;
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


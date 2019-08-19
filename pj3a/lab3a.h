/*
 * lab3a.h
 * Copyright (C) 2019 Jim Zenn <zenn@ucla.edu>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef LAB3A_H
#define LAB3A_H

#include "ext2_fs.h"

/* macros */
#define BOOT_BLOCK_SIZE 1024

/* consts */

/** Superblock Summary
 * A single new-line terminated line, comprised of eight comma-separated fields
 * (with no white-space), summarizing the key file system parameters:
 *  - SUPERBLOCK
 *  - total number of blocks (decimal)
 *  - total number of i-nodes (decimal)
 *  - block size (in bytes, decimal)
 *  - i-node size (in bytes, decimal)
 *  - blocks per group (decimal)
 *  - i-nodes per group (decimal)
 *  - first non-reserved i-node (decimal)
 */
const char SUPERBLOCK_SUMMARY[] = "SUPERBLOCK,%d,%d,%d,%d,%d,%d,%d\n";

/** Group Summary
 * Scan each of the groups in the file system. For each group, produce a
 * new-line terminated line for each group, each comprised of nine comma-
 * separated fields (with no white space), summarizing its contents.
 *  - GROUP
 *  - group number (decimal, starting from zero)
 *  - total number of blocks in this group (decimal)
 *  - total number of i-nodes in this group (decimal)
 *  - number of free blocks (decimal)
 *  - number of free i-nodes (decimal)
 *  - block number of free block bitmap for this group (decimal)
 *  - block number of free i-node bitmap for this group (decimal)
 *  - block number of first block of i-nodes in this group (decimal)
 */
const char GROUP_SUMMARY[] = "GROUP,%d,%d,%d,%d,%d,%d,%d,%d\n";

/* typedefs */
typedef struct ext2_super_block superblock_t;

/* global variables */
int fs_fd;
__u32 block_size;
superblock_t superblock;

#endif /* !LAB3A_H */

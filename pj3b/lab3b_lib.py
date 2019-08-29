from dataclasses import dataclass
from typing import List


@dataclass
class Superblock:
    """superblock summary"""
    block_num: int  # total number of blocks
    inode_num: int  # total number of i-nodes
    block_size: int  # block size
    inode_size: int  # inode size
    blocks_per_group: int  # blocks per group
    inodes_per_group: int  # inodes per group
    first_non_reserved_inode: int  # first non-reserved i-node

    @classmethod
    def from_entries(cls, args):
        return Superblock(int(args[1]), int(args[2]), int(args[3]),
                          int(args[4]), int(args[5]), int(args[6]),
                          int(args[7]))


@dataclass
class Group:
    """group summary"""
    group_index: int  # group number
    block_num: int  # total number of blocks in this group
    inode_num: int  # total number of inodes in this group
    free_block_num: int  # number of free blocks
    free_inode_num: int  # number of free inodes
    block_bitmap_index: int  # block number of free block bitmap
    inode_bitmap_index: int  # block number of free i-node bitmap
    first_inode_index: int  # block number of first block of i-nodes

    @classmethod
    def from_entries(cls, args):
        return Group(int(args[1]), int(args[2]), int(args[3]), int(args[4]),
                     int(args[5]), int(args[6]), int(args[7]), int(args[8]))


@dataclass
class Inode:
    """inode summary"""
    inode_index: int  # inode number
    file_type: str  # 'f': file, 'd': directory, 's': symlink,'?' others
    mode: int  # mode (low order 12-bits, octal ... suggested format "%o")
    uid: int  # owner id
    gid: int  # group id
    links_count: int  # links count
    creation_time: str  # time of last I-node change (mm/dd/yy hh:mm:ss, GMT)
    modification_time: str  # modification time (mm/dd/yy hh:mm:ss, GMT)
    access_time: str  # time of last access (mm/dd/yy hh:mm:ss, GMT)
    file_size: int  # file size
    block_num: int  # number of blocks of disk space taken up by this file
    block_addresses: List[int]

    @classmethod
    def from_entries(cls, args):
        block_addresses = args[12:27]
        return Inode(int(args[1]), args[2], int(args[3]), int(args[4]),
                     int(args[5]), int(args[6]), args[7], args[8], args[9],
                     int(args[10]), int(args[11]), block_addresses)


@dataclass
class Dirent:
    """Directory entry summary"""
    parent_inode_index: int  # the I-node number of the parent directory
    logical_byte_offset: int  # logical byte offset of this entry
    inode_index: int  # inode number of the referenced file
    entry_length: int  # entry length
    name_length: int  # name length
    name: str  # name

    @classmethod
    def from_entries(cls, args):
        return Dirent(int(args[1]), int(args[2]), int(args[3]), int(args[4]),
                      int(args[5]), args[6])


@dataclass
class Indirect:
    """Indirect block reference summary"""
    inode_index: int  # I-node number of the owning file (decimal)
    level: int  # 1: single indirect, 2: double, 3: triple
    logical_block_offset: int
    block_index: int  # block number
    ref_block_index: int  # reference block number

    @classmethod
    def from_entries(cls, args):
        return Indirect(int(args[1]), int(args[2]), int(args[3]), int(args[4]),
                        int(args[5]))


@dataclass
class Block:
    """Block summary"""
    inode_index: int
    offset: int
    level: int

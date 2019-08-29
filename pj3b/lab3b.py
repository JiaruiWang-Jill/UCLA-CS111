#!/usr/local/cs/bin/python3


# NAME: Qingwei Zeng
# EMAIL: zenn@ucla.edu
# ID: 005181694

import sys
from typing import Dict

from lab3b_lib import *


class Ext2_summary:
    superblock: Superblock
    block_dict: Dict[int, List] = {}
    inodes: Dict[int, Inode] = {}
    dirents: Dict[int, Dirent] = {}
    inode_link_count = {}
    inode_ref = {}

    ifree = set()
    bfree = set()
    inode_dict_parents = {}

    reserved_blocks = {0, 1, 2, 3, 4, 5, 6, 7, 64}
    total_blocks: int
    total_inodes: int
    inconsistent = False

    @property
    def total_blocks(self):
        return self.superblock.block_num

    @property
    def total_inodes(self):
        return self.superblock.inode_num

    def parse_superblock(self, entries):
        self.superblock = Superblock.from_entries(entries)

    def parse_bfree(self, entries):
        self.bfree.add(int(entries[1]))

    def parse_ifree(self, entries):
        self.ifree.add(int(entries[1]))

    def parse_inode(self, entries):
        inode = Inode.from_entries(entries)
        self.inodes[inode.inode_index] = inode
        self.analyze_inode(inode)

    def parse_indirect(self, entries):
        indirect = Indirect.from_entries(entries)
        self.analyze_indirect(indirect)

    def parse_dirent(self, entries):
        dirent = Dirent.from_entries(entries)
        self.dirents[dirent.inode_index] = dirent
        self.analyze_dirent(dirent)

    def parse_records(self, records):
        for i in records:
            entries = i.split(",")
            record_header = entries[0]
            if record_header == 'SUPERBLOCK':
                self.parse_superblock(entries)
            elif record_header == 'BFREE':
                self.parse_bfree(entries)
            elif record_header == 'IFREE':
                self.parse_ifree(entries)
            elif record_header == 'INODE':
                self.parse_inode(entries)
            elif record_header == 'INDIRECT':
                self.parse_indirect(entries)
            elif record_header == 'DIRENT':
                self.parse_dirent(entries)

    @classmethod
    def level_string(cls, level: int):
        if level == 1:
            return "INDIRECT"
        if level == 2:
            return "DOUBLE INDIRECT"
        if level == 3:
            return "TRIPLE INDIRECT"
        return None

    @classmethod
    def level_offset(cls, level: int):
        if level == 1:
            return 12
        if level == 2:
            return 268
        if level == 3:
            return 65804
        return 0

    def analyze_inode(self, inode: Inode):
        # put in inode dict (link count) {inode number:link count}
        for i in range(12, 27):  # block addresses
            level = i - 23 if i > 23 else 0

            block_index = int(inode.block_addresses[i - 12])
            if block_index == 0:  # unused block address
                continue

            if not 0 <= block_index <= self.total_blocks:  # check validity
                message = filter(None,
                                 ['INVALID', self.level_string(level), 'BLOCK',
                                  str(block_index), 'IN INODE',
                                  str(inode.inode_index), 'AT OFFSET',
                                  str(self.level_offset(level))])
                print(' '.join(message))
                self.inconsistent = True
                continue

            if block_index in self.reserved_blocks and block_index != 0:
                message = filter(None,
                                 ['RESERVED', self.level_string(level), 'BLOCK',
                                  str(block_index), 'IN INODE',
                                  str(inode.inode_index), 'AT OFFSET',
                                  str(self.level_offset(level))])
                print(' '.join(message))
                self.inconsistent = True
                continue

            if block_index not in self.block_dict:
                self.block_dict[block_index] = list()
            self.block_dict[block_index].append(
                [inode.inode_index, level])

    def analyze_indirect(self, indirect: Indirect):
        if not 0 <= indirect.ref_block_index <= self.total_blocks:
            message = filter(None,
                             ['INVALID', self.level_string(indirect.level),
                              'BLOCK', str(indirect.ref_block_index),
                              'IN INODE', str(indirect.inode_index),
                              'AT OFFSET',
                              str(self.level_offset(indirect.level))])
            print(' '.join(message))
            self.inconsistent = True
        elif indirect.ref_block_index in self.reserved_blocks:
            message = filter(None,
                             ['RESERVED', self.level_string(indirect.level),
                              'BLOCK', str(indirect.ref_block_index),
                              'IN INODE', str(indirect.inode_index),
                              'AT OFFSET',
                              str(self.level_offset(indirect.level))])
            print(' '.join(message))
            self.inconsistent = True
        else:
            if indirect.ref_block_index not in self.block_dict:
                self.block_dict[indirect.ref_block_index] = list()
            self.block_dict[indirect.ref_block_index].append(
                [indirect.inode_index, indirect.level])

    def analyze_dirent(self, dirent: Dirent):
        if dirent.inode_index not in self.inode_link_count:
            self.inode_link_count[dirent.inode_index] = 0
        self.inode_link_count[dirent.inode_index] += 1

        if not 1 <= dirent.inode_index <= self.total_inodes:
            message = filter(None,
                             ['DIRECTORY INODE', str(dirent.parent_inode_index),
                              'NAME', dirent.name[0:-2] + "'", "INVALID INODE",
                              str(dirent.inode_index)])
            print(' '.join(message))
            self.inconsistent = True

        if dirent.name.startswith("'.'"):
            if dirent.parent_inode_index != dirent.inode_index:
                message = filter(None,
                                 ['DIRECTORY INODE',
                                  str(dirent.parent_inode_index),
                                  "NAME '.' LINK TO INODE",
                                  str(dirent.inode_index), 'SHOULD BE',
                                  str(dirent.parent_inode_index)])
                print(' '.join(message))
                self.inconsistent = True

        elif dirent.name.startswith("'..'"):
            self.inode_dict_parents[
                dirent.parent_inode_index] = dirent.inode_index
        else:
            self.inode_ref[dirent.inode_index] = dirent.parent_inode_index

    def main(self):
        # check the arguments
        argc = len(sys.argv)
        if argc != 2:
            sys.stderr.write(
                'This program accepts exactly one argument. {} given.\n'
                    .format(argc))
            exit(1)

        # open the file from the command line argument
        try:
            records = open(sys.argv[1], "r").readlines()
        except:
            sys.stderr.write('Failed to open file\n')
            exit(1)

        self.parse_records(records)

        for block_index in range(1, self.total_blocks + 1):
            if block_index not in self.bfree and \
                    block_index not in self.block_dict and \
                    block_index not in self.reserved_blocks:
                print('UNREFERENCED BLOCK ' + str(block_index))
                self.inconsistent = True
            if block_index in self.bfree and block_index in self.block_dict:
                print('ALLOCATED BLOCK ' + str(block_index) + ' ON FREELIST')
                self.inconsistent = True

        for inode_index in range(1, self.total_inodes + 1):
            if inode_index not in self.ifree and \
                    inode_index not in self.inodes and \
                    inode_index not in self.inode_ref and \
                    inode_index not in range(1, 11):
                print('UNALLOCATED INODE ' + str(
                    inode_index) + ' NOT ON FREELIST')
                self.inconsistent = True
            if inode_index in self.inodes and inode_index in self.ifree:
                print('ALLOCATED INODE ' + str(inode_index) + ' ON FREELIST')
                self.inconsistent = True

        for block in self.block_dict:
            if len(self.block_dict[block]) > 1:
                self.inconsistent = True
                for ref in self.block_dict[block]:
                    message = filter(None, ['DUPLICATE', self.level_string(
                        ref[1]), 'BLOCK', str(block), 'IN INODE',
                                            str(ref[0]), 'AT OFFSET',
                                            str(self.level_offset(ref[1]))])
                    print(' '.join(message))

        for parent in self.inode_dict_parents:
            if parent == 2:
                if self.inode_dict_parents[parent] != 2:
                    message = filter(None, [
                        'DIRECTORY INODE 2 NAME \'..\' LINK TO INODE',
                        str(self.inode_dict_parents[parent]),
                        'SHOULD BE 2'])
                    print(' '.join(message))
                    self.inconsistent = True
            elif parent not in self.inode_ref:
                print("DIRECTORY INODE " + str(
                    self.inode_dict_parents[
                        parent]) + " NAME '..' LINK TO INODE " + str(
                    parent) + " SHOULD BE " + str(
                    self.inode_dict_parents[parent]))
                self.inconsistent = True
            elif self.inode_dict_parents[parent] != self.inode_ref[parent]:
                print("DIRECTORY INODE " + str(
                    parent) + " NAME '..' LINK TO INODE " + str(
                    parent) + " SHOULD BE " + str(self.inode_ref[parent]))
                self.inconsistent = True

        for inode_index in self.inodes:
            links = self.inode_link_count[inode_index] if inode_index in self.inode_link_count else 0
            if links != self.inodes[inode_index].links_count:
                print('INODE ' + str(inode_index) + ' HAS ' + str(links) +
                      ' LINKS BUT LINKCOUNT IS ' + str(self.inodes[inode_index].links_count))
                self.inconsistent = True

        for dirent in self.dirents.values():
            if dirent.inode_index in self.ifree and \
                    dirent.inode_index in self.inode_ref:
                parent_inode = self.inode_ref[dirent.inode_index]
                dir_name = dirent.name
                print('DIRECTORY INODE ' + str(parent_inode) + ' NAME ' +
                      dir_name[0:-2] + "' UNALLOCATED INODE " + str(
                    dirent.inode_index))
                self.inconsistent = True

        exit(2 if self.inconsistent else 0)


mayhem = Ext2_summary()
mayhem.main()

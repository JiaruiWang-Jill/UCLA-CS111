#! /bin/sh
#
# tests.sh
# Copyright (C) 2019 Jim Zenn <zenn@ucla.edu>
#
# Distributed under terms of the MIT license.
#

LIST="./lab2_list"


# lab2b-1.png
for t in 1 2 4 8 12 16 24; do
  $LIST --threads=$t --iterations=1000 --sync=m
  $LIST --threads=$t --iterations=1000 --sync=s
done




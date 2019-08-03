#! /bin/sh
#
# tests.sh
# Copyright (C) 2019 Jim Zenn <zenn@ucla.edu>
#
# Distributed under terms of the MIT license.
#

LIST="./lab2_list"


# lab2_list-1.png
for i in 10 100 1000 10000 20000; do
  $LIST --threads=1 --iterations=$i
done

# lab2_list-2.png
for t in 2 4 8 12; do
  for i in 1 10 100 1000; do
    $LIST --threads=$t --iterations=$i
  done
done
for t in 2 4 8 12; do
  for i in 1 2 4 8 16 32; do
    for y in i d il dl; do
      $LIST --threads=$t --iterations=$i --yield=$y
    done
  done
done

# lab2_list-3.png
for y in i d il dl; do
  for s in s m; do
    $LIST --threads=12 --iterations=32 --yield=$y --sync=$s
  done
done

# lab2_list-4.png
for t in 1 2 4 8 12 16 24; do
  for s in s m; do
    $LIST --threads=$t --iterations=1000 --sync=$s
  done
done


#! /bin/sh
#
# tests.sh
# Copyright (C) 2019 Jim Zenn <zenn@ucla.edu>
#
# Distributed under terms of the MIT license.
#

LIST="./lab2_list"


#	lab2b_1.png
# throughput vs. number of threads for mutex and spin-lock synchronized list
# operations.
#	lab2b_2.png
# mean time per mutex wait and mean time per operation for mutex-synchronized
# list operations.
for t in 1 2 4 8 12 16 24; do
  $LIST --threads=$t --iterations=1000 --sync=m
  $LIST --threads=$t --iterations=1000 --sync=s
done

# lab2b_3.png
# successful iterations vs. number of threads for each synchronization method.
for t in 1 4 8 12 16; do
  for i in 1 2 4 8 16; do
    $LIST --list=4 --threads=$t --iterations=$i --yield=id
  done
done
for t in 1 4 8 12 16; do
  for i in 10 20 40 80; do
    for s in s m; do
      $LIST --list=4 --threads=$t --iterations=$i --sync=$s --yield=id
    done
  done
done

# lab2b_4.png
# throughput vs. number of threads for mutex synchronized partitioned lists.
for t in 1 2 4 8 12; do
  for l in 1 4 8 16; do
    $LIST --list=$l --threads=$t --sync=m --iterations=1000
  done
done

# lab2b_5.png
# throughput vs. number of threads for spin-lock-synchronized partitioned lists.
for t in 1 2 4 8 12; do
  for l in 1 4 8 16; do
    $LIST --list=$l --threads=$t --sync=s --iterations=1000
  done
done



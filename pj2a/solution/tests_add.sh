#! /bin/sh
#
# tests.sh
# Copyright (C) 2019 Jim Zenn <zenn@ucla.edu>
#
# Distributed under terms of the MIT license.
#

ADD="./lab2_add"

for t in 2 4 8 12; do
  for i in 100 1000 10000 100000; do
    $ADD --threads=$t --iterations=$i
  done
done

# lab2_add-1.png
#   threads and iterations required to generate a failure (with and without
#   yields)
for t in 2 4 8 12; do
  for i in 10 20 40 80 100 1000 10000; do
    $ADD --threads=$t --iterations=$i
    $ADD --threads=$t --iterations=$i --yield
  done
done

# lab2_add-2.png
#   average time per operation with and without yields.
for t in 2 8; do
  for i in 100 1000 10000 100000; do
    $ADD --threads=$t --iterations=$i
    $ADD --threads=$t --iterations=$i --yield
  done
done

# lab2_add-3.png
#   average time per (single threaded) operation vs. the number of iterations
for i in 100 1000 10000 100000; do
  $ADD --threads=1 --iterations=$i
done

# lab2_add-4.png
#   threads and iterations that can run successfully with yields under each
#   of the synchronization options.
for t in 2 4 8 12; do
    $ADD --threads=$t --iterations=10000 --yield --sync=m
    $ADD --threads=$t --iterations=10000 --yield --sync=c
    $ADD --threads=$t --iterations=1000 --yield --sync=s
done

# lab2_add-5.png
# average time per (protected) operation vs. the number of threads.
for t in 1 2 4 8 12; do
    $ADD --threads=$t --iterations=10000
    $ADD --threads=$t --iterations=10000 --sync=m
    $ADD --threads=$t --iterations=10000 --sync=c
    $ADD --threads=$t --iterations=10000 --sync=s
done


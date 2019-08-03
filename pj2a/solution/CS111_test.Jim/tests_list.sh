#! /bin/sh
#
# tests.sh
# Copyright (C) 2019 Jim Zenn <zenn@ucla.edu>
#
# Distributed under terms of the MIT license.
#

LIST="./lab2_list"


$LIST --threads=1  --iterations=10
$LIST --threads=1  --iterations=100
$LIST --threads=1  --iterations=1000
$LIST --threads=1  --iterations=10000
$LIST --threads=1  --iterations=20000

$LIST --threads=2  --iterations=1
$LIST --threads=2  --iterations=10
$LIST --threads=2  --iterations=100
$LIST --threads=2  --iterations=1000
$LIST --threads=4  --iterations=1
$LIST --threads=4  --iterations=10
$LIST --threads=4  --iterations=100
$LIST --threads=4  --iterations=1000
$LIST --threads=8  --iterations=1
$LIST --threads=8  --iterations=10
$LIST --threads=8  --iterations=100
$LIST --threads=8  --iterations=1000
$LIST --threads=12 --iterations=1
$LIST --threads=12 --iterations=10
$LIST --threads=12 --iterations=100
$LIST --threads=12 --iterations=1000

$LIST --threads=2  --iterations=1   --yield=i
$LIST --threads=2  --iterations=2   --yield=i
$LIST --threads=2  --iterations=4   --yield=i
$LIST --threads=2  --iterations=8   --yield=i
$LIST --threads=2  --iterations=16  --yield=i
$LIST --threads=2  --iterations=32  --yield=i
$LIST --threads=4  --iterations=1   --yield=i
$LIST --threads=4  --iterations=2   --yield=i
$LIST --threads=4  --iterations=4   --yield=i
$LIST --threads=4  --iterations=8   --yield=i
$LIST --threads=4  --iterations=16  --yield=i
$LIST --threads=8  --iterations=1   --yield=i
$LIST --threads=8  --iterations=2   --yield=i
$LIST --threads=8  --iterations=4   --yield=i
$LIST --threads=8  --iterations=8   --yield=i
$LIST --threads=8  --iterations=16  --yield=i
$LIST --threads=12 --iterations=1   --yield=i
$LIST --threads=12 --iterations=2   --yield=i
$LIST --threads=12 --iterations=4   --yield=i
$LIST --threads=12 --iterations=8   --yield=i
$LIST --threads=12 --iterations=16  --yield=i

$LIST --threads=2  --iterations=1   --yield=d
$LIST --threads=2  --iterations=2   --yield=d
$LIST --threads=2  --iterations=4   --yield=d
$LIST --threads=2  --iterations=8   --yield=d
$LIST --threads=2  --iterations=16  --yield=d
$LIST --threads=2  --iterations=32  --yield=d
$LIST --threads=4  --iterations=1   --yield=d
$LIST --threads=4  --iterations=2   --yield=d
$LIST --threads=4  --iterations=4   --yield=d
$LIST --threads=4  --iterations=8   --yield=d
$LIST --threads=4  --iterations=16  --yield=d
$LIST --threads=8  --iterations=1   --yield=d
$LIST --threads=8  --iterations=2   --yield=d
$LIST --threads=8  --iterations=4   --yield=d
$LIST --threads=8  --iterations=8   --yield=d
$LIST --threads=8  --iterations=16  --yield=d
$LIST --threads=12 --iterations=1   --yield=d
$LIST --threads=12 --iterations=2   --yield=d
$LIST --threads=12 --iterations=4   --yield=d
$LIST --threads=12 --iterations=8   --yield=d
$LIST --threads=12 --iterations=16  --yield=d

$LIST --threads=2  --iterations=1   --yield=il
$LIST --threads=2  --iterations=2   --yield=il
$LIST --threads=2  --iterations=4   --yield=il
$LIST --threads=2  --iterations=8   --yield=il
$LIST --threads=2  --iterations=16  --yield=il
$LIST --threads=2  --iterations=32  --yield=il
$LIST --threads=4  --iterations=1   --yield=il
$LIST --threads=4  --iterations=2   --yield=il
$LIST --threads=4  --iterations=4   --yield=il
$LIST --threads=4  --iterations=8   --yield=il
$LIST --threads=4  --iterations=16  --yield=il
$LIST --threads=8  --iterations=1   --yield=il
$LIST --threads=8  --iterations=2   --yield=il
$LIST --threads=8  --iterations=4   --yield=il
$LIST --threads=8  --iterations=8   --yield=il
$LIST --threads=8  --iterations=16  --yield=il
$LIST --threads=12 --iterations=1   --yield=il
$LIST --threads=12 --iterations=2   --yield=il
$LIST --threads=12 --iterations=4   --yield=il
$LIST --threads=12 --iterations=8   --yield=il
$LIST --threads=12 --iterations=16  --yield=il

$LIST --threads=2  --iterations=1   --yield=dl
$LIST --threads=2  --iterations=2   --yield=dl
$LIST --threads=2  --iterations=4   --yield=dl
$LIST --threads=2  --iterations=8   --yield=dl
$LIST --threads=2  --iterations=16  --yield=dl
$LIST --threads=2  --iterations=32  --yield=dl
$LIST --threads=4  --iterations=1   --yield=dl
$LIST --threads=4  --iterations=2   --yield=dl
$LIST --threads=4  --iterations=4   --yield=dl
$LIST --threads=4  --iterations=8   --yield=dl
$LIST --threads=4  --iterations=16  --yield=dl
$LIST --threads=8  --iterations=1   --yield=dl
$LIST --threads=8  --iterations=2   --yield=dl
$LIST --threads=8  --iterations=4   --yield=dl
$LIST --threads=8  --iterations=8   --yield=dl
$LIST --threads=8  --iterations=16  --yield=dl
$LIST --threads=12 --iterations=1   --yield=dl
$LIST --threads=12 --iterations=2   --yield=dl
$LIST --threads=12 --iterations=4   --yield=dl
$LIST --threads=12 --iterations=8   --yield=dl
$LIST --threads=12 --iterations=16  --yield=dl

$LIST --threads=12 --iterations=32 --yield=i  --sync=m
$LIST --threads=12 --iterations=32 --yield=d  --sync=m
$LIST --threads=12 --iterations=32 --yield=il --sync=m
$LIST --threads=12 --iterations=32 --yield=dl --sync=m
$LIST --threads=12 --iterations=32 --yield=i  --sync=s
$LIST --threads=12 --iterations=32 --yield=d  --sync=s
$LIST --threads=12 --iterations=32 --yield=il --sync=s
$LIST --threads=12 --iterations=32 --yield=dl --sync=s

$LIST --threads=1  --iterations=1000
$LIST --threads=1  --iterations=1000 --sync=m
$LIST --threads=2  --iterations=1000 --sync=m
$LIST --threads=4  --iterations=1000 --sync=m
$LIST --threads=8  --iterations=1000 --sync=m
$LIST --threads=12 --iterations=1000 --sync=m
$LIST --threads=16 --iterations=1000 --sync=m
$LIST --threads=24 --iterations=1000 --sync=m
$LIST --threads=1  --iterations=1000 --sync=s
$LIST --threads=2  --iterations=1000 --sync=s
$LIST --threads=4  --iterations=1000 --sync=s
$LIST --threads=8  --iterations=1000 --sync=s
$LIST --threads=12 --iterations=1000 --sync=s
$LIST --threads=16 --iterations=1000 --sync=s
$LIST --threads=24 --iterations=1000 --sync=s

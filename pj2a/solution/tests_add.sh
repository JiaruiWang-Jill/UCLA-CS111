#! /bin/sh
#
# tests.sh
# Copyright (C) 2019 Jim Zenn <zenn@ucla.edu>
#
# Distributed under terms of the MIT license.
#

ADD="./lab2_add"

$ADD --threads=2  --iterations=100
$ADD --threads=2  --iterations=1000
$ADD --threads=2  --iterations=10000
$ADD --threads=2  --iterations=100000
$ADD --threads=4  --iterations=100
$ADD --threads=4  --iterations=1000
$ADD --threads=4  --iterations=10000
$ADD --threads=4  --iterations=100000
$ADD --threads=8  --iterations=100
$ADD --threads=8  --iterations=1000
$ADD --threads=8  --iterations=10000
$ADD --threads=8  --iterations=100000
$ADD --threads=12 --iterations=100
$ADD --threads=12 --iterations=1000
$ADD --threads=12 --iterations=10000
$ADD --threads=12 --iterations=100000

$ADD --threads=2  --iterations=10     --yield
$ADD --threads=4  --iterations=10     --yield
$ADD --threads=8  --iterations=10     --yield
$ADD --threads=12 --iterations=10     --yield
$ADD --threads=2  --iterations=20     --yield
$ADD --threads=4  --iterations=20     --yield
$ADD --threads=8  --iterations=20     --yield
$ADD --threads=12 --iterations=20     --yield
$ADD --threads=2  --iterations=40     --yield
$ADD --threads=4  --iterations=40     --yield
$ADD --threads=8  --iterations=40     --yield
$ADD --threads=12 --iterations=40     --yield
$ADD --threads=2  --iterations=80     --yield
$ADD --threads=4  --iterations=80     --yield
$ADD --threads=8  --iterations=80     --yield
$ADD --threads=12 --iterations=80     --yield
$ADD --threads=2  --iterations=100    --yield
$ADD --threads=4  --iterations=100    --yield
$ADD --threads=8  --iterations=100    --yield
$ADD --threads=12 --iterations=100    --yield
$ADD --threads=2  --iterations=1000   --yield
$ADD --threads=4  --iterations=1000   --yield
$ADD --threads=8  --iterations=1000   --yield
$ADD --threads=12 --iterations=1000   --yield
$ADD --threads=2  --iterations=10000  --yield
$ADD --threads=4  --iterations=10000  --yield
$ADD --threads=8  --iterations=10000  --yield
$ADD --threads=12 --iterations=10000  --yield
$ADD --threads=2  --iterations=100000 --yield
$ADD --threads=4  --iterations=100000 --yield
$ADD --threads=8  --iterations=100000 --yield
$ADD --threads=12 --iterations=100000 --yield

$ADD --threads=1  --iterations=100
$ADD --threads=1  --iterations=1000
$ADD --threads=1  --iterations=10000
$ADD --threads=1  --iterations=100000
$ADD --threads=1  --iterations=1000000
$ADD --threads=2  --iterations=100
$ADD --threads=2  --iterations=1000
$ADD --threads=2  --iterations=10000
$ADD --threads=2  --iterations=100000
$ADD --threads=2  --iterations=1000000
$ADD --threads=4  --iterations=100
$ADD --threads=4  --iterations=1000
$ADD --threads=4  --iterations=10000
$ADD --threads=4  --iterations=100000
$ADD --threads=4  --iterations=1000000
$ADD --threads=1  --iterations=100     --yield
$ADD --threads=1  --iterations=1000    --yield
$ADD --threads=1  --iterations=10000   --yield
$ADD --threads=1  --iterations=100000  --yield
$ADD --threads=1  --iterations=1000000 --yield
$ADD --threads=2  --iterations=100     --yield
$ADD --threads=2  --iterations=1000    --yield
$ADD --threads=2  --iterations=10000   --yield
$ADD --threads=2  --iterations=100000  --yield
$ADD --threads=2  --iterations=1000000 --yield
$ADD --threads=4  --iterations=100     --yield
$ADD --threads=4  --iterations=1000    --yield
$ADD --threads=4  --iterations=10000   --yield
$ADD --threads=4  --iterations=100000  --yield
$ADD --threads=4  --iterations=1000000 --yield

$ADD --threads=2  --iterations=10000 --yield --sync=m
$ADD --threads=4  --iterations=10000 --yield --sync=m
$ADD --threads=8  --iterations=10000 --yield --sync=m
$ADD --threads=12 --iterations=10000 --yield --sync=m
$ADD --threads=2  --iterations=10000 --yield --sync=c
$ADD --threads=4  --iterations=10000 --yield --sync=c
$ADD --threads=8  --iterations=10000 --yield --sync=c
$ADD --threads=12 --iterations=10000 --yield --sync=c
$ADD --threads=2  --iterations=10000 --yield --sync=s
$ADD --threads=4  --iterations=10000 --yield --sync=s
$ADD --threads=8  --iterations=1000  --yield --sync=s
$ADD --threads=12 --iterations=1000  --yield --sync=s

$ADD --threads=1  --iterations=10000 --sync=m
$ADD --threads=2  --iterations=10000 --sync=m
$ADD --threads=4  --iterations=10000 --sync=m
$ADD --threads=8  --iterations=10000 --sync=m
$ADD --threads=12 --iterations=10000 --sync=m
$ADD --threads=1  --iterations=10000 --sync=c
$ADD --threads=2  --iterations=10000 --sync=c
$ADD --threads=4  --iterations=10000 --sync=c
$ADD --threads=8  --iterations=10000 --sync=c
$ADD --threads=12 --iterations=10000 --sync=c
$ADD --threads=1  --iterations=10000 --sync=s
$ADD --threads=2  --iterations=10000 --sync=s
$ADD --threads=4  --iterations=10000 --sync=s
$ADD --threads=8  --iterations=10000 --sync=s
$ADD --threads=12 --iterations=10000 --sync=s

#! /usr/bin/gnuplot
#
# purpose:
#	 generate data reduction graphs for the multi-threaded list project
#
# input: lab2_list.csv
#	1. test name
#	2. threads
#	3. iterations per thread
#	4. number of lists
#	5. operations performed (threads x iterations x (ins + lookup + delete))
#	6. run time (ns)
#	7. run time per operation (ns)
#	8. lock acquisition time per operation (ns)
#
# output:
#	lab2b_1.png ... throughput vs. number of threads for mutex and spin-lock
#	                synchronized list operations.
#	lab2b_2.png ... mean time per mutex wait and mean time per operation for
#	                mutex-synchronized list operations.
# lab2b_3.png ... successful iterations vs. number of threads for each
#                 synchronization method.
# lab2b_4.png ... throughput vs. number of threads for mutex synchronized
#                 partitioned lists.
# lab2b_5.png ... throughput vs. number of threads for spin-lock-synchronized
#                 partitioned lists.
#

# general plot parameters
set terminal png
set datafile separator ","

# lab2b_1.png
set title "Throughput vs # of Threads"
set xlabel "# of threads"
set logscale x 2
set ylabel "Throughput (op/s)"
set logscale y 10
set output 'lab2b_1.png'
plot \
  "< grep 'list-none-s,[0-9]*,1000,1,' lab2b_list.csv" using \
  ($2):(1000000000/($7)) \
  title 'sync=s' with linespoints lc rgb 'green', \
  "< grep 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using \
  ($2):(1000000000/($7)) \
  title 'sync=m' with linespoints lc rgb 'red'

# lab2b_2.png
set title "Per-op Mutex Cost and pPer-op Runtime vs # of Threads"
set xlabel "# of Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "time/Operation(ns/op)"
set logscale y 10
set output 'lab2b_2.png'
plot \
  "< grep 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):($7) \
  title 'per-op runtime' with linespoints lc rgb 'green', \
  "< grep 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):($8) \
  title 'per-op mutex cost' with linespoints lc rgb 'red'

# lab2b_3.png
set title "Successful Iterations vs. # of Threads"
set logscale x 2
set xrange [0.75:]
set xlabel "Threads"
set ylabel "Successful Iterations"
set logscale y 10
set output 'lab2b_3.png'
plot \
  "< grep 'list-id-none,[0-9]*,[0-9]*,4,' lab2b_list.csv" using ($2):($3) \
  title 'sync=none' with points lc rgb 'green', \
  "< grep 'list-id-s,[0-9]*,[0-9]*,4,' lab2b_list.csv" using ($2):($3) \
  title 'sync=s' with points lc rgb 'red', \
  "< grep 'list-id-m,[0-9]*,[0-9]*,4,' lab2b_list.csv" using ($2):($3) \
  title 'sync=m' with points lc rgb 'blue'

# lab2b_4.png
set title "Scalability-4: Throughput of Mutex Synchronized Partitioned Lists"
set xlabel "# of Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Throughput (operations/sec)"
set logscale y 10
set output 'lab2b_4.png'
plot \
  "< grep 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using \
  ($2):(1000000000/($7)) \
  title 'lists=1' with linespoints lc rgb 'blue', \
  "< grep 'list-none-m,[0-9]*,1000,4,' lab2b_list.csv" using \
  ($2):(1000000000/($7)) \
  title 'lists=4' with linespoints lc rgb 'green', \
  "< grep 'list-none-m,[0-9]*,1000,8,' lab2b_list.csv" using \
  ($2):(1000000000/($7)) \
  title 'lists=8' with linespoints lc rgb 'red', \
  "< grep 'list-none-m,[0-9]*,1000,16,' lab2b_list.csv" using \
  ($2):(1000000000/($7)) \
  title 'lists=16' with linespoints lc rgb 'yellow'

# lab2b_5.png
set title "Throughput vs # of threads"
set xlabel "# of Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Throughput (op/sec)"
set logscale y 10
set output 'lab2b_5.png'
plot \
  "< grep 'list-none-s,[0-9]*,1000,1,' lab2b_list.csv" using \
  ($2):(1000000000/($7)) \
  title 'lists=1' with linespoints lc rgb 'blue', \
  "< grep 'list-none-s,[0-9]*,1000,4,' lab2b_list.csv" using \
  ($2):(1000000000/($7)) \
  title 'lists=4' with linespoints lc rgb 'yellow', \
  "< grep 'list-none-s,[0-9]*,1000,8,' lab2b_list.csv" using \
  ($2):(1000000000/($7)) \
  title 'lists=8' with linespoints lc rgb 'red', \
  "< grep 'list-none-s,[0-9]*,1000,16,' lab2b_list.csv" using \
  ($2):(1000000000/($7)) \
  title 'lists=16' with linespoints lc rgb 'green'

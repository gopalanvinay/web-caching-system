set datafile separator ','
set ylabel "Req/Sec"
set xlabel 'Threads'
set y2tics
set ytics nomirror
set y2label "Latency"
set style line 100 lt 1 lc rgb "grey" lw 0.5
set grid ls 100
set ytics 0.5
set xtics 1
set style line 101 lw 3 lt rgb "#f62aa0"
set style line 102 lw 3 lt rgb "#26dfd0"

set xtics rotate # rotate labels on the x axis
set key right center # legend placement

plot 'thread_latencies.csv' using 1:2 with lines ls 101 title "Latency", '' using 1:3 with lines ls 102 title "Req/Sec"

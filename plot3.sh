#!/usr/bin/gnuplot
#
# Plot the utility function from the results of the main experiment
#
# http://blog.thinkoriginally.com/2011/06/14/gnuplot-box-and-whiskers-plot/

name=system("echo $name")
set terminal pngcairo font "arial,10" size 500,500
set output name.'.png'

set autoscale
set xrange[0:1]
set yrange[0:1] # may need to manually set this

# Data columns: X Min 1stQuartile Median 3rdQuartile Max Titles
set bars 4.0
set style fill empty
set datafile separator ','
set xlabel "Utility of delay"
set ylabel "Utility of PDR"
plot name using 3:4

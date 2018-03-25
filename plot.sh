#!/usr/bin/gnuplot
#
# Plot main experiment data in a box and whisker plot
#
# http://blog.thinkoriginally.com/2011/06/14/gnuplot-box-and-whiskers-plot/

name=system("echo $name")
ymax=system("echo $ymax")
units=system("echo $units")
set terminal pngcairo font "arial,10" size 500,500
set output name.'.png'

set autoscale
set xrange[0:1]
set yrange[0:ymax] # may need to manually set this

# Data columns: X Min 1stQuartile Median 3rdQuartile Max Titles
set bars 4.0
set style fill empty
set datafile separator ','
set xlabel "Probability"
set ylabel name.' ('.units.')'
plot name.'.csv' using 1:3:2:6:5 with candlesticks title "Quartiles" whiskerbars, \
  ''         using 1:4:4:4:4 with candlesticks lt -1 notitle

# Gnuplot style used for examples
#
# Based on
# http://guidolan.blogspot.com/2010/03/how-to-create-beautiful-gnuplot-graphs.html
#

set xtics nomirror

set macro

# Palette URL: http://colorschemedesigner.com/#3K40zsOsOK-K-
red_000 = "#F9B7B0"
red_025 = "#F97A6D"
red_050 = "#E62B17"
red_075 = "#8F463F"
red_100 = "#6D0D03"

blue_000 = "#A9BDE6"
blue_025 = "#7297E6"
blue_050 = "#1D4599"
blue_075 = "#2F3F60"
blue_100 = "#031A49"

green_000 = "#A6EBB5"
green_025 = "#67EB84"
green_050 = "#11AD34"
green_075 = "#2F6C3D"
green_100 = "#025214"

brown_000 = "#F9E0B0"
brown_025 = "#F9C96D"
brown_050 = "#E69F17"
brown_075 = "#8F743F"
brown_100 = "#6D4903"

line_width = "2"
axis_width = "1.5"
ps = "1.2"

#set term wxt enhanced font my_font
set pointsize ps
set style line 1 linecolor rgbcolor blue_025 linewidth @line_width pt 7
set style line 2 linecolor rgbcolor green_025 linewidth @line_width pt 5
set style line 3 linecolor rgbcolor red_025 linewidth @line_width pt 9
set style line 4 linecolor rgbcolor brown_025 linewidth @line_width pt 13
set style line 5 linecolor rgbcolor blue_050 linewidth @line_width pt 11
set style line 6 linecolor rgbcolor green_050 linewidth @line_width pt 7
set style line 7 linecolor rgbcolor red_050 linewidth @line_width pt 5
set style line 8 linecolor rgbcolor brown_050 linewidth @line_width pt 9
set style line 9 linecolor rgbcolor blue_075 linewidth @line_width pt 13
set style line 10 linecolor rgbcolor green_075 linewidth @line_width pt 11
set style line 11 linecolor rgbcolor red_075 linewidth @line_width pt 7
set style line 12 linecolor rgbcolor brown_075 linewidth @line_width pt 5
set style line 13 linecolor rgbcolor blue_100 linewidth @line_width pt 9
set style line 14 linecolor rgbcolor green_100 linewidth @line_width pt 13
set style line 15 linecolor rgbcolor red_100 linewidth @line_width pt 11
set style line 16 linecolor rgbcolor brown_100 linewidth @line_width pt 7
set style line 17 linecolor rgbcolor "#224499" linewidth @line_width pt 5

set style increment user

#set key outside box width 2 height 2 enhanced spacing 2

# Line style for grid
set style line 81 lt 3  # dashed
set style line 81 lt rgb "#808080" lw 0.5  # grey
set grid back linestyle 81
unset grid

set nomacro

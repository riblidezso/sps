#hisztogram kozepen kene legyen, de akkor kene minimum, maximum stb
#

reset

cmargin=4
lrmargin=8
topmargin=1
botmargin=4

set terminal pngcairo enhanced color dashed font 'Arial,12' lw 1 size 30cm,30cm 
set output 'diag_multi.png'

set multiplot

#TOP LEFT
set size 0.5,1.0/3.0 
set origin 0,0.66

set lmargin lrmargin
set rmargin cmargin
set tmargin topmargin
set bmargin botmargin

set xlabel "{/Symbol l} [A]"
set ylabel "Intenzitas"
p "../fit.txt" u 1:($2>0.001?($2):1/0) w l title "mert spektrum", "../fit.txt" u 1:($2>0.001?($4):1/0) w l title "illesztett spektrum"

#TOP RIGHT
set size 0.5,1.0/3.0
set origin 0.5,0.66

set lmargin cmargin
set rmargin lrmargin

set xlabel "number of iterations"
set ylabel ""

p "../acc-rate.dat" w l title "acceptance rate"

#CENTER LEFT
set size 0.5,1.0/3.0
set origin 0,1.0/3.0

set lmargin lrmargin
set rmargin cmargin

set ylabel "log(P)"
p [200:] "../chi_evol.txt" w l title "log(P) evolution"


#CENTER RIGHT 
set size 0.5,1.0/3.0
set origin 0.5,1.0/3.0

set lmargin cmargin
set rmargin lrmargin

set xlabel "number of iterations"
set ylabel ""

p "../worse-acc-rate.dat" w l title "acceptance rate of worse points"

#BOTTOM LEFT

set size 0.5,1.0/3.0
set origin 0,0

set xlabel "number of iterations"
set ylabel ""

p "../worse-rate.dat" w l title "rate of worse points proposed"

unset multiplot
unset output

#BOTTOM RIGHT

set size 0.5,1.0/3.0
set origin 0.5,0

unset ytics
set y2tics

set lmargin cmargin
set rmargin lrmargin

unset multiplot
unset output



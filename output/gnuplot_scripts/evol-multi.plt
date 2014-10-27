#hisztogram kozepen kene legyen, de akkor kene minimum, maximum stb
#

reset

cmargin=4
lrmargin=8
topmargin=1
botmargin=4

set terminal pngcairo enhanced color dashed font 'Arial,12' lw 1 size 30cm,30cm 
set output 'evol-multi.png'

set multiplot

set xlabel "no. of iterations"

#TOP LEFT
set size 0.5,1.0/3.0 
set origin 0,0.66

set lmargin lrmargin
set rmargin cmargin
set tmargin topmargin
set bmargin botmargin

set ylabel "Z"
p "../metall.dat" u 1 w l title "metallicity evolution"


#TOP RIGHT
set size 0.5,1.0/3.0
set origin 0.5,0.66

set lmargin cmargin
set rmargin lrmargin

set ylabel "{/Symbol m}"
p "../dust_mu.dat" u 1 w l   title "{/Symbol m} evolution"

#CENTER LEFT
set size 0.5,1.0/3.0
set origin 0,1.0/3.0

set lmargin lrmargin
set rmargin cmargin

set ylabel "{/Symbol t}_v"
p "../dust_tau_v.dat" u 1 w l  title "{/Symbol t}_v evolution" 

#CENTER RIGHT 
set size 0.5,1.0/3.0
set origin 0.5,1.0/3.0

set lmargin cmargin
set rmargin lrmargin

set ylabel "age [Gyrs]"
p "../age.dat" u ($1)/(10**9) w l  title "age evolution"

#BOTTOM LEFT

set size 0.5,1.0/3.0
set origin 0,0

set lmargin lrmargin
set rmargin cmargin

set ylabel "{/Symbol t} [Gyrs]"
p "../sfr_tau.dat" u ($1)/(10**9) w l title "{/Symbol t} evolution"

#BOTTOM RIGHT

#set size 0.5,1.0/3.0
#set origin 0.5,0

#set lmargin cmargin
#set rmargin lrmargin

#set ylabel "{/Symbol m} x {/Symbol t}_v"
#p "../points.dat" u ($2*$1) w l title "{/Symbol m} x {/Symbol t}_v evolution"


unset multiplot
unset output



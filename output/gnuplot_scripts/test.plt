#hisztogram kozepen kene legyen, de akkor kene minimum, maximum stb
#

reset

if (!strstrt(GPVAL_COMPILE_OPTIONS,"+STATS")) {
    print "No support for stats command"
} 
else {

nbins=20

stats '../points.dat' using 1 name "tauv" nooutput
stats '../points.dat' using 2 name "mu" nooutput
stats '../points.dat' using 3 name "tau" nooutput
stats '../points.dat' using 4 name "age" nooutput
stats '../points.dat' using 5 name "metall" nooutput

cmargin=2
lrmargin=8
topmargin=1
botmargin=4

set terminal pngcairo enhanced color dashed font 'Arial,12' lw 2 size 40cm,40cm 
set output 'test.png'

set multiplot

#TOP LEFT
set size 0.5,1.0/3.0 
set origin 0,0.66

set xtics in mirror
set ytics in mirror

set ylabel "P"

set lmargin lrmargin
set rmargin cmargin
set tmargin topmargin
set bmargin botmargin

set xlabel "age [Gyr]"

binwidth=(age_max-age_min)/(nbins*1e9)

bin(x,width)=width*floor(x/width)

p [] [0:0.25] "../points.dat" u (bin(($4/1e9),binwidth)+binwidth*0.5):(1.0/age_records) smooth freq with boxes notitle 
#title "age distribution"

#TOP RIGHT
set size 0.5,1.0/3.0
set origin 0.5,0.66

unset ytics 
set y2tics mirror

unset ylabel

set lmargin cmargin
set rmargin lrmargin

set xlabel "{/Symbol m} "

binwidth=(mu_max-mu_min)/nbins

p [] [0:0.25] "../points.dat" u (bin($2,binwidth)):(1.0/mu_records) smooth freq with boxes notitle 
#title "{/Symbol m} distribution"


#CENTER LEFT
set size 0.5,1.0/3.0
set origin 0,1.0/3.0

set ylabel "P"

unset y2tics
set ytics

set xlabel "1000  Z"

set lmargin lrmargin
set rmargin cmargin

binwidth=(metall_max-metall_min)*1000/(nbins)

p [] [0:0.25] "../points.dat" u (bin(($5*1000),binwidth)):(1.0/metall_records) smooth freq with boxes notitle 
#title "metallicity distribution"

#CENTER RIGHT 
set size 0.5,1.0/3.0
set origin 0.5,1.0/3.0

unset ytics
set y2tics mirror

unset ylabel

set lmargin cmargin
set rmargin lrmargin

binwidth=(tauv_max-tauv_min)/nbins

set xlabel "{/Symbol t}_v"

p [] [0:0.25] "../points.dat" u (bin($1,binwidth)):(1.0/tauv_records) smooth freq with boxes notitle 
#title "{/Symbol t}_v distribution"

#BOTTOM LEFT

set size 0.5,1.0/3.0
set origin 0,0

set ylabel "P"

unset y2tics
set ytics

set lmargin lrmargin
set rmargin cmargin

binwidth=(tau_max-tau_min)/(nbins*1e9)

set xlabel "{/Symbol t} [Gyr]"

p [] [0:0.25] "../points.dat" u (bin(($3/1e9),binwidth)):(1.0/tau_records) smooth freq with boxes notitle 
#title "age distribution"

#BOTTOM RIGHT

set size 0.5,1.0/3.0
set origin 0.5,0

unset ytics
set y2tics

set lmargin cmargin
set rmargin lrmargin

binwidth=0.004
#(mu_max*tauv_max-mu_min*tauv_min)/nbins
#ez igy gyenge

set xlabel "{/Symbol t}_v {/Symbol m} "

p [] [0:0.25] "../points.dat" u (bin(($1*$2),binwidth)):(1.0/tau_records) smooth freq with boxes notitle 
#title "age distribution"


unset multiplot
unset output }

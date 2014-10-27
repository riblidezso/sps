#plotting histograms

reset

if (!strstrt(GPVAL_COMPILE_OPTIONS,"+STATS")) {
    print "No support for stats command"
} 
else {

nbins=20

stats '../dust_tau_v.dat' using 1 name "tauv" nooutput
stats '../dust_mu.dat' using 1 name "mu" nooutput
stats '../sfr_tau.dat' using 1 name "tau" nooutput
stats '../age.dat' using 1 name "age" nooutput
stats '../metall.dat' using 1 name "metall" nooutput
stats '../vdisp.dat' using 1 name "vdisp" nooutput

cmargin=2
lrmargin=8
topmargin=1
botmargin=4

set terminal pngcairo enhanced color dashed font 'Arial,12' lw 2 size 40cm,40cm 
set output 'hist.png'

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

binwidth=(age_max-age_min+1)/(nbins*1e9)

bin(x,width)=width*floor(x/width)

p [] [0:0.25] "../age.dat" u (bin(($1/1e9),binwidth)+binwidth*0.5):(1.0/age_records) smooth freq with boxes notitle 
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

binwidth=(mu_max-mu_min+0.05)/nbins

p [] [0:0.25] "../dust_mu.dat" u (bin($1,binwidth)):(1.0/mu_records) smooth freq with boxes notitle 
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

binwidth=(metall_max-metall_min+0.0001)*1000/(nbins)

p [] [0:0.25] "../metall.dat" u (bin(($1*1000),binwidth)):(1.0/metall_records) smooth freq with boxes notitle 
#title "metallicity distribution"

#CENTER RIGHT 
set size 0.5,1.0/3.0
set origin 0.5,1.0/3.0

unset ytics
set y2tics mirror

unset ylabel

set lmargin cmargin
set rmargin lrmargin

binwidth=(tauv_max-tauv_min+0.1)/nbins

set xlabel "{/Symbol t}_v"

p [] [0:0.25] "../dust_tau_v.dat" u (bin($1,binwidth)):(1.0/tauv_records) smooth freq with boxes notitle 
#title "{/Symbol t}_v distribution"

#BOTTOM LEFT

set size 0.5,1.0/3.0
set origin 0,0

set ylabel "P"

unset y2tics
set ytics

set lmargin lrmargin
set rmargin cmargin

binwidth=(tau_max-tau_min+1)/(nbins*1e9)

set xlabel "{/Symbol t} [Gyr]"

p [] [0:0.25] "../sfr_tau.dat" u (bin(($1/1e9),binwidth)):(1.0/tau_records) smooth freq with boxes notitle 
#title "age distribution"

#BOTTOM RIGHT

set size 0.5,1.0/3.0
set origin 0.5,0

unset ytics
set y2tics

set lmargin cmargin
set rmargin lrmargin

binwidth=(vdisp_max-vdisp_min+0.0001)/(nbins)
set xlabel "{/Symbol s}_v "

p [] [0:0.25] "../vdisp.dat" u (bin(($1),binwidth)):(1.0/tau_records) smooth freq with boxes notitle 
#title "age distribution"


unset multiplot
unset output 

reset

set terminal pngcairo enhanced color dashed font 'Arial,12' lw 2 size 32cm,18cm 
set output 'vdisp_hist.png'

set size 1,1
set origin 0,0

set lmargin lrmargin
set rmargin lrmargin
set tmargin topmargin
set bmargin botmargin

set xtics in mirror
set ytics in mirror

set ylabel "P"

set xlabel "vdisp [km/s]"

binwidth=300000*(vdisp_max-vdisp_min+0.00001)/(nbins)

p [] [0:0.25] "../points.dat" u (bin(($6*300000),binwidth)+binwidth*0.5):(1.0/vdisp_records) smooth freq with boxes notitle 
#title "vdisp distribution"

unset output

}




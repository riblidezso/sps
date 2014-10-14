
reset

if (!strstrt(GPVAL_COMPILE_OPTIONS,"+STATS")) {
    print "No support for stats command"
} else {

#stats '../fit.txt' using 4 name "fitpoints" nooutput
stats '../fit.txt' using 1 name "lampoints" nooutput

set terminal pngcairo enhanced color dashed font 'Arial,12' lw 1 size 50cm,30cm 
set output 'fit.png'

set xlabel "{/Symbol l} [A]"
set ylabel "Intensity"
#p [lampoints_min-100:lampoints_max+100][fitpoints_min*0.9:fitpoints_max*1.1] "../fit.txt" u 1:($2>0.001?($2):1/0) w l lw 2 title "observed spectrum", "../fit.txt" u 1:($2>0.001?($4):1/0) w l lw 2 title "fitted spectrum", "../fit.txt" u 1:($2>0.001?($4-$2):1/0) w l lw 2 title "fitted spectrum"

p [lampoints_min-100:lampoints_max+100][] "../fit.txt" u 1:($2>0.001?($2):1/0) w l lw 2 title "observed spectrum", "../fit.txt" u 1:($2>0.001?($4):1/0) w l lw 2 title "fitted spectrum", "../fit.txt" u 1:($2>0.001?($4-$2):1/0) w l lw 2 title "difference"


unset output
}


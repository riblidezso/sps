set xlabel "age [Gyrs]"
set ylabel "Z"
set auto
set term pngcairo enhanced  
set output "scatter-age-metall.png"
p "../points.dat" u ($4/(10**9)):5 w dots   title "age-metallicity scatter"
set output
set term wxt

set xlabel "{/Symbol t}_v"
set ylabel "{/Symbol m}"
set auto
set term pngcairo enhanced  
set output "scatter-dtau-dmu.png"
p "../points.dat" u 1:2 w dots   title "{/Symbol t}_v - {/Symbol m} scatter"
set output
set term wxt

set xlabel "{/Symbol t}_v"
set ylabel "Z"
set auto
set term pngcairo enhanced  
set output "scatter-metall-dtau.png"
p "../points.dat" u 1:5 w dots   title "{/Symbol t}_v - metallicity scatter"
set output
set term wxt

set xlabel "Z"
set ylabel "{/Symbol m}"
set auto
set term pngcairo enhanced  
set output "scatter-metall-dmu.png"
p "../points.dat" u 5:2 w dots   title "Z - {/Symbol m} scatter"
set output
set term wxt

set xlabel "age"
set ylabel "{/Symbol m}"
set auto
set term pngcairo enhanced  
set output "scatter-age-dmu.png"
p "../points.dat" u ($4/(10**9)):2 w dots   title "age - {/Symbol m} scatter"
set output
set term wxt

set xlabel "age"
set ylabel "{/Symbol t}_v"
set auto
set term pngcairo enhanced  
set output "scatter-age-dtau.png"
p "../points.dat" u ($4/10**9):1 w dots   title "age - {/Symbol t}_v scatter"
set output


set term wxt
set xlabel "age"
set ylabel "{/Symbol t}"
set auto
set term pngcairo enhanced  
set output "scatter-age-tau.png"
p "../points.dat" u ($4/10**9):($3/10**9) w dots   lw 0.1 title "age - {/Symbol t} scatter"
set output

set term wxt
set xlabel "metall"
set ylabel "{/Symbol t}"
set auto
set term pngcairo enhanced  
set output "scatter-metall-tau.png"
p "../points.dat" u 5:($3/10**9) w dots   title "metall - {/Symbol t} scatter"
set output

set term wxt
set xlabel "{/Symbol t}_v"
set ylabel "{/Symbol t}"
set auto
set term pngcairo enhanced  
set output "scatter-dtau-tau.png"
p "../points.dat" u 1:($3/10**9) w dots   title "{/Symbol t}_v - {/Symbol t} scatter"
set output

set term wxt
set xlabel "{/Symbol m}"
set ylabel "{/Symbol t}"
set auto
set term pngcairo enhanced  
set output "scatter-dmu-tau.png"
p "../points.dat" u 2:($3/10**9) w dots   title "{/Symbol m} - {/Symbol t} scatter"
set output

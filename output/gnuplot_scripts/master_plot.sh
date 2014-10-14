#!/bin/bash

rm -r plots
mkdir plots
cd plots

echo plotting fit...
gnuplot ../gnuplot_scripts/fit.plt
echo plotting marginalized distributions...
gnuplot ../gnuplot_scripts/test.plt
echo plotting 2D scatterplots...
gnuplot ../gnuplot_scripts/scatterpng.plt
echo plotting parameter evolution plot...
gnuplot ../gnuplot_scripts/evol-multi.plt
echo plotting diagnostics plot...
gnuplot ../gnuplot_scripts/diag_multi.plt


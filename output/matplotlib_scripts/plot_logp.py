#!/usr/bin/python

import numpy
import matplotlib.pyplot as plt

#reading files
params=dict()
params["log_p"]=numpy.loadtxt("chi_evol.dat")

fig=plt.figure()
ax=fig.add_subplot(1,1,1)

########################################
#plot parameter evolutions
########################################
for param in params:
	ax.plot(params[param])

	ax.set_xlabel("N")
	ax.set_ylabel(param)

	ax.set_xscale("log")
	ax.set_yscale("log")

	fig.savefig("plots-python/evol_"+param+".png",dpi=100)
	ax.cla()


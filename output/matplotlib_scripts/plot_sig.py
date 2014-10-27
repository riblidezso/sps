#!/usr/bin/python

#import modules pandas might not be necessary
#it is just for really fast reading (more than 10x faster than numpy)

import numpy
import matplotlib.pyplot as plt
import pandas

#reading files
params=dict()
params["sigmas_evol_dust_tau_v"]=numpy.array(pandas.read_csv("sigmas_evol_dust_tau_v.dat"))
params["sigmas_evol_dust_mu"]=numpy.array(pandas.read_csv("sigmas_evol_dust_mu.dat"))
params["sigmas_evol_sfr_tau"]=numpy.array(pandas.read_csv("sigmas_evol_sfr_tau.dat"))
params["sigmas_evol_age"]=numpy.array(pandas.read_csv("sigmas_evol_age.dat"))
params["sigmas_evol_metall"]=numpy.array(pandas.read_csv("sigmas_evol_metall.dat"))
params["sigmas_evol_vdisp"]=numpy.array(pandas.read_csv("sigmas_evol_vdisp.dat"))

fig=plt.figure()
ax=fig.add_subplot(1,1,1)

########################################
#plot parameter evolutions
########################################
for param in params:
	ax.plot(params[param])
	ax.set_xlabel("N")
	ax.set_ylabel(param)
	fig.savefig("plots-python/evol_"+param+".png",dpi=100)
	ax.cla()


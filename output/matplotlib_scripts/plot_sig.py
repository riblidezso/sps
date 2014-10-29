#!/usr/bin/python


import numpy
import matplotlib.pyplot as plt

#reading files
params=dict()
params["sigmas_evol_dust_tau_v"]=numpy.loadtxt("sigmas_evol_dust_tau_v.dat")
params["sigmas_evol_dust_mu"]=numpy.loadtxt("sigmas_evol_dust_mu.dat")
params["sigmas_evol_sfr_tau"]=numpy.loadtxt("sigmas_evol_sfr_tau.dat")
params["sigmas_evol_age"]=numpy.loadtxt("sigmas_evol_age.dat")
params["sigmas_evol_metall"]=numpy.loadtxt("sigmas_evol_metall.dat")
params["sigmas_evol_vdisp"]=numpy.loadtxt("sigmas_evol_vdisp.dat")

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


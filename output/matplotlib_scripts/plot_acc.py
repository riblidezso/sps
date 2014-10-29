#!/usr/bin/python


import numpy
import matplotlib.pyplot as plt

#reading files
params=dict()
params["acc_ratios_dust_tau_v"]=numpy.loadtxt("acc_ratios_dust_tau_v.dat")
params["acc_ratios_dust_mu"]=numpy.loadtxt("acc_ratios_dust_mu.dat")
params["acc_ratios_sfr_tau"]=numpy.loadtxt("acc_ratios_sfr_tau.dat")
params["acc_ratios_age"]=numpy.loadtxt("acc_ratios_age.dat")
params["acc_ratios_metall"]=numpy.loadtxt("acc_ratios_metall.dat")
params["acc_ratios_vdisp"]=numpy.loadtxt("acc_ratios_vdisp.dat")

fig=plt.figure()
ax=fig.add_subplot(1,1,1)

########################################
#plot parameter evolutions
########################################
for param in params:
	ax.step(range(len(params[param])),params[param])
	ax.set_xlabel("N")
	ax.set_ylabel(param)
	fig.savefig("plots-python/evol_"+param+".png",dpi=100)
	ax.cla()


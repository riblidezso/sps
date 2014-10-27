#!/usr/bin/python

#import modules pandas might not be necessary
#it is just for really fast reading (more than 10x faster than numpy)

import numpy
import matplotlib.pyplot as plt
import pandas

#reading files
params=dict()
params["acc_ratios_dust_tau_v"]=numpy.array(pandas.read_csv("acc_ratios_dust_tau_v.dat"))
params["acc_ratios_dust_mu"]=numpy.array(pandas.read_csv("acc_ratios_dust_mu.dat"))
params["acc_ratios_sfr_tau"]=numpy.array(pandas.read_csv("acc_ratios_sfr_tau.dat"))
params["acc_ratios_age"]=numpy.array(pandas.read_csv("acc_ratios_age.dat"))
params["acc_ratios_metall"]=numpy.array(pandas.read_csv("acc_ratios_metall.dat"))
params["acc_ratios_vdisp"]=numpy.array(pandas.read_csv("acc_ratios_vdisp.dat"))

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


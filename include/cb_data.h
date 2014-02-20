#ifndef CB_DATA_H
#define CB_DATA_H

#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <ctime>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits>

#include"table.h"

class cb_data{

public: //functions

	//Reading model from an ASCII file
	int read_buff_atof();
	//gets some important data from model
	int get_data_from_model();
	//gets dust parameters and modifies data
	int dust();
	//gets the ages of galaxies	
	int get_age();
	//gets sfr parameters
	int get_sfr();
	//writes the result in a table format
	int write_convresult();

	//The c++ version of the convolution:
	int conv_to_age_vector();
	//The openCL convolution
	int opencl_convolve();

private: //data

	//stores all numbers of the data ascii file
	std::vector<double> rawdata;
	//stores the ages where model data is available
	std::vector<double> time;
	//stores the wavelengths of the model
	std::vector<double> wavelengths;

	//no. of timesteps
	int ntimesteps;
	//no. of spectrum points
	int nspecsteps;
	//the beginning of spectrum data in rawdata
	int offset;
	//no. of not used data between spectra
	int nfillingdata;

	//dust parameters:
	double dust_tau_v;
	double dust_mu;

	//sfr parameter:
	double tau;

	//stores the ages of the galaxy
	std::vector<double> ages;
	int n_ages;

	//stores the results of the convolution
	table conv_result;

private: //functions

	//modifies rawdata, due to dust
	void dust_modif();

	//returns star formation rate
	double sfr( double, double);

};

#endif

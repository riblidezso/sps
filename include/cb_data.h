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

/*quick tests*/
	int quick_test();
	int quick_test_openCL();

/*interactive functions*/
	//Reading model from an ASCII file
	int usr_read_buff_atof();
	//gets dust parameters and modifies data
	int usr_get_dust();
	//gets the ages of galaxies	
	int usr_get_age();
	//gets sfr parameters
	int usr_get_sfr();
	//writes the result in a table format
	int usr_write_convresult();

/*functions*/

	//Reading model from an ASCII file 
	int read_buff_atof(std::string infilename);
	//writes the result in a table format
	int write_convresult(std::string outfilename);

	//sets dst parameters
	void set_dust(double tau_v_value,double mu_value);
	//sets age
	void set_age(std::vector<double> age_values);
	void set_sfr(double tau_value);

/*	//gets dust parameters and modifies data
	int dust();
	//gets the ages of galaxies	
	int age();
	//gets sfr parameters
	int get_sfr();
	//writes the result in a table format
	int write_convresult(std::string output_filename);

*/

/*routines*/
	
	//The c++ version of the convolution:
	int conv_to_age_vector();
	//The openCL convolution
	int opencl_convolve();

private: //functions

	//gets some important data from model
	//called by read_buff_atof()
	int get_data_from_model();



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

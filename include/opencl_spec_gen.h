#ifndef OPENCL_SPEC_GEN_H
#define OPENCL_SPEC_GEN_H

#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <ctime>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits>


#include "read.h"

class opencl_spec_gen{

public: //functions

	//constructor

//	spec_gen(read input);
	
	void cpy(read& input);

/*interactive functions*/

	//gets dust parameters and modifies data
	int usr_get_dust();
	//gets the ages of galaxies	
	int usr_get_age();
	//gets sfr parameters
	int usr_get_sfr();
	//gets imf
	int usr_get_imf();
	//gets metallicity
	int usr_get_metall();
	//writes the result 
	int usr_write_result();


	//interpolates models
	int metall_interpol(std::vector<std::vector<double> >& original_models);

	//if no metall interpol
	//then just choosing one of the models
	int chose_model(std::vector<std::vector<double> >& original_models, int no_model);
		
	//modifies rawdata, due to dust
	void dust_modif();



/*functions*/

	//writes the result in a table format
	int write_result(std::string outfilename);

	void set_dust(double tau_v_value,double mu_value);
	void set_age(double age_value);
	void set_sfr(double tau_value);


/*convolution routines*/

	//conv to only one age
	int conv_to_age();
	int opencl_conv_to_age(std::vector<double>&  model_cont);


public: //data

	//stores all numbers of the data ascii file
	std::vector<double> model;
	//stores the ages where model data is available
	std::vector<double> time;
	//stores the wavelengths of the model
	std::vector<double> wavelengths;

	//no. of timesteps
	int ntimesteps;
	//no. of spectrum points
	int nspecsteps;

	//dust parameters:
	double dust_tau_v;
	double dust_mu;

	int dust_option;
	//sfr parameter:
	double sfr_tau;
	//age
	double age;
	double metall;
	std::string imf;

	//result 
	std::vector<double> conv_to_age_res;
	

public: //functions

	//returns star formation rate
	double sfr( double, double);
	
	//writes a vector to a file
	int write_vector(std::vector<double>& vec_to_write, std::string outfilename);

	//writes a table (vector of vectors) to a file
	//(vector is a row or column)
	int write_table_row(std::vector< std::vector<double > >& table_to_write, std::string outfilename);
	int write_table_col(std::vector< std::vector<double > >& table_to_write, std::string outfilename);

};

#endif
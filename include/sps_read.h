#ifndef SPS_READ_H
#define SPS_READ_H

#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>

class sps_read{

public: //functions

	//reads in all models
	int read_model_bin_all_cont();

	//reads in all models 
	int read_model_bin_all();
	//Reading 1 from binary file 
	int read_model_bin(int i,std::string infilename);

	//reads timesteps
	int read_time_bin();
	//reads timesteps_cont
	int read_model_bin_cont(std::string infilename);

	//reads wavelengths
	int read_wavel_bin();

	//reads sample_spec
	//from ascii file
	int usr_read_sample();

	//reads measured sdss spec
	//from csv file
	int usr_read_sdss_csv();

public: //data

	//stores the ages where model data is available
	std::vector<double> time;
	//stores the wavelengths of the model
	std::vector<double> wavelengths;

	//the models, one vector is one model
	//and one model contains 221 spectra contiugiously
	std::vector<std::vector<double> > models;

	std::vector<double> model_cont;

	//sample spectrum container
	std::vector<double> sample_spec;

	//sdss_spec containers
	std::vector<double> mes_spec_wavel;
	std::vector<double> mes_spec;
	std::vector<double> mes_spec_err_l;
	std::vector<double> mes_spec_err_h;
	std::vector<double> mes_spec_mask;
};

#endif

#ifndef READ_H
#define READ_H

#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>

class read{

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
	int read::usr_read_sample();

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

};

#endif

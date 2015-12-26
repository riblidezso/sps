#ifndef SPS_DATA_H
#define SPS_DATA_H

#ifdef __APPLE__
#include "OpenCL/opencl.h"
#else
#include "CL/cl.h"
#endif

#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>

class sps_data{

//functions
public:
    
    //read the binary sps model data
    int read_binary_sps_model();
    
    //read sdss measurement if filename != none
    // if yes read from user input
    int read_measurement(std::string input_fname);
    
    //models are resampled to the resolution os the measurement
    int resample_models_2_mes(std::string imf);
    
    
private:
    
    //read sdss measurement data from file
    int read_sdss_measurement(std::string infilename );
    
    //read the sdss measurement from csv file with user input
    int usr_read_sdss_csv();

	
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

	
//data
public:

	//stores the ages where model data is available
	std::vector<double> time;
	//stores the wavelengths of the model
	std::vector<double> wavelengths;

	//the models, one vector is one model
	//and one model contains 221 spectra contiugiously
	std::vector<std::vector<double> > models;

    //contigous models
	std::vector<double> model_cont;
    //contigous models resampled to measurement wavelength
    std::vector<cl_float> resampled_model_cont;

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

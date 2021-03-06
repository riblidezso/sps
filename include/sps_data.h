#ifndef SPS_DATA_H
#define SPS_DATA_H

//number of models with different metallicity 
#define NO_METALL_MODELS 6

#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>

class sps_data{

public:

    sps_data(std::string measurement_fname,std::string imf);
    
//functions
private:
    
    //read the binary sps model data
    int read_binary_sps_model(std::string imf);
    
    //reads in all models
    int read_model_bin_all_cont(std::string imf);
    
    //reads timesteps_cont
    int read_model_bin_cont(std::string infilename);
    
    //reads timesteps
    int read_time_bin();
    
    //reads wavelengths
    int read_wavel_bin();
    
    
    //read sdss measurement data from file
    int read_measurement(std::string infilename );
    
    //models are resampled to the resolution os the measurement
    int resample_models_2_mes();

	
//data
public:
    std::string spsfast_path;

	//stores the ages where model data is available
	std::vector<double> time;
	//stores the wavelengths of the model
	std::vector<double> wavelengths;

    //contigous models
	std::vector<double> model_cont;
    
    //contigous models resampled to measurement wavelength
    std::vector<double> resampled_model_cont;

	//sdss_spec containers
	std::vector<double> mes_spec_wavel;
	std::vector<double> mes_spec;
	std::vector<double> mes_spec_err_l;
	std::vector<double> mes_spec_err_h;
	std::vector<double> mes_spec_mask;
};

#endif

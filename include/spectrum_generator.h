#ifndef SPECTRUM_GENERATOR_H
#define SPECTRUM_GENERATOR_H

#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <map>
#include <chrono>
#include <string.h>
#include "sps_data.h"
#include "sps_write.h"


#ifdef __APPLE__
    #include "OpenCL/opencl.h"
#else
    #include "CL/cl.h"
#endif



class spectrum_generator{
public:

//constructor
	spectrum_generator(sps_data& input_data,std::string kernel_filename,int input_platform,int input_device);

/*
 FUNCTIONS
 */
    
public:
    
//intial functions
    
    //copy and move data from the sps_data object
    int copy_data(sps_data& model);
	//do all opencl related initialization
	int opencl_initialize(std::string kernel_filename,int platform,int device);
    
private:
    //query and select opencl platform
    int select_opencl_platform(int input_platform);
    int query_opencl_platforms(cl_uint& n_platforms,cl_platform_id*& platforms);
    int query_opencl_platform_info(int i, cl_platform_id platform);
    //query and select opencl device
    int select_opencl_device(int input_device);
    int query_opencl_devices(cl_platform_id platform,cl_uint& n_devices,cl_device_id*& devices);
    int query_opencl_device_info(int i, cl_device_id device);
    
    int opencl_create_context();
    int opencl_create_command_queue();
    int opencl_build_program(std::string kernel_filename);
    int convert_file_to_string(std::string infilename , std::string& s);
    int opencl_create_buffers();
    int opencl_create_kernels();
    int set_initial_kernel_args();


//functions called during operation

public:
    int generate_spectrum(std::map<std::string,double>& parameters, std::vector<double>& sfr);
    int generate_spectrum(std::map<std::string,double>& parameters);
private:
    int generate_spectrum(cl_kernel kernel_spec_gen);
    int set_params( std::map<std::string,double>& parameters, std::vector<double>& sfr);
    int set_params( std::map<std::string,double>& parameters, cl_kernel kernel_spec_gen );
    int change_kernel_params(cl_kernel kernel_spec_gen);

public:
    double compare_to_measurement();
private:
    cl_float get_factor_to_scale_spectra_to_measurement();
    double get_chi_square(cl_float scale_factor);
    
public:
    std::vector<cl_float> get_result();

//functions called at the end
    
public:
	int clean_resources();
    int write_specs(std::vector< std::vector<cl_float> >& results,std::string out_fname);
	
	//writes the fitted, and the measured model
	//int write_fit_result();
    
    
    
    
/*
 DATA
 */
    
private:
    
//sps model data
    
    //ages where model data is available
    std::vector<cl_float> time;
    //stores the resampled model
    std::vector<cl_float> resampled_model;
    //sizes of model data points
    int ntimesteps;
    
    
//measured spectrum data
    
	//the intensity
	std::vector<cl_float> mes_spec;
	//error is pessimistic, uses the larger 
	//of low and high errors
	std::vector<cl_float> mes_spec_err;
	//mask from measurement
	//bad data points, and emission lines are not fitted
	std::vector<cl_float> mes_spec_mask;
	//the wavelengths of the measurement (in Angstroms)
	std::vector<cl_float> mes_spec_wavel;
	//number of measured points
	int mes_nspecsteps;


//model parameters
    
    //sfr mode
    std::string sfr_mode;
    std::vector<cl_float> sfr;
    
	//-age is the beggining time of the sytnhesis
	//-sfr_tau: now SFR is an exponential decay
	//sfr_tau is the time constant of the decay
	//-metall : is the metallicity interpolated from the
	//available metallicity models
	//-dust_mu, dust_tau_v are the parameters of the
	// Charlot & Fall dust model
	//-vdisp is the velocity dispersion in velocity of light units
	cl_float dust_tau_v,dust_mu,sfr_tau, age, metall, vdisp;


    //aggregated value of weighted squared errors
	cl_float chi;


//kernels
    
	//generates the spectra with no vdisp, sfr from file or exponential
	cl_kernel kernel_spec_gen_exp,kernel_spec_gen_file;
    
	//adds vdisp (this is a different kernel, because
	//the convolution of the spectra is not fully paralel,
	//you need some neighbour points to work with)
	cl_kernel kernel_vel_disp;
    
    //calculates the factors to pull measured and model
    //spectra together
    cl_kernel kernel_get_factors;
	
	//calculates error normalized squares
	cl_kernel kernel_chi_calc;
	

//buffers on the GPU (other device) (last "d" indicates device)
	
    //buffers to read on GPU
    cl_mem time_d,wavel_d,resampled_model_d,sfr_d;
	cl_mem mes_spec_d,mes_spec_err_d,mes_spec_mask_d;
	
    //buffers to read and write on GPU
	cl_mem model_d,result_no_vel_d,result_d,factor1_d,factor2_d,chi_d;
    
    
//opencl related objects
    
    cl_uint n_platforms;
    cl_platform_id* platforms;
    cl_platform_id platform;
    
    cl_uint n_devices;
    cl_device_id* devices;
    cl_device_id device;
    
    cl_context context;
    cl_program program;
    cl_command_queue commandQueue;


};

#endif

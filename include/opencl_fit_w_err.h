#ifndef OPENCL_FIT_W_ERR_H
#define OPENCL_FIT_W_ERR_H

#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>

//include the opencl header
#ifdef __APPLE__
    #include "OpenCL/opencl.h"
#else
    #include "CL/cl.h"
#endif

//include the read class header
//this reads models and measurement
//this module uses its outputs
#include "sps_read.h"



class opencl_fit_w_err{
public: //contructor

	//contructor copies some data from model
	opencl_fit_w_err(sps_read& model);

public: //functions

//intial functions
	//models are resampled to the resolution os the measurement
	int resample_models_2_mes(sps_read& model);
	
	//does basic opencl operations (choose platfrom devices etc)
	int opencl_initialize(std::string kernel_filename);
	//reads the kernel files to a string
	int convertToString(std::string infilename , std::string& s);

	//creates kernels objects, and allocates buffers on the GPU (or other device)
	int opencl_kern_mem();

	//passes buffer arguments to kernels
	int set_kern_arg();


//fucntions called in every interation
	//the module communicates with the MCMC module through
	//this function: MCMC sets the modified parameters here
	int set_params(
				double s_dust_tau_v,
				double s_dust_mu,
				double s_sfr_tau,
				double s_age,
				double s_metall,
				double s_vdisp);



	//passes changed arguments to kernels
	int change_kernel_params();

	//runs the kernel
	//this is the core of the program
	//kernels calculate the models, and compare it to
	//the measurement
	int call_kernels();

	//if the result is the best one yet, it reads it back
	//from GPU to CPU memmory
	int read_best_result();

//functions called at the end
	//deletes buffers, and opencl objects
	int clean_resources();
	
	//writes the fitted, and the measured model
	int write_fit_result();



public: //data

	//beggining and end of execution,
	//TODO this should be improved!!
	//linux, mac and windows write different times 
	double t1, t2;

//measured spectrum containers
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
	size_t mes_nspecsteps;

//data for generating spectra
	//ages where model data is available
	std::vector<cl_float> time;
	//wavelengths of the model
	std::vector<cl_float> wavelengths;
	
	//will store the resampled model
	//not resampled model is not copied here 
	// from read module, only used
	std::vector<cl_float> resampled_model;

	//numbers of points
	int nspecsteps;
	int ntimesteps;

	//indicator, which imf is used
	//TODO choice should be in a config file!
	//now it is hardcoded Chabrier
	std::string imf;


	//fitted parameters
	//-age is the beggining time of the sytnhesis
	//-sfr_tau: now SFR is an exponential decay
	//sfr_tau is the time constant of the decay
	//-metall : is the metallicity interpolated from the
	//available metallicity models
	//-dust_mu, dust_tau_v are the parameters of the
	// Charlot & Fall dust model
	//-vdisp is the velocity dispersion in velocity of light units
	cl_float dust_tau_v,dust_mu,sfr_tau, age, metall, vdisp;


//buffers to write
	//factors calucalted on gpu to pull spectra together
	std::vector<cl_float> factor1,factor2;
	//chis calculated on gpu
	//they are not actual chis but the sum of 
	//error normalized squares
	std::vector<cl_float> chis;
	//aggregated value of chis
	cl_float chi;

	//best fitting model
	std::vector <cl_float> result;


//opencl related objects, and data

	//platfroms and devices are only used in the 
	//initializing function so they are not global now

	//context program commmandque (opencl magic)
	cl_context context;
	cl_program program;
	cl_command_queue commandQueue;


//kernels
	
	//generates the spectra with no vdisp
	cl_kernel kernel_spec_gen;

	//adds vdisp (this is a different kernel, because
	//the convolution of the spectra is not fully paralel,
	//you need some neighbour points to work with)
	//also calculates the factors to pull measured and model
	//spectra together
	cl_kernel kernel_vel_disp;
	
	//calculates error normalized squares
	cl_kernel kernel_chi_calc;
	

//buffers on the GPU (other device) (last "d" indicates device)
	//buffers to read on GPU
	cl_mem model_without_dust_d,time_d,wavel_d,resampled_model_d;
	cl_mem mes_spec_d,mes_spec_err_d,mes_spec_mask_d;
	//buffers to write on GPU
	cl_mem model_d,result_no_vel_d,result_d,factor1_d,factor2_d,chi_d;	

};

#endif

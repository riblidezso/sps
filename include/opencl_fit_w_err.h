#ifndef OPENCL_FIT_W_ERR_H
#define OPENCL_FIT_W_ERR_H

#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>

#ifdef __APPLE__
    #include "OpenCL/opencl.h"
#else
    #include "CL/cl.h"
#endif

#include <random>

#include "sps_read.h"

#include <map>
#include <time.h>

class opencl_fit_w_err{
public:
	opencl_fit_w_err(sps_read& model);

public: //functions

	int resample_models_2_mes(sps_read& model);

	int convertToString(const char *filename, std::string& s);
	int opencl_initialize(std::string kernel_filename);
	int opencl_kern_mem();

	int set_params(
				double s_dust_tau_v,
				double s_dust_mu,
				double s_sfr_tau,
				double s_age,
				double s_metall,
				double s_vdisp);

	int set_kern_arg();

	int change_kernel_params();

	int call_kernels();

	int read_best_result();

	int clean_resources();
	
	int write_fit_result();

public: //data

	clock_t t1, t2;

//measured spectrum containers
	std::vector<cl_float> mes_spec;
	std::vector<cl_float> mes_spec_err;
	std::vector<cl_float> mes_spec_mask;
	std::vector<cl_float> mes_spec_wavel;
	//size
	size_t mes_nspecsteps;

//data for generating spectra

	//stores the ages where model data is available
	std::vector<cl_float> time;
	//stores the wavelengths of the model
	std::vector<cl_float> wavelengths;
	//the resampled model
	std::vector<cl_float> resampled_model;

	//sizes
	int nspecsteps;
	int ntimesteps;

	//indicating which imf is used
	std::string imf;

	//fitted parameters
	cl_float dust_tau_v,dust_mu,sfr_tau, age, metall, vdisp;

//buffers to write
	//factors calucalted on gpu
	//to pull spectra together
	std::vector<cl_float> factor1,factor2;
	//chis calculated on gpu
	std::vector<cl_float> chis;
	//aggregated value of chi
	cl_float chi;

	//best fitting model
	std::vector <cl_float> result;


//opencl

	//context program commmandque
	cl_context context;
	cl_program program;
	cl_command_queue commandQueue;

	//kernels
	cl_kernel kernel_spec_gen;
	cl_kernel kernel_vel_disp;
	cl_kernel kernel_chi_calc;
	
	//data
	cl_mem model_without_dust_d, time_d,wavel_d,resampled_model_d;
	cl_mem mes_spec_d,mes_spec_err_d,mes_spec_mask_d;
	//buffers to write
	cl_mem model_d,result_no_vel_d,result_d,factor1_d,factor2_d,chi_d;	

	


};

#endif

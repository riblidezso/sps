#ifndef OPENCL_FIT1_H
#define OPENCL_FIT1_H

#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>

#include <CL/cl.h>
#include <ctime>

#include "read.h"

class opencl_fit1{
public:
	opencl_fit1(read& model);

public: //functions

	int convertToString(const char *filename, std::string& s);
	int opencl_start(std::string kernel_filename);
	int opencl_spec(read& model);
	int set_kern_arg();
	int call_kernels();

	int set_initial_params(double s_dust_tau_v,
							double s_dust_mu,
							double s_sfr_tau,
							double s_age,
							double s_metall);
	int change_params();

	int evaluate_chi();

	int write_results(read& model);

	int clean_resources();

public: //data

	int nspecsteps;
	int ntimesteps;
	clock_t t1, t2;

/*opencl*/
	cl_context context;
	cl_program program;
	cl_command_queue commandQueue;

	cl_kernel kernel;
	cl_kernel kernel2;
	
	//data
	cl_mem model_without_dust_d, time_d,wavel_d,sample_spec_d;
	//buffers to write
	cl_mem model_d,result_d,factor1_d,factor2_d,chi_d;	

/*fit data */
	std::string imf;

	double dust_tau_v,dust_mu,sfr_tau, age, metall;
	double d_dust_tau_v, d_dust_mu, d_sfr_tau, d_age, d_metall;
	double best_dust_tau_v, best_dust_mu, best_sfr_tau, best_age, best_metall;

	double chi_before, best_chi;

	std::vector<double> factor1;		//stores one factor 
	std::vector<double> factor2;		//stores an other
	std::vector<double> chis;			//stores the chi sqr values


/* results */
	std::vector<double> out_chi_evol;	// stores the chi values
	std::vector<double> out_best_chi_evol;	// stores the best chi values
	std::vector<double> out_acc_chi_evol;	// stores the accepted chi values
	std::vector<double> out_worse_acc_chi_evol;	// stores the worse than before, but accepted chi values

	std::vector <double> result;		// stores the best fitting model

	//stores the ages where model data is available
	std::vector<double> time;
	//stores the wavelengths of the model
	std::vector<double> wavelengths;


	//sample spectrum container
	std::vector<double> sample_spec;

};

#endif

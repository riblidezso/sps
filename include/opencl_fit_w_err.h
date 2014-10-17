#ifndef OPENCL_FIT_W_ERR_H
#define OPENCL_FIT_W_ERR_H

#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>

#include <CL/cl.h>
#include <ctime>

#include <random>

#include "read.h"

class opencl_fit_w_err{
public:
	opencl_fit_w_err(read& model);

public: //functions
	int resample_models_2_mes(read& model);

	int convertToString(const char *filename, std::string& s);
	int opencl_initialize(std::string kernel_filename);
	int opencl_kern_mem();
	int set_kern_arg();
	int call_kernels();

	int set_initial_params(	double s_dust_tau_v,
				double s_dust_mu,
				double s_sfr_tau,
				double s_age,
				double s_metall,
				double s_vdisp);

	int change_params(double opt_acc);

	int evaluate_chi(double temp);

	int record_data();

	int write_results();

	int clean_resources();

public: //data

	int nspecsteps;
	int ntimesteps;
	clock_t t1, t2;

/*opencl*/
	cl_context context;
	cl_program program;
	cl_command_queue commandQueue;

	cl_kernel kernel_spec_gen;
	cl_kernel kernel_vel_disp;
	cl_kernel kernel_chi_calc;
	
	//data
	cl_mem model_without_dust_d, time_d,wavel_d,resampled_model_d;
	cl_mem mes_spec_d,mes_spec_err_d,mes_spec_mask_d;
	//buffers to write
	cl_mem model_d,result_no_vel_d,result_d,factor1_d,factor2_d,chi_d;	

/*fit data */
	std::string imf;

	double dust_tau_v,dust_mu,sfr_tau, age, metall, vdisp;
	double d_dust_tau_v, d_dust_mu, d_sfr_tau, d_age, d_metall, d_vdisp;
	double best_dust_tau_v, best_dust_mu, best_sfr_tau, best_age, best_metall, best_vdisp;

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


	//measured spectrum containers
	std::vector<double> mes_spec;
	std::vector<double> mes_spec_err;
	std::vector<double> mes_spec_mask;
	std::vector<double> mes_spec_wavel;




	std::vector<double> acc;		//stores the last some accept or nots (0,1)
	std::vector<double> acc_ratio;	// stores averaged acceptance rates
	
	std::vector<double> worse;		//stores the last some worse ot nots (0,1)
	std::vector<double> worse_rate;	// stores averaged rates

	std::vector<double> worse_acc;		//stores the last some worse accept or nots (0,1)
	std::vector<double> worse_acc_ratio;	// stores averaged worse acceptance rates

	std::default_random_engine generator;
	int iter;

	int accepted;
	double chi;

	std::vector<double> temp_point;
	std::vector< std::vector <double> > points;

	double sigma;

	std::vector<double> resampled_model;
	size_t mes_nspecsteps;

	//burnin indicator
	bool burnin_ended;

	//parameter fixing variables
	double fix_dust_tau_v;
	double fix_dust_mu;
	double fix_sfr_tau;
	double fix_age;
	double fix_metall;
	double fix_vdisp;

};

#endif

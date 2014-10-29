#include <iostream>
#include <sstream>
#include <ctime>

#ifdef __APPLE__
    #include "OpenCL/opencl.h"
#else
    #include "CL/cl.h"
#endif

#include <string>
#include <string.h>

#include "sps_read.h"
#include "sps_write.h"
#include "opencl_fit_w_err.h"
#include "sps_mcmc.h"


int main(int argc, char* argv[])
{
	int error=0;
	int MAXITER;

	//reading models
	sps_read model;
	error=model.read_time_bin();
	if(error!=0)
		return 1;
	error|=model.read_wavel_bin();
	if(error!=0)
		return 1;
	error|=model.read_model_bin_all_cont();
	if(error!=0)
		return 1;
	
	//reading spectrum to fit	
	error|=model.usr_read_sdss_csv();


	//initialize objects
	opencl_fit_w_err fitter(model);
	sps_mcmc mcmc_fitter;

	//test config read
	if(argc==3)
	{
		//initializing parameters in mcmc module
		mcmc_fitter.read_config(argv[1]);
		std::cout<<"config file read"<<std::endl;
		MAXITER=atoi(argv[2]);
		
		
	}
	else
	{
		std::cerr<<"ERROR please use max 2 command line arguments"<<std::endl;
		std::cerr<<"\t 1. command line argument should point to congfig file"<<std::endl;
		std::cerr<<"\t current directory is \"sps/bin\""<<std::endl;
		std::cerr<<"\n\t 2. command line argument is number of iterations (recommended: 40000)"<<std::endl;
		return 1;
	}

	//resampling models
	fitter.resample_models_2_mes(model);

	//basic stuff
	error|=fitter.opencl_initialize("fit_w_err.cl");
	if(error!=0)
		return 1;
	//kernel and memory
	error|=fitter.opencl_kern_mem();
	if(error!=0)
		return 1;
	//setting the device memories of kernels
	error|=fitter.set_kern_arg();
	if(error!=0)
		return 1;


	std::cout<<"fitting started...:\n";

	// fitting 
	for(mcmc_fitter.iter= 0; mcmc_fitter.iter<(MAXITER);mcmc_fitter.iter++)
	{
		error=mcmc_fitter.control_step_size(0.5);
		if(error!=0)
			break;

		error=mcmc_fitter.change_params();
		if(error!=0)
			break;

		//more error checking
		error=fitter.set_params(
				mcmc_fitter.parameters["dust_tau_v"],
				mcmc_fitter.parameters["dust_mu"],
				mcmc_fitter.parameters["sfr_tau"],
				mcmc_fitter.parameters["age"],
				mcmc_fitter.parameters["metall"],
				mcmc_fitter.parameters["vdisp"]	);


		error=fitter.change_kernel_params();
		if(error!=0)
			break;

		error=fitter.call_kernels();
		if(error!=0)
			break;
		
		error=mcmc_fitter.evaluate_chi((double) fitter.chi);
		if(error!=0)
			break;

		error=fitter.read_best_result();
		if(error!=0)
			break;

		error=mcmc_fitter.record_data();
		if(error!=0)
			break;

		if( (mcmc_fitter.iter % 1000) == 0 && mcmc_fitter.iter>0)
			std::cout<<mcmc_fitter.iter<<" iterations done..."<<"\n";
		if( (mcmc_fitter.iter % 10000) == 0 && mcmc_fitter.iter>0)
			std::cout<<"\n";	
	}

	error=mcmc_fitter.write_results();
	if(error!=0)
		return 1;
	error=fitter.write_fit_result();
	if(error!=0)
		return 1;

	error=fitter.clean_resources();
	if(error!=0)
		return 1;

	if (error==0)
	{
		std::cout<<"Passed!"<<std::endl;
	}

	return	error;
}

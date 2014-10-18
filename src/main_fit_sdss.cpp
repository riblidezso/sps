#include <iostream>
#include <sstream>
#include <ctime>

#include <CL/cl.h>

#include <string>
#include <string.h>

#include "read.h"
#include "write.h"
#include "opencl_fit_w_err.h"


int main(int argc, char* argv[])
{
	int error=0;
	int MAXITER;

	//reading models
	read model;
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

	opencl_fit_w_err fitter(model);

	//test config read
	if(argc==3)
	{
		fitter.read_config(argv[1]);
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

	//set ininital parameter guesses read
	//from the config file above
	fitter.set_initial_params();
	//fix parameters if asked to
	fitter.fix_params();


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
	for(fitter.iter= 0; fitter.iter<(MAXITER);fitter.iter++)
	{
		error=fitter.change_params(0.3);
		if(error!=0)
			break;

		error=fitter.call_kernels();
		if(error!=0)
			break;
		
		error=fitter.evaluate_chi(1);
		if(error!=0)
			break;

		error=fitter.record_data();
		if(error!=0)
			break;

		if( (fitter.iter % 1000) == 0 && fitter.iter>0)
			std::cout<<fitter.iter<<" iterations done..."<<"\n";
		if( (fitter.iter % 10000) == 0 && fitter.iter>0)
			std::cout<<"\n";	
	}

	error=fitter.write_results();
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

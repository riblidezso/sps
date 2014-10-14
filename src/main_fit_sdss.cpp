#include <iostream>
#include <sstream>
#include <ctime>

#include <CL/cl.h>

#include <string>
#include <string.h>

#include "read.h"
#include "write.h"
#include "opencl_fit_w_err.h"

//reading model!

int main()
{
	int error=0;

	//reading models
	read model;
	model.read_time_bin();
	model.read_wavel_bin();
	model.read_model_bin_all_cont();
	
	//reading spectrum to fit	
	model.usr_read_sdss_csv();

	opencl_fit_w_err fitter(model);
	//resampling model
	fitter.resample_mes(model);

	//basic stuff
	error=fitter.opencl_start("fit_w_err.cl");
	//kernel and memory
	error=fitter.opencl_kern_mem();
	//setting the device memories of kernels
	error=fitter.set_kern_arg();

	fitter.set_initial_params(1.0,0.3,1e09,5e9,0.01);

	std::cout<<"no. of iterations:\n";

	// fitting 
	for(fitter.iter= 0; fitter.iter<(5000*10);fitter.iter++)
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
			std::cout<<fitter.iter<<"\n";
		if( (fitter.iter % 10000) == 0 && fitter.iter>0)
			std::cout<<"\n";	
	}

	error=fitter.write_results();

	error=fitter.clean_resources();

	std::cout<<"\nready\n";

	if (error==0)
	{
		std::cout<<"Passed! (press enter)"<<std::endl;
		std::cin.get();
	}

	return	error;
}

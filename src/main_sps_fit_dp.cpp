#include <iostream>
#include <sstream>
#include <ctime>

#include <CL/cl.h>

#include <string>
#include <string.h>

#include "read.h"
#include "write.h"
#include "opencl_fit1.h"

int main()
{
	int error=0;

//reading models
	read model;
	model.read_time_bin();
	model.read_wavel_bin();
	model.read_model_bin_all_cont();
	//reading spectrum to fit
	model.usr_read_sample();


	opencl_fit1 fitter(model);
	//basic stuff
	error=fitter.opencl_start("fit_dp.cl");
	//kernel and memory
	error=fitter.opencl_spec(model);
	//setting the device memories of kernels
	error=fitter.set_kern_arg();

	fitter.set_initial_params(0.6,0.5,5e09,5e9,0.01);

	std::cout<<"no. of iterations:\n";

	/* actual fitting */
	for(int i=0; i<2000	;i++)
	{
		fitter.change_params();

		error=fitter.call_kernels();
		
		error=fitter.evaluate_chi();

		if( (i % 100) == 0 && i>0)
			std::cout<<i<<"  ";
		if( (i % 1000) == 0 && i>0)
			std::cout<<"\n";
	}

	error=fitter.write_results(model);

	error=fitter.clean_resources();

	std::cout<<"\nready\n";

	if (error==0)
	{
		std::cout<<"Passed! (press enter)"<<std::endl;
		std::cin.get();
	}

	return	error;

}
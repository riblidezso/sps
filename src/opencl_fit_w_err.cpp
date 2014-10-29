#include "opencl_fit_w_err.h"
#include "sps_read.h"
#include "sps_write.h"

#include <float.h>
#include <string.h>



//initalizing from the read object
opencl_fit_w_err::opencl_fit_w_err(sps_read& model)
{
	//copy data sizes
	nspecsteps=model.wavelengths.size();
	ntimesteps=model.time.size();
	mes_nspecsteps=model.mes_spec_wavel.size();

	//copy small data
	time=std::vector<cl_double>(model.time.begin(),model.time.end());
	mes_spec=std::vector<cl_double>(model.mes_spec.begin(),model.mes_spec.end());
	mes_spec_wavel=std::vector<cl_double>(model.mes_spec_wavel.begin(),model.mes_spec_wavel.end());
	mes_spec_mask=std::vector<cl_double>(model.mes_spec_mask.begin(),model.mes_spec_mask.end());
	mes_spec_err=std::vector<cl_double>(model.mes_spec_err_h.begin(),model.mes_spec_err_h.end());

	//resize the data scturcures that will 
	//be computed at every wavelength
	factor1.resize(mes_nspecsteps);		 
	factor2.resize(mes_nspecsteps);		
	chis.resize(mes_nspecsteps);
	result.resize(mes_nspecsteps);

	//imf, if you want to use salpeter
	//uncomment that line and comment chabrier
	imf="chabrier";
	//imf="salpeter";

}


//this function gets 
//original models from read object
//and measuremant data

//and then resamples all the models to the wavelengths of
//the measurement, this reduces computation time 6900->3-4000
int opencl_fit_w_err::resample_models_2_mes(sps_read& model)
{
	//resize the vector that will store the resampled model
	resampled_model.resize(ntimesteps * mes_nspecsteps * 6);

	//wd, wu are weigths for interpolation, delta is wavelength distance
	double wd,wu,delta;

	//these will store temporary positions
	int low,high,place,place1;
	high=0;

	//calculate model offset from imf
	// 0-5 is chabrier, 6-11 is salpeter
	// this is not very elegant
	int offset;
	if( imf == "chabrier")
		offset=0;
	else if(imf=="salpeter")
		offset=6;

	//the resampling
	//loop over measurement points
	for(int i=0;i<mes_nspecsteps;i++)
	{
		//find nearest model wavelength points

		//find the first model wavelength that is bigger
		//than measurement wavelength
		while(mes_spec_wavel[i] > model.wavelengths[high])
		{
			high++;
			//there might be problems if the one measured data is 
			//larger than last model data, but this is unlikely
			if (high==nspecsteps)
			{
				std::cerr<<"\nERROR measurement wavelength"<<mes_spec_wavel[i]<<"\n";
				std::cerr<<"can't be interpolated from model wavelength points\n\n";
				return 1;
			}
		}
			
		//there might be problems if the first measured data is 
		//smaller than first model data, but this is unlikely
		if (high==0)
		{
			std::cerr<<"\nERROR measurement wavelength"<<mes_spec_wavel[i]<<"\n";
			std::cerr<<"can't be interpolated from model wavelength points\n\n";
			return 1;
		}

		//the one before the first bigger is definitely smaller
		low=high-1;

		//calculate distance and weights
		delta=model.wavelengths[high] - model.wavelengths[low];
		wd= (model.wavelengths[high]-mes_spec_wavel[i])/delta;
		wu= (mes_spec_wavel[i]- model.wavelengths[low])/delta;


		//the intepolation

		//loop over different metallicity models
		for(int k=offset;k<offset+6;k++)
		{
			//loop over timesteps
			for(int j=0;j<ntimesteps;j++)
			{
				//calculate positions in the big continous models
				//vector (matrix) that stores all models
				place=mes_nspecsteps*ntimesteps*k + mes_nspecsteps*j;
				place1=nspecsteps*ntimesteps*k + nspecsteps*j;
				//interpolation
				resampled_model[place+i]=wd*model.model_cont[low+place1]+wu*model.model_cont[high+place1];
			}
		}
	}
	return 0;
}

//function from ATI SDK
// convert the kernel file into a string 
int opencl_fit_w_err::convertToString(const char *filename, std::string& s)
{
	size_t size;
	char*  str;
	std::fstream f(filename, (std::fstream::in | std::fstream::binary));

	if(f.is_open())
	{
		size_t fileSize;
		f.seekg(0, std::fstream::end);
		size = fileSize = (size_t)f.tellg();
		f.seekg(0, std::fstream::beg);
		str = new char[size+1];
		if(!str)
		{
			f.close();
			return 0;
		}

		f.read(str, fileSize);
		f.close();
		str[size] = '\0';
		s = str;
		delete[] str;
		return 0;
	}
	std::cerr<<"Error: failed to open kernel file\n:"<<filename<<std::endl;
	return 1;
}


int opencl_fit_w_err::opencl_initialize(std::string kernel_filename)
{
	//error variable
	cl_int	status=0;


	//Getting OpenCL platforms and choose an available one.
	cl_uint numPlatforms;			//the NO. of platforms
	cl_platform_id* platforms = NULL; 	//id of available platforms
	cl_platform_id 	platform = NULL;	//id of the chosen platform


	//getting NO. of platforms
	status = clGetPlatformIDs(0, NULL, &numPlatforms); 
	if (status != CL_SUCCESS)
	{
		std::cerr << "Error: Getting platforms!" << std::endl;
		std::cerr << "Error number= " <<status<< std::endl;
		return status;
	}

	//Choosing platform
	if(numPlatforms > 0)
	{
		//getting platform ids
		platforms =  new cl_platform_id[numPlatforms];
		status = clGetPlatformIDs(numPlatforms, platforms, NULL);

		//printing platform names
		std::cout<<"\nPlatform info:"<<std::endl;
		for(unsigned int i=0;i<numPlatforms;i++)
		{
			//get platform name size
			size_t platform_name_size;
			status = clGetPlatformInfo( platforms[i] , CL_PLATFORM_NAME ,0, NULL, &platform_name_size);

			//get platform name
			char* platform_name = new char[platform_name_size];
			status = clGetPlatformInfo( platforms[i] , CL_PLATFORM_NAME ,platform_name_size, platform_name,NULL);

			//print info
			std::cout<<i<<". platform: "<<platform_name<<"\n";
			delete[] platform_name;		
		}

		//choosing platform
		std::cout<<"\nChoose platform: (0)"<<std::endl;
		int platform_choice=0;
		std::string temp_line;
		getline(std::cin,temp_line);
		std::stringstream temp_sstr;
		temp_sstr<<temp_line;
		temp_sstr>>platform_choice;
		std::cout<<"platform choice"<<platform_choice<<std::endl;
		platform = platforms[platform_choice];

		delete[] platforms;		
	}

	//Query the platform and choose the  device.

	cl_uint		numDevices = 0; 	//NO. of devices
	cl_device_id	*devices;		// device ids
	cl_device_id	device;			//id of chosen device

	//getting number of devices
	status = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 0, NULL, &numDevices);	
	devices = new cl_device_id[numDevices];

	//getting device ids
	status = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, numDevices, devices, NULL);
	
	//printing device info
	std::cout<<"\nDevice info:"<<std::endl;
	for (unsigned int i=0;i<numDevices;i++)
	{
		//get device vendor size
		size_t device_vendor_size;
		status = clGetDeviceInfo( devices[i], CL_DEVICE_VENDOR, 0,NULL,&device_vendor_size);

		//get device vendor
		char* device_vendor = new char[device_vendor_size];
		status = clGetDeviceInfo( devices[i], CL_DEVICE_VENDOR, device_vendor_size,device_vendor,NULL);

		//get device name size
		size_t device_name_size;
		status = clGetDeviceInfo( devices[i], CL_DEVICE_NAME, 0,NULL,&device_name_size);

		//get device name
		char* device_name = new char[device_name_size];
		status = clGetDeviceInfo( devices[i], CL_DEVICE_NAME, device_name_size,device_name,NULL);

		//get devicetype 
		cl_device_type device_type;
		status = clGetDeviceInfo( devices[i], CL_DEVICE_TYPE, sizeof(cl_device_type),&device_type,NULL);

		//print info
		std::cout<<i<<". device vendor: "<<device_vendor<<std::endl;
		std::cout<<"            name:"<<device_name<<std::endl;

		//device type 
		if( device_type == CL_DEVICE_TYPE_CPU )
			std::cout<<"            type: CPU"<<std::endl;
		if( device_type == CL_DEVICE_TYPE_GPU )
			std::cout<<"            type: GPU"<<std::endl;
		if( device_type == CL_DEVICE_TYPE_ACCELERATOR )
			std::cout<<"            type: ACCELERATOR"<<std::endl;
		if( device_type == CL_DEVICE_TYPE_DEFAULT)
			std::cout<<"            type: DEFAULT"<<std::endl;

		delete[] device_vendor;
		delete[] device_name;
	}

	//choosing device
	std::cout<<"\nChoose device: (0)"<<std::endl;
	int device_choice=0;
	std::string temp_line1;
	getline(std::cin,temp_line1);
	std::stringstream temp_sstr1;
	temp_sstr1<<temp_line1;
	temp_sstr1>>device_choice;
	device = devices[device_choice];

	//starting clock for the whole program
	t1 = clock();

	//Create context
	context = clCreateContext(NULL,1, devices,NULL,NULL,&status);
	if (status!=0)
	{
		std::cerr<<"ERROR creating context: "<<status<<std::endl;
		return status;
	}
 
	//Creating command queue associate with the context
	commandQueue = clCreateCommandQueue(context, device, 0, &status);
	if (status!=0)
	{
		std::cerr<<"ERROR creating commandqueue: "<<status<<std::endl;
		return status;
	}

	//open kernel file and convert it to char array
	const char *filename = kernel_filename.c_str();
	std::string sourceStr;
	status = convertToString(filename, sourceStr);
	const char *source = sourceStr.c_str();
	size_t sourceSize[] = {strlen(source)};

	//Create program object
	program = clCreateProgramWithSource(context, 1, &source, sourceSize, &status);
	if (status!=0)
	{
		std::cout<<"ERROR creating program: "<<status<<std::endl;
		return status;
	}

	//Building program 
	status=clBuildProgram(program, 1,devices,NULL,NULL,NULL);
	if (status!=0)
	{
		//print ERROR but do not quit, there may be just warnings
		std::cerr<<"ERROR building program: "<<status<<std::endl;

		//Getting build log size
		size_t logsize=0;
		clGetProgramBuildInfo(program,device,CL_PROGRAM_BUILD_LOG,0,NULL,&logsize);
		std::cout<<logsize<<std::endl;

		//Getting build log
		char* log = new char[logsize];	
		clGetProgramBuildInfo(program,device,CL_PROGRAM_BUILD_LOG,logsize,log,NULL);

		//print log info
		std::cout<<"log:\n "<<log<<std::endl;
		delete[] log;

		return status;
	}

	return status;
}

//this function allocates buffers on devices
//and creates kernels
int opencl_fit_w_err::opencl_kern_mem()
{
	cl_int status=0;
	
	// Allocate memory on device 
	
	//data
	resampled_model_d = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, resampled_model.size() * sizeof(cl_double),(void *) resampled_model.data(), &status);
	time_d = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, ntimesteps * sizeof(cl_double),(void *) time.data(), &status);
	wavel_d = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, mes_nspecsteps * sizeof(cl_double),(void *) mes_spec_wavel.data(), &status);
	mes_spec_d = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, mes_nspecsteps * sizeof(cl_double),(void *) mes_spec.data(), &status);
	mes_spec_err_d = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, mes_nspecsteps * sizeof(cl_double),(void *) mes_spec_err.data(), &status);
	mes_spec_mask_d = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, mes_nspecsteps * sizeof(cl_double),(void *) mes_spec_mask.data(), &status);
	


	//buffers to write
	model_d = clCreateBuffer(context, CL_MEM_READ_WRITE,	sizeof(cl_double) * ntimesteps * mes_nspecsteps , NULL, &status);
	result_no_vel_d = clCreateBuffer(context, CL_MEM_READ_WRITE,	sizeof(cl_double) * mes_nspecsteps , NULL, &status);
	result_d = clCreateBuffer(context, CL_MEM_READ_WRITE,	sizeof(cl_double) * mes_nspecsteps , NULL, &status);
	factor1_d = clCreateBuffer(context, CL_MEM_WRITE_ONLY,	sizeof(cl_double) * mes_nspecsteps , NULL, &status);
	factor2_d = clCreateBuffer(context, CL_MEM_WRITE_ONLY,	sizeof(cl_double) * mes_nspecsteps , NULL, &status);
	chi_d = clCreateBuffer(context,  CL_MEM_WRITE_ONLY,		sizeof(cl_double) * mes_nspecsteps , NULL, &status);		

	//error check
	if (status!=0)
	{
		std::cerr<<"ERROR creating buffers: "<<status<<std::endl;
		return status;
	}


	// Create kernel objects
	kernel_spec_gen = clCreateKernel(program,"spec_gen", &status);
	if (status!=0)
	{
		std::cerr<<"ERROR creating kernel_spec_gen: "<<status<<std::endl;
		return status;
	}	

	kernel_vel_disp = clCreateKernel(program,"mask_veloc_disp", &status);
	if (status!=0)
	{
		std::cerr<<"ERROR creating kernel_vel_disp: "<<status<<std::endl;
		return status;
	}	

	kernel_chi_calc = clCreateKernel(program,"chi_calculation", &status);
	if (status!=0)
	{
		std::cerr<<"ERROR creating kernel_chi_calc: "<<status<<std::endl;
		return status;
	}	

	return status;
}

//Setting kernel arguments
//
//with arrays this need not be done again, even if
//data changes the kernel will read the new data
int opencl_fit_w_err::set_kern_arg()
{
	//error variable
	cl_int status=0;

	//kernel_spec_gen
	status = clSetKernelArg(kernel_spec_gen, 0, sizeof(cl_mem), &resampled_model_d);
	status |= clSetKernelArg(kernel_spec_gen, 1, sizeof(cl_mem), &time_d);
	status |= clSetKernelArg(kernel_spec_gen, 2, sizeof(cl_mem), &wavel_d);

	status |= clSetKernelArg(kernel_spec_gen, 3, sizeof(cl_mem), &mes_spec_d);
	status |= clSetKernelArg(kernel_spec_gen, 4, sizeof(cl_mem), &mes_spec_err_d);
	status |= clSetKernelArg(kernel_spec_gen, 5, sizeof(cl_mem), &mes_spec_mask_d);
		
	status |= clSetKernelArg(kernel_spec_gen, 6, sizeof(cl_mem), &model_d);
	status |= clSetKernelArg(kernel_spec_gen, 7, sizeof(cl_mem), &result_no_vel_d);
	status |= clSetKernelArg(kernel_spec_gen, 8, sizeof(cl_mem), &factor1_d);
	status |= clSetKernelArg(kernel_spec_gen, 9, sizeof(cl_mem), &factor2_d);
	
	status |= clSetKernelArg(kernel_spec_gen, 10, sizeof(int), &mes_nspecsteps);
	status |= clSetKernelArg(kernel_spec_gen, 11, sizeof(int), &ntimesteps);

	//kernel_vel_disp
	status = clSetKernelArg(kernel_vel_disp, 0, sizeof(cl_mem), &wavel_d);

	status |= clSetKernelArg(kernel_vel_disp, 1, sizeof(cl_mem), &mes_spec_d);
	status |= clSetKernelArg(kernel_vel_disp, 2, sizeof(cl_mem), &mes_spec_err_d);
	status |= clSetKernelArg(kernel_vel_disp, 3, sizeof(cl_mem), &mes_spec_mask_d);
		
	status |= clSetKernelArg(kernel_vel_disp, 4, sizeof(cl_mem), &result_no_vel_d);
	status |= clSetKernelArg(kernel_vel_disp, 5, sizeof(cl_mem), &result_d);
	status |= clSetKernelArg(kernel_vel_disp, 6, sizeof(cl_mem), &factor1_d);
	status |= clSetKernelArg(kernel_vel_disp, 7, sizeof(cl_mem), &factor2_d);
	
	status |= clSetKernelArg(kernel_vel_disp, 8, sizeof(int), &mes_nspecsteps);

	//kernel_chi_calc 
	status |= clSetKernelArg(kernel_chi_calc, 0, sizeof(cl_mem), &mes_spec_d);
	status |= clSetKernelArg(kernel_chi_calc, 1, sizeof(cl_mem), &mes_spec_err_d);
	status |= clSetKernelArg(kernel_chi_calc, 2, sizeof(cl_mem), &mes_spec_mask_d);
	status |= clSetKernelArg(kernel_chi_calc, 3, sizeof(cl_mem), &result_d);
	status |= clSetKernelArg(kernel_chi_calc, 4, sizeof(cl_mem), &chi_d);
	status |= clSetKernelArg(kernel_chi_calc, 6, sizeof(cl_mem), &wavel_d);
	
	//error check
	if (status!=0)
		std::cerr<<"ERROR setting kernel arguments: "<<status<<std::endl;

	return status;
}

//set params
int opencl_fit_w_err::set_params(
				double s_dust_tau_v,
				double s_dust_mu,
				double s_sfr_tau,
				double s_age,
				double s_metall,
				double s_vdisp)
{
	dust_tau_v = s_dust_tau_v;
	dust_mu = s_dust_mu;
	sfr_tau = s_sfr_tau;
	age = s_age;
	metall = s_metall;
	vdisp = s_vdisp;

	return 0;
}

int opencl_fit_w_err::change_kernel_params()
{
	cl_int status=0;

	//this part finds the metallicity models
	//nearest to metall, these will be use
	//at the interpolation step
	int offset;
	if( imf == "chabrier")
		offset=0;
	else if(imf == "salpeter")
		offset=6;
	
	double model_metal[6]={0.0001,0.0004,0.004,0.008,0.02,0.05};
	int modelno;
	//finding models to interpolate metall
	for(int j=0;j<6;j++)
	{
		if(model_metal[j] > metall)
		{
			modelno=j-1+offset;
			break;
		}
	}

	
//(ready reached max no of constant arguments...)
//set  the kernel arguments that has changed
	status = clSetKernelArg(kernel_spec_gen, 12, sizeof(cl_double), &dust_tau_v);
	status |= clSetKernelArg(kernel_spec_gen, 13, sizeof(cl_double), &dust_mu);
	status |= clSetKernelArg(kernel_spec_gen, 14, sizeof(cl_double), &sfr_tau);
	status |= clSetKernelArg(kernel_spec_gen, 15, sizeof(cl_double), &age);
	status |= clSetKernelArg(kernel_spec_gen, 16, sizeof(cl_double), &metall);
	status |= clSetKernelArg(kernel_spec_gen, 17, sizeof(int), &modelno);
	if (status!=0)
	{
		std::cerr<<"ERROR setting kernel_spec_gen arguments: "<<status<<std::endl;
		return status;
	}

	status = clSetKernelArg(kernel_vel_disp, 9, sizeof(cl_double), &vdisp);
	if (status!=0)
	{
		std::cerr<<"ERROR setting kernel_vel_disp arguments: "<<status<<std::endl;
		return status;
	}	
	return status;
}



//this function launches the kernels and reads back result from host(GPU)
int opencl_fit_w_err::call_kernels()
{
	//error variable
	cl_int status=0;

	//temp1, temp2 are sums of vectors factor1,factor2
	//they are use to calculate the "factor" that pulls together observed
	//and model spectra
	cl_double temp_1,temp_2,factor;

	//generating spectrum (no velocity dispersion)
	// Running the kernel.
	size_t global_work_size[1] = {mes_nspecsteps};
	status = clEnqueueNDRangeKernel(commandQueue, kernel_spec_gen, 1, NULL, global_work_size, NULL, 0, NULL,NULL);
	if (status!=0)
	{
		std::cerr<<"ERROR running kernel_spec_gen: "<<status<<std::endl;
		return status;
	}


	//next kernel for velocity dispersion
	// Running the kernel
	status = clEnqueueNDRangeKernel(commandQueue, kernel_vel_disp, 1, NULL, global_work_size, NULL, 0, NULL,NULL);
	if (status!=0)
	{
		std::cerr<<"ERROR running kernel_vel_disp: "<<status<<std::endl;
		return status;
	}

	//Read the result back to host memory
	status = clEnqueueReadBuffer(commandQueue, factor1_d, CL_TRUE, 0, mes_nspecsteps * sizeof(cl_double) , factor1.data(), 0, NULL,NULL);
	status = clEnqueueReadBuffer(commandQueue, factor2_d, CL_TRUE, 0, mes_nspecsteps * sizeof(cl_double) , factor2.data(), 0, NULL, NULL);
	if (status!=0)
	{
		std::cerr<<"ERROR reading buffer,from kernel_vel: "<<status<<std::endl;
		return status;
	}


	//summing factors, to pull spectra together
	temp_1=0;
	temp_2=0;
	for(int i=0;i<mes_nspecsteps;i++)
	{
		temp_1+=factor1[i];
		temp_2+=factor2[i];
	}
	factor=temp_1/temp_2;


	//next kernel for chi
	//Set kernel_chi_calc argument
	status |= clSetKernelArg(kernel_chi_calc, 5, sizeof(cl_double), &factor);
	if (status!=0)
	{
		std::cerr<<"ERROR setting kernel_chi_calc arguments: "<<status<<std::endl;
		return status;
	}
	//Running the kernel
	status = clEnqueueNDRangeKernel(commandQueue, kernel_chi_calc, 1, NULL, global_work_size, NULL, 0, NULL, NULL); 
	if (status!=0)
	{
		std::cerr<<"ERROR running kernel_chi_calc: "<<status<<std::endl;
		return status;
	}	

	//Read the chis to host memory.
	//now we dont read back the fittes spectrum
	//only when it is the best so far
	status = clEnqueueReadBuffer(commandQueue, chi_d, CL_TRUE, 0, mes_nspecsteps * sizeof(cl_double) , chis.data(), 0,NULL,NULL);
	if (status!=0)
	{
		std::cerr<<"ERROR reading chi buffer: "<<status<<std::endl;
		return status;
	}

	//summing chisquares
	chi=0;
	for(int i=0;i<mes_nspecsteps;i++)
		chi+=chis[i];

	return status;
}

int opencl_fit_w_err::read_best_result()
{
	cl_int status=0;

	status = clEnqueueReadBuffer(commandQueue, result_d, CL_TRUE, 0, mes_nspecsteps * sizeof(cl_double) , result.data(), 0, NULL, NULL);

	if (status!=0)
		std::cout<<"ERROR reading buffer: "<<status<<std::endl;

	return status;	
}

int opencl_fit_w_err::write_fit_result()
{
	//lets hope it works
	std::vector<std::vector <double> > output;
	output.push_back(std::vector<double>(mes_spec_wavel.begin(),mes_spec_wavel.end()));
	output.push_back(std::vector<double>(mes_spec.begin(),mes_spec.end()));
	output.push_back(std::vector<double>(mes_spec_err.begin(),mes_spec_err.end()));
	output.push_back(std::vector<double>(result.begin(),result.end()));

	//functions from sps_write 
	write_table_col(output,"../output/fit.dat");

	//info out
	std::cout<<"fit writing succesful: "<<std::endl;
	return 0;
}

int opencl_fit_w_err::clean_resources()
{
	cl_int status=0;

	//Clean the resources.
	//release kernels
	status = clReleaseKernel(kernel_spec_gen);
	status = clReleaseKernel(kernel_chi_calc);
	status = clReleaseKernel(kernel_vel_disp);

	//Release the program object.
	status = clReleaseProgram(program);

	//Release mem objects.
	status = clReleaseMemObject(resampled_model_d);
	status = clReleaseMemObject(time_d);
	status = clReleaseMemObject(wavel_d);

	status = clReleaseMemObject(mes_spec_d);
	status = clReleaseMemObject(mes_spec_err_d);
	status = clReleaseMemObject(mes_spec_mask_d);
	
	status = clReleaseMemObject(result_d);
	status = clReleaseMemObject(result_no_vel_d);
	status = clReleaseMemObject(model_d);
	status = clReleaseMemObject(factor1_d);
	status = clReleaseMemObject(factor2_d);
	status = clReleaseMemObject(chi_d);

	//Release  Command queue.
	status = clReleaseCommandQueue(commandQueue);
	//Release context.
	status = clReleaseContext(context);
	if (status!=0)
		std::cerr<<"ERROR releasing objects: "<<status<<std::endl;


	//stop clock	
	t2 = clock();
	double diff = (((double)t2 - (double)t1)/CLOCKS_PER_SEC);
	//time info out
	std::cout<< "It took "<< diff <<" second(s)."<< std::endl;

	return status;
}

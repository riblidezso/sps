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

	//copy (and there is type conversion too) small data, that wont be modified 
	time=std::vector<cl_float>(model.time.begin(),model.time.end());
	mes_spec=std::vector<cl_float>(model.mes_spec.begin(),model.mes_spec.end());
	mes_spec_wavel=std::vector<cl_float>(model.mes_spec_wavel.begin(),model.mes_spec_wavel.end());
	mes_spec_mask=std::vector<cl_float>(model.mes_spec_mask.begin(),model.mes_spec_mask.end());
	mes_spec_err=std::vector<cl_float>(model.mes_spec_err_h.begin(),model.mes_spec_err_h.end());

	//resize the data structures to mes_nspecteps
	//that will be computed at every wavelength
	factor1.resize(mes_nspecsteps);		 
	factor2.resize(mes_nspecsteps);		
	chis.resize(mes_nspecsteps);
	result.resize(mes_nspecsteps);

	//imf, if you want to use salpeter
	//uncomment that line and comment chabrier
	//TODO -> config file!!
	imf="chabrier";
	//imf="salpeter";

}


 
//gets original models  from read object
//and resamples all the models to the wavelengths of
//the measurement
int opencl_fit_w_err::resample_models_2_mes(sps_read& model)
{
	//calculate model offset from imf
	// 0-5 is chabrier, 6-11 is salpeter
	// this is not very elegant
	int offset;
	if( imf == "chabrier")
		offset=0;
	else if(imf=="salpeter")
		offset=6;

	//resize the vector that will store the resampled model
	//this is a huge contigous vector because this makes it
	//easy to pass it to GPU (only have to give pointer and size)
	resampled_model.resize(ntimesteps * mes_nspecsteps * 6);

	//wd, wu are weigths for interpolation, delta is wavelength distance
	double wd,wu,delta;

	//these will store temporary positions
	int low,high,place,place1;
	high=0;



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
			//there might be problems if the one measured data wavelength is 
			//larger than last model data, but this seems unlikely
			if (high==nspecsteps)
			{
				std::cerr<<"\nERROR measurement wavelength"<<mes_spec_wavel[i]<<"\n";
				std::cerr<<"can't be interpolated from model wavelength points\n\n";
				return 1;
			}
		}
			
		//there might be problems if the first measured wavelength is 
		//smaller than first model data, but this seems also unlikely
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
				//calculate positions in the big continous models vector
				place=mes_nspecsteps*ntimesteps*k + mes_nspecsteps*j;
				place1=nspecsteps*ntimesteps*k + nspecsteps*j;
				//interpolation
				resampled_model[place+i]=wd*model.model_cont[low+place1]+wu*model.model_cont[high+place1];
			}
		}
	}
	return 0;
}


// load the kernel file into a null terminated string 
int opencl_fit_w_err::convertToString(std::string infilename, std::string& str)
{
	//open file in binary i/o 
	std::ifstream infile(infilename.c_str(), std::ios::binary |std::ios::ate); 
	//check file 
	if(!(infile)) 
	{ 
		std::cout<<"\nERROR CAN'T OPEN KERNEL FILE: "<<infilename<<"\n"<<std::endl; 
		return 1; 
	} 

	//get the size of the file 
	std::ifstream::pos_type size;							 
	size = infile.tellg();	 
	//go to the begginging of file								 
	infile.seekg (0, std::ios::beg); 


	//read file
	str.resize(size);
	infile.read((char*)(str.c_str()), size);
	//append "\0"
	str+='\0';

	return 0;
}

//chooses opencl platfrom, and device
//creates context, commandque
//loads kernel files, and builds them
int opencl_fit_w_err::opencl_initialize(std::string kernel_filename)
{
	//error variable
	cl_int	status=0;

	//Getting OpenCL platforms and choose an available one.
	cl_uint numPlatforms;				//the NO. of platforms
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


			//get platform version size
			size_t platform_version_size;
			status = clGetPlatformInfo( platforms[i], CL_PLATFORM_VERSION, 0,NULL,&platform_version_size);
	
			//get platform version
			char* platform_version = new char[platform_version_size];
			status = clGetPlatformInfo( platforms[i], CL_PLATFORM_VERSION , platform_version_size, platform_version,NULL);


			//print info
			std::cout<<i<<". platform:\t"<<platform_name<<"\n";
			std::cout<<i<<". version:\t "<<platform_version<<"\n";
			std::cout<<std::endl;

			delete[] platform_name;		
			delete[] platform_version;		
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

	//Query the platform and choose the  device
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


		//get device version size
		size_t device_version_size;
		status = clGetDeviceInfo( devices[i], CL_DEVICE_VERSION, 0,NULL,&device_version_size);

		//get device version
		char* device_version = new char[device_version_size];
		status = clGetDeviceInfo( devices[i], CL_DEVICE_VERSION , device_version_size, device_version,NULL);


		//print info
		std::cout<<i<<". device vendor:\t"<<device_vendor<<std::endl;
		std::cout<<"            name:\t"<<device_name<<std::endl;

		//device type 
		if( device_type == CL_DEVICE_TYPE_CPU )
			std::cout<<"            type:\tCPU"<<std::endl;
		if( device_type == CL_DEVICE_TYPE_GPU )
			std::cout<<"            type:\tGPU"<<std::endl;
		if( device_type == CL_DEVICE_TYPE_ACCELERATOR )
			std::cout<<"            type:\tACCELERATOR"<<std::endl;
		if( device_type == CL_DEVICE_TYPE_DEFAULT)
			std::cout<<"            type:\tDEFAULT"<<std::endl;


		std::cout<<"            version:\t"<<device_version<<std::endl;

		delete[] device_vendor;
		delete[] device_name;
		delete[] device_version;
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
	//(it is started after all the interactive steps ended)
	t1 = clock();

	//Create context
	context = clCreateContext(NULL,numDevices, devices,NULL,NULL,&status);
	if (status!=0)
	{
		std::cerr<<"ERROR creating context: "<<status<<std::endl;
		return status;
	}
 
	//Creating command queue associated with the context
	commandQueue = clCreateCommandQueue(context, device, 0, &status);
	if (status!=0)
	{
		std::cerr<<"ERROR creating commandqueue: "<<status<<std::endl;
		return status;
	}

	//open kernel file and convert it to char array
	//const char *filename = kernel_filename.c_str();
	std::string sourceStr;
	status = convertToString(kernel_filename, sourceStr);
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
	//only prints log if there was an error
	//if there are only warnings it is not printed
	status=clBuildProgram(program,numDevices,devices,NULL,NULL,NULL);
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

//allocates buffers on devices
//and creates kernel objects
int opencl_fit_w_err::opencl_kern_mem()
{
	//error variable
	cl_int status=0;
	
	// Allocate memory on device 
	
	//data to read
	resampled_model_d = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, resampled_model.size() * sizeof(cl_float),(void *) resampled_model.data(), &status);
	//error check
	if (status!=0)
	{
		std::cerr<<"ERROR creating buffers: "<<status<<std::endl;
		return status;
	}
	time_d = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, ntimesteps * sizeof(cl_float),(void *) time.data(), &status);
	//error check
	if (status!=0)
	{
		std::cerr<<"ERROR creating buffers: "<<status<<std::endl;
		return status;
	}
	wavel_d = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, mes_nspecsteps * sizeof(cl_float),(void *) mes_spec_wavel.data(), &status);
	//error check
	if (status!=0)
	{
		std::cerr<<"ERROR creating buffers: "<<status<<std::endl;
		return status;
	}
	mes_spec_d = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, mes_nspecsteps * sizeof(cl_float),(void *) mes_spec.data(), &status);
	//error check
	if (status!=0)
	{
		std::cerr<<"ERROR creating buffers: "<<status<<std::endl;
		return status;
	}
	mes_spec_err_d = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, mes_nspecsteps * sizeof(cl_float),(void *) mes_spec_err.data(), &status);
	//error check
	if (status!=0)
	{
		std::cerr<<"ERROR creating buffers: "<<status<<std::endl;
		return status;
	}
	mes_spec_mask_d = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, mes_nspecsteps * sizeof(cl_float),(void *) mes_spec_mask.data(), &status);
	//error check
	if (status!=0)
	{
		std::cerr<<"ERROR creating buffers: "<<status<<std::endl;
		return status;
	}
	

	//buffers to read and write
	model_d = clCreateBuffer(context, CL_MEM_READ_WRITE,	sizeof(cl_float) * ntimesteps * mes_nspecsteps , NULL, &status);
	//error check
	if (status!=0)
	{
		std::cerr<<"ERROR creating buffers: "<<status<<std::endl;
		return status;
	}
	result_no_vel_d = clCreateBuffer(context, CL_MEM_READ_WRITE,	sizeof(cl_float) * mes_nspecsteps , NULL, &status);
	//error check
	if (status!=0)
	{
		std::cerr<<"ERROR creating buffers: "<<status<<std::endl;
		return status;
	}
	result_d = clCreateBuffer(context, CL_MEM_READ_WRITE,	sizeof(cl_float) * mes_nspecsteps , NULL, &status);
	//error check
	if (status!=0)
	{
		std::cerr<<"ERROR creating buffers: "<<status<<std::endl;
		return status;
	}
	factor1_d = clCreateBuffer(context, CL_MEM_WRITE_ONLY,	sizeof(cl_float) * mes_nspecsteps , NULL, &status);
	//error check
	if (status!=0)
	{
		std::cerr<<"ERROR creating buffers: "<<status<<std::endl;
		return status;
	}
	factor2_d = clCreateBuffer(context, CL_MEM_WRITE_ONLY,	sizeof(cl_float) * mes_nspecsteps , NULL, &status);
	//error check
	if (status!=0)
	{
		std::cerr<<"ERROR creating buffers: "<<status<<std::endl;
		return status;
	}
	chi_d = clCreateBuffer(context,  CL_MEM_WRITE_ONLY,		sizeof(cl_float) * mes_nspecsteps , NULL, &status);		
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
//with the buffers on the GPU (device)
//this need not be done again, even if
//data changes the kernel will read the new data
int opencl_fit_w_err::set_kern_arg()
{
	//error variable
	cl_int status=0;

	//not all kernel arguments are set now
	//some will be changed/calculated later
	//those will be set later

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
	//error check
	if (status!=0)
	{
		std::cerr<<"ERROR setting kernel spec_gen arguments: "<<status<<std::endl;
		return status;
	}


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
	//error check
	if (status!=0)
	{
		std::cerr<<"ERROR setting kernel vel_disp arguments: "<<status<<std::endl;
		return status;
	}

	//kernel_chi_calc 
	status = clSetKernelArg(kernel_chi_calc, 0, sizeof(cl_mem), &mes_spec_d);
	status |= clSetKernelArg(kernel_chi_calc, 1, sizeof(cl_mem), &mes_spec_err_d);
	status |= clSetKernelArg(kernel_chi_calc, 2, sizeof(cl_mem), &mes_spec_mask_d);
	status |= clSetKernelArg(kernel_chi_calc, 3, sizeof(cl_mem), &result_d);
	status |= clSetKernelArg(kernel_chi_calc, 4, sizeof(cl_mem), &chi_d);
	status |= clSetKernelArg(kernel_chi_calc, 6, sizeof(cl_mem), &wavel_d);
	//error check
	if (status!=0)
	{
		std::cerr<<"ERROR setting kernel chi calc arguments: "<<status<<std::endl;
		return status;
	}


	return status;
}

//set params (also type conversion if needed)
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
	//error variable
	cl_int status=0;

	//this part finds the metallicity models
	//nearest to metall value, these will be used
	//at the interpolation step in the kernel
	int offset;
	if( imf == "chabrier")
		offset=0;
	else if(imf == "salpeter")
		offset=6;

	//metallicity of models:
	double model_metal[6]={0.0001,0.0004,0.004,0.008,0.02,0.05};

	//the index of the model wich is lower than metall
	//this will be passed a the kernel:
	int modelno;

	//find modelno
	for(int j=0;j<6;j++)
	{
		if(model_metal[j] > metall)
		{
			modelno=j-1+offset;
			break;
		}
	}


	//set  the kernel arguments that has changed
	//TODO (ready reached max no of constant arguments?)

	//kernel spec_gen
	status = clSetKernelArg(kernel_spec_gen, 12, sizeof(cl_float), &dust_tau_v);
	status |= clSetKernelArg(kernel_spec_gen, 13, sizeof(cl_float), &dust_mu);
	status |= clSetKernelArg(kernel_spec_gen, 14, sizeof(cl_float), &sfr_tau);
	status |= clSetKernelArg(kernel_spec_gen, 15, sizeof(cl_float), &age);
	status |= clSetKernelArg(kernel_spec_gen, 16, sizeof(cl_float), &metall);
	status |= clSetKernelArg(kernel_spec_gen, 17, sizeof(int), &modelno);
	if (status!=0)
	{
		std::cerr<<"ERROR setting kernel_spec_gen arguments: "<<status<<std::endl;
		return status;
	}

	//kernel vel_disp
	status = clSetKernelArg(kernel_vel_disp, 9, sizeof(cl_float), &vdisp);
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
	cl_float temp_1,temp_2,factor;

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
	status = clEnqueueReadBuffer(commandQueue, factor1_d, CL_TRUE, 0, mes_nspecsteps * sizeof(cl_float) , factor1.data(), 0, NULL,NULL);
	if (status!=0)
	{
		std::cerr<<"ERROR reading buffer,from kernel_vel: "<<status<<std::endl;
		return status;
	}
	status = clEnqueueReadBuffer(commandQueue, factor2_d, CL_TRUE, 0, mes_nspecsteps * sizeof(cl_float) , factor2.data(), 0, NULL, NULL);
	if (status!=0)
	{
		std::cerr<<"ERROR reading buffer,from kernel_vel: "<<status<<std::endl;
		return status;
	}

	//summing factors, to pull spectra together
	//summing is sequential on CPU
	//paralell summing would an overkill for 3-4000 no i guess
	temp_1=0;
	temp_2=0;
	for(int i=0;i<mes_nspecsteps;i++)
	{
		temp_1+=factor1[i];
		temp_2+=factor2[i];
	}
	factor=temp_1/temp_2;


	//next kernel for chi calculation
	//Set kernel_chi_calc argument
	status |= clSetKernelArg(kernel_chi_calc, 5, sizeof(cl_float), &factor);
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
	//now we dont read back the fitted spectrum
	//only when it is the best so far
	status = clEnqueueReadBuffer(commandQueue, chi_d, CL_TRUE, 0, mes_nspecsteps * sizeof(cl_float) , chis.data(), 0,NULL,NULL);
	if (status!=0)
	{
		std::cerr<<"ERROR reading chi buffer: "<<status<<std::endl;
		return status;
	}

	//summing chis
	chi=0;
	for(int i=0;i<mes_nspecsteps;i++)
		chi+=chis[i];

	return status;
}

//read the result modeled spectrum, when the chi is the lowest
int opencl_fit_w_err::read_best_result()
{
	//error
	cl_int status=0;

	//read from GPU (device)
	status = clEnqueueReadBuffer(commandQueue, result_d, CL_TRUE, 0, mes_nspecsteps * sizeof(cl_float) , result.data(), 0, NULL, NULL);
	//error check
	if (status!=0)
		std::cout<<"ERROR reading buffer: "<<status<<std::endl;

	return status;	
}

int opencl_fit_w_err::write_fit_result()
{
	//type conversion for the simple write_table function i wrote
	std::vector<std::vector <double> > output;
	output.push_back(std::vector<double>(mes_spec_wavel.begin(),mes_spec_wavel.end()));
	output.push_back(std::vector<double>(mes_spec.begin(),mes_spec.end()));
	output.push_back(std::vector<double>(mes_spec_err.begin(),mes_spec_err.end()));
	output.push_back(std::vector<double>(mes_spec_mask.begin(),mes_spec_mask.end()));
	output.push_back(std::vector<double>(result.begin(),result.end()));

	//table writing function from sps_write 
	write_table_col(output,"../output/fit.dat");

	//info out
	std::cout<<"fit writing succesful: sps/output/fit.dat"<<std::endl;
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

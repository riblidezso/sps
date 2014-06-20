#include "opencl_fit1.h"
#include "read.h"
#include "write.h"

opencl_fit1::opencl_fit1(read& model)
{
	nspecsteps=model.wavelengths.size();
	ntimesteps=model.time.size();

	factor1.resize(nspecsteps);		 
	factor2.resize(nspecsteps);		
	chis.resize(nspecsteps);

	result.resize(nspecsteps);

	chi_before=DBL_MAX;
	best_chi=DBL_MAX;

	imf="chabrier";//!!!!!!!!!!!!!!!!!
}


//function from ATI SDK
/* convert the kernel file into a string */
int opencl_fit1::convertToString(const char *filename, std::string& s)
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
	std::cout<<"Error: failed to open file\n:"<<filename<<std::endl;
	return 1;
}

int opencl_fit1::opencl_start(std::string kernel_filename)
{
	cl_int	status;
	/*Step1: Getting platforms and choose an available one.*/
	cl_uint numPlatforms;				//the NO. of platforms
	cl_platform_id* platforms = NULL; 	//id of available platforms
	cl_platform_id 	platform = NULL;	//id of the chosen platform


	//getting NO. of platforms
	status = clGetPlatformIDs(0, NULL, &numPlatforms); 
	if (status != CL_SUCCESS)
	{
		std::cout << "Error: Getting platforms!" << std::endl;
		return 1;
	}

	/*Choosing platform*/
	if(numPlatforms > 0)
	{
		/*getting platform ids*/
		platforms =  new cl_platform_id[numPlatforms];
		status = clGetPlatformIDs(numPlatforms, platforms, NULL);

		/*printing platform names*/
		std::cout<<"\nPlatform info:"<<std::endl;
		for(unsigned int i=0;i<numPlatforms;i++)
		{
			size_t platform_name_size;
			status = clGetPlatformInfo( platforms[i] , CL_PLATFORM_NAME ,0, NULL, &platform_name_size);
			char* platform_name = new char[platform_name_size];
			status = clGetPlatformInfo( platforms[i] , CL_PLATFORM_NAME ,platform_name_size, platform_name,NULL);
			std::cout<<i<<". platform: "<<platform_name<<"\n";
			free(platform_name);
		}

		/*choosing platform*/
/*		std::cout<<"\nChoose platform: (type the number)"<<std::endl;
		int platform_choice;
		std::string temp_line;
		getline(std::cin,temp_line);
		std::stringstream temp_sstr;
		temp_sstr<<temp_line;
		temp_sstr>>platform_choice;
		platform = platforms[platform_choice];
*/
		platform=platforms[1];
		delete[] platforms;		
	}

	/*Step 2:Query the platform and choose the  device.*/

	cl_uint		numDevices = 0; 	/*NO. of devices*/
	cl_device_id	*devices;		/* device ids */
	cl_device_id	device;			/*id of chosen device*/

	/*getting number of devices*/
	status = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 0, NULL, &numDevices);	
	devices = new cl_device_id[numDevices];

	/*getting device ids*/
	status = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, numDevices, devices, NULL);
	
	/*printing device info*/
	std::cout<<"\nDevice info:"<<std::endl;
	for (unsigned int i=0;i<numDevices;i++)
	{
		size_t device_vendor_size;
		status = clGetDeviceInfo( devices[i], CL_DEVICE_VENDOR, 0,NULL,&device_vendor_size);
		char* device_vendor = new char[device_vendor_size];
		status = clGetDeviceInfo( devices[i], CL_DEVICE_VENDOR, device_vendor_size,device_vendor,NULL);

		size_t device_name_size;
		status = clGetDeviceInfo( devices[i], CL_DEVICE_NAME, 0,NULL,&device_name_size);
		char* device_name = new char[device_name_size];
		status = clGetDeviceInfo( devices[i], CL_DEVICE_NAME, device_name_size,device_name,NULL);

		cl_device_type device_type;
		status = clGetDeviceInfo( devices[i], CL_DEVICE_TYPE, sizeof(cl_device_type),&device_type,NULL);

		std::cout<<i<<". device vendor: "<<device_vendor<<std::endl;
		std::cout<<"            name:"<<device_name<<std::endl;

		if( device_type == CL_DEVICE_TYPE_CPU )
			std::cout<<"            type: CPU"<<std::endl;
		if( device_type == CL_DEVICE_TYPE_GPU )
			std::cout<<"            type: GPU"<<std::endl;
		if( device_type == CL_DEVICE_TYPE_ACCELERATOR )
			std::cout<<"            type: ACCELERATOR"<<std::endl;
		if( device_type == CL_DEVICE_TYPE_DEFAULT)
			std::cout<<"            type: DEFAULT"<<std::endl;

		free(device_vendor);
		free(device_name);
	}

	/*choosing device*/
/*	std::cout<<"\nChoose device: (type the number)"<<std::endl;
	int device_choice;
	std::string temp_line1;
	getline(std::cin,temp_line1);
	std::stringstream temp_sstr1;
	temp_sstr1<<temp_line1;
	temp_sstr1>>device_choice;
	device = devices[device_choice];
*/
	device=devices[0];

	//starting clock
	t1 = clock();

	/*Step 3: Create context.*/
	context = clCreateContext(NULL,1, devices,NULL,NULL,&status);
	if (status!=0)
	{
		std::cout<<"ERROR creating context: "<<status<<std::endl;
	}
	 
	/*Step 4: Creating command queue associate with the context.*/
	commandQueue = clCreateCommandQueue(context, device, 0, &status);
	if (status!=0)
	{
		std::cout<<"ERROR creating commandqueue: "<<status<<std::endl;
	}

	/*Step 5: Create program object */
	const char *filename = kernel_filename.c_str();
	std::string sourceStr;
	status = convertToString(filename, sourceStr);
	const char *source = sourceStr.c_str();
	size_t sourceSize[] = {strlen(source)};
	program = clCreateProgramWithSource(context, 1, &source, sourceSize, &status);
	if (status!=0)
	{
		std::cout<<"ERROR creating program: "<<status<<std::endl;
	}	

	/*Step 6: Build program. */
	status=clBuildProgram(program, 1,devices,NULL,NULL,NULL);
	if (status!=0)
	{
		std::cout<<"ERROR building program: "<<status<<std::endl;

		//Getting build log
		size_t logsize=0;
		clGetProgramBuildInfo(program,device,CL_PROGRAM_BUILD_LOG,0,NULL,&logsize);
		std::cout<<logsize<<std::endl;
		char* log;
		log = new char[logsize];	
		clGetProgramBuildInfo(program,device,CL_PROGRAM_BUILD_LOG,logsize,log,NULL);
		std::cout<<"log:\n "<<log<<std::endl;
		delete[] log;
	}

	

}

int opencl_fit1::opencl_spec(read& model)
{
	int status=0;

	int nspecsteps=model.wavelengths.size();
	int ntimesteps=model.time.size();
	
	/*Step 7: Allocate memory */
	
	//data
	model_without_dust_d = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, model.model_cont.size() * sizeof(double),(void *) model.model_cont.data(), &status);
	time_d = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, ntimesteps * sizeof(double),(void *) model.time.data(), &status);
	wavel_d = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, nspecsteps * sizeof(double),(void *) model.wavelengths.data(), &status);
	sample_spec_d = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, nspecsteps * sizeof(double),(void *) model.sample_spec.data(), &status);
	
	//buffers to write
	model_d = clCreateBuffer(context, CL_MEM_READ_WRITE,	sizeof(double) * ntimesteps * nspecsteps , NULL, &status);
	result_d = clCreateBuffer(context, CL_MEM_READ_WRITE,	sizeof(double) * nspecsteps , NULL, &status);
	factor1_d = clCreateBuffer(context, CL_MEM_WRITE_ONLY,	sizeof(double) * nspecsteps , NULL, &status);
	factor2_d = clCreateBuffer(context, CL_MEM_WRITE_ONLY,	sizeof(double) * nspecsteps , NULL, &status);
	chi_d = clCreateBuffer(context,  CL_MEM_WRITE_ONLY,		sizeof(double) * nspecsteps , NULL, &status);		
	if (status!=0)
	{
		std::cout<<"ERROR creating buffers: "<<status<<std::endl;
	}

	/*Step 8: Create kernel objects */
	kernel = clCreateKernel(program,"fit_dp_a", &status);
	if (status!=0)
	{
		std::cout<<"ERROR creating kernel: "<<status<<std::endl;
	}
	kernel2 = clCreateKernel(program,"fit_dp_b", &status);
	if (status!=0)
	{
		std::cout<<"ERROR creating kernel2: "<<status<<std::endl;
	}

	return status;
}

int opencl_fit1::set_kern_arg()
{
	/*Setting kernel arguments*/
	int status=0;

	//kernel1
	status = clSetKernelArg(kernel, 0, sizeof(cl_mem), &model_without_dust_d);
	status |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &time_d);
	status |= clSetKernelArg(kernel, 2, sizeof(cl_mem), &wavel_d);
	status |= clSetKernelArg(kernel, 3, sizeof(cl_mem), &sample_spec_d);
		
	status |= clSetKernelArg(kernel, 4, sizeof(cl_mem), &model_d);
	status |= clSetKernelArg(kernel, 5, sizeof(cl_mem), &result_d);
	status |= clSetKernelArg(kernel, 6, sizeof(cl_mem), &factor1_d);
	status |= clSetKernelArg(kernel, 7, sizeof(cl_mem), &factor2_d);
	
	status |= clSetKernelArg(kernel, 8, sizeof(int), &nspecsteps);
	status |= clSetKernelArg(kernel, 9, sizeof(int), &ntimesteps);

	//kernel2 
	status |= clSetKernelArg(kernel2, 0, sizeof(cl_mem), &sample_spec_d);
	status |= clSetKernelArg(kernel2, 1, sizeof(cl_mem), &result_d);
	status |= clSetKernelArg(kernel2, 2, sizeof(cl_mem), &chi_d);

	if (status!=0)
		std::cout<<"ERROR setting kernel arguments: "<<status<<std::endl;

	return status;
}

int opencl_fit1::set_initial_params(double s_dust_tau_v,
									double s_dust_mu,
									double s_sfr_tau,
									double s_age,
									double s_metall)
{

	dust_tau_v=s_dust_tau_v;
	dust_mu=s_dust_mu;
	sfr_tau=s_sfr_tau;
	age=s_age;
	metall=s_metall;

	return 0;
}



int opencl_fit1::change_params()
{
	int status=0;
	int sign;

	int fac=50;
	if (best_chi<1)
	{
		fac=200;
	}

	do{
		status=0;
		//creating random jump
		sign= ( (rand() % 2) * 2 ) -1;
		d_dust_tau_v=sign * dust_tau_v * log(1+ rand() / (fac * double(RAND_MAX)));
		sign= ( (rand() % 2) * 2 ) -1;
		d_dust_mu=sign * dust_mu* log(1+ rand() / (fac * double(RAND_MAX))) ;
		sign= ( (rand() % 2) * 2 ) -1;
		d_sfr_tau=sign * sfr_tau*  log(1+ rand() / (fac  * double(RAND_MAX)));
		sign= ( (rand() % 2) * 2 ) -1;
		d_age=sign * age*  log(1+  rand() / (fac  * double(RAND_MAX)));
		sign= ( (rand() % 2) * 2 ) -1;
		d_metall=sign * metall * log (1+ rand() / ( fac * double(RAND_MAX))) ;
		sign= ( (rand() % 2) * 2 ) -1;
		d_age=sign * age*  log(1+  rand() / (fac  * double(RAND_MAX)));

		/*Step 9: Set Kernel arguments.*/
		dust_tau_v+=d_dust_tau_v;
		dust_mu+=d_dust_mu;
		sfr_tau+=d_sfr_tau;
		age+=d_age;
		metall+=d_metall;

		if(dust_mu <=0.1 || dust_mu >1 || dust_tau_v <0.05 || dust_tau_v>2
			|| age<1e+9 || age>2e+10 ||sfr_tau>20e+10 
			|| sfr_tau<=1e+8 || metall<0.0001 || metall>0.05)
			
		{
			dust_tau_v-=d_dust_tau_v;
			dust_mu-=d_dust_mu;
			sfr_tau-=d_sfr_tau;
			age-=d_age;
			metall-=d_metall;

			status=1;
		}
		
	}while(status==1);

	int offset;
	if( imf == "chabrier")
		offset=0;
	else if(imf=="salpeter")
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

	
//already reached max no of constant arguments...
	status = clSetKernelArg(kernel, 10, sizeof(double), &dust_tau_v);
	status |= clSetKernelArg(kernel, 11, sizeof(double), &dust_mu);
	status |= clSetKernelArg(kernel, 12, sizeof(double), &sfr_tau);
	status |= clSetKernelArg(kernel, 13, sizeof(double), &age);
	status |= clSetKernelArg(kernel, 14, sizeof(double), &metall);
	status |= clSetKernelArg(kernel, 15, sizeof(int), &modelno);
	if (status!=0)
	{
		std::cout<<"ERROR setting kernel arguments: "<<status<<std::endl;
		return status;
	}

	return status;
}

int opencl_fit1::call_kernels()
{
	cl_int status=0;
	double temp_1,temp_2,factor,chi;

//	cl_event kernel_event[1];
	/*Step 10: Running the kernel.*/
	size_t global_work_size[1] = {nspecsteps};
	status = clEnqueueNDRangeKernel(commandQueue, kernel, 1, NULL, global_work_size, NULL, 0, NULL,NULL);// &kernel_event[0]);
	if (status!=0)
		std::cout<<"ERROR running kernel: "<<status<<std::endl;

	/*Step 11: Read the result back to host memory.*/
	status = clEnqueueReadBuffer(commandQueue, factor1_d, CL_TRUE, 0, nspecsteps * sizeof(double) , factor1.data(), 0, NULL,NULL); // kernel_event, NULL);
	status = clEnqueueReadBuffer(commandQueue, factor2_d, CL_TRUE, 0, nspecsteps * sizeof(double) , factor2.data(), 0, NULL, NULL);
	if (status!=0)
	{
		std::cout<<"ERROR reading buffer: "<<status<<std::endl;
		return 1;
	}

	//summing factors
	temp_1=0;
	temp_2=0;
	for(int i=0;i<nspecsteps;i++)
	{
		temp_1+=factor1[i];
		temp_2+=factor2[i];
	}
	factor=temp_1/temp_2;


	//next kernel for chi

	/* Set kernel2 argument.*/
	status |= clSetKernelArg(kernel2, 3, sizeof(double), &factor);
	if (status!=0)
	{
		std::cout<<"ERROR setting kernel2 arguments: "<<status<<std::endl;
		return 1;
	}

	/*Step 10: Running the kernel.*/
	//	size_t global_work_size[1] = {nspecsteps};
	status = clEnqueueNDRangeKernel(commandQueue, kernel2, 1, NULL, global_work_size, NULL, 0, NULL, NULL); //&kernel_event[0]);
	if (status!=0)
	{
		std::cout<<"ERROR running kernel2: "<<status<<std::endl;
		return 1;
	}	

	/*Step 11: Read the result back to host memory.*/
	status = clEnqueueReadBuffer(commandQueue, chi_d, CL_TRUE, 0, nspecsteps * sizeof(double) , chis.data(), 0,NULL,NULL);// kernel_event, NULL);
	if (status!=0)
	{
		std::cout<<"ERROR reading buffer: "<<status<<std::endl;
		return 1;
	}

	
	return status;
}

int opencl_fit1::evaluate_chi()
{
	int status=0;

	//summing chisquare
	double chi=0;
	for(int i=0;i<nspecsteps;i++)
	{
		chi+=chis[i];
	}

	out_chi_evol.push_back(chi);

	//if no better than before
	if(chi_before < chi)
	{
		double limit=exp(chi_before-chi) * RAND_MAX;
		double rand_num=rand();

		if(rand_num > -1 )//limit )
		{
			//rejection
			dust_tau_v-=d_dust_tau_v;
			dust_mu-=d_dust_mu;
			sfr_tau-=d_sfr_tau;
			age-=d_age;
			metall-=d_metall;
			out_acc_chi_evol.push_back(0);
			out_worse_acc_chi_evol.push_back(10);
		}
		else
		{
			chi_before=chi;
			out_acc_chi_evol.push_back(chi);
			out_worse_acc_chi_evol.push_back(chi);
		}
	}
	else if (best_chi > chi )
	{
		best_chi=chi;
		
		best_dust_tau_v=dust_tau_v;
		best_dust_mu=dust_mu;
		best_sfr_tau=sfr_tau;
		best_age=age;
		best_metall=metall;

		chi_before=chi;
					
		status = clEnqueueReadBuffer(commandQueue, result_d, CL_TRUE, 0, nspecsteps * sizeof(double) , result.data(), 0, NULL, NULL);
		if (status!=0)
		{
			std::cout<<"ERROR reading buffer: "<<status<<std::endl;
		}
		out_acc_chi_evol.push_back(chi);
		out_worse_acc_chi_evol.push_back(0);
	}
	else 
	{
		out_acc_chi_evol.push_back(chi);
		out_worse_acc_chi_evol.push_back(0);
	}


	out_best_chi_evol.push_back(best_chi);

	return status;
}

int opencl_fit1::write_results(read& model)
{
	std::vector<std::vector <double> > output;
	output.push_back(model.wavelengths);
	output.push_back(model.sample_spec);
	output.push_back(result);

	write_table_col(output,"../output/fit.txt");
	write_vector(out_chi_evol,"../output/chi_evol.txt");
	write_vector(out_best_chi_evol,"../output/best_chi_evol.txt");
	write_vector(out_acc_chi_evol,"../output/acc_chi_evol.txt");
	write_vector(out_worse_acc_chi_evol,"../output/worse_acc_chi_evol.txt");
	
	std::cout<<"\nbest params:\n\n";
	std::cout<<"dust_tau_v="<<best_dust_tau_v<<std::endl;
	std::cout<<"dust_mu="<<best_dust_mu<<std::endl;
	std::cout<<"sfr_tau="<<best_sfr_tau<<std::endl;
	std::cout<<"age="<<best_age<<std::endl;
	std::cout<<"metall="<<best_metall<<std::endl;
	std::cout<<"chisquare="<<best_chi<<std::endl;

	return 0;
}

int opencl_fit1::clean_resources()
{
	int status=0;

	/*Step 12: Clean the resources.*/
	status = clReleaseKernel(kernel);				//Release kernels.
	status = clReleaseKernel(kernel2);	

	status = clReleaseProgram(program);				//Release the program object.

	status = clReleaseMemObject(model_without_dust_d); //Release mem objects.
	status = clReleaseMemObject(time_d);		
	status = clReleaseMemObject(wavel_d);
	status = clReleaseMemObject(sample_spec_d);
	status = clReleaseMemObject(result_d);
	status = clReleaseMemObject(model_d);	
	status = clReleaseMemObject(factor1_d);
	status = clReleaseMemObject(factor2_d);
	status = clReleaseMemObject(chi_d);

	status = clReleaseCommandQueue(commandQueue);	//Release  Command queue.
	status = clReleaseContext(context);				//Release context.
	if (status!=0)
		std::cout<<"ERROR releasing objects: "<<status<<std::endl;
	
	t2 = clock();
	double diff = (((double)t2 - (double)t1)/CLOCKS_PER_SEC);
	//some info out
	std::cout<< "It took "<< diff <<" second(s)."<< std::endl;

	return status;
}
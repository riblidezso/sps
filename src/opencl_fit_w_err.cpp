#include "opencl_fit_w_err.h"
#include "read.h"
#include "write.h"

#include <float.h>
#include <string.h>




opencl_fit_w_err::opencl_fit_w_err(read& model)
{
	nspecsteps=model.wavelengths.size();
	ntimesteps=model.time.size();
	mes_nspecsteps=model.mes_spec_wavel.size();

	time=model.time;
	mes_spec=model.mes_spec;
	mes_spec_wavel=model.mes_spec_wavel;
	mes_spec_mask=model.mes_spec_mask;
	mes_spec_err=model.mes_spec_err_h;

	factor1.resize(mes_nspecsteps);		 
	factor2.resize(mes_nspecsteps);		
	chis.resize(mes_nspecsteps);

	result.resize(mes_nspecsteps);

	temp_point.resize(5);

	chi_before=DBL_MAX;
	best_chi=DBL_MAX;

	imf="chabrier";//!!!!!!!!!!!!!!!!!

	sigma=0.008;

}

int opencl_fit_w_err::resample_mes(read& model)
{
	//resize model
	res_model.resize(ntimesteps * mes_nspecsteps * 6);

	double wd,wu,delta;
	int low, high,place,place1;
	high=0;

	//imf offset 
	int offset;
	if( imf == "chabrier")
		offset=0;
	else if(imf=="salpeter")
		offset=6;

	//resampling
	for(int i=0;i<mes_nspecsteps;i++)
	{
		//finding nearest points
		while(mes_spec_wavel[i] > model.wavelengths[high])
			high++;

		low=high-1;

		//weights
		delta=model.wavelengths[high] - model.wavelengths[low];
		wd= (model.wavelengths[high]-mes_spec_wavel[i])/delta;
		wu= (mes_spec_wavel[i]- model.wavelengths[low])/delta;

		for(int k=offset;k<offset+6;k++)
		{
			for(int j=0;j<ntimesteps;j++)
			{
				place=mes_nspecsteps*ntimesteps*k + mes_nspecsteps*j;
				place1=nspecsteps*ntimesteps*k + nspecsteps*j;
				res_model[place+i]=wd*model.model_cont[low+place1]+wu*model.model_cont[high+place1];
			}
		}
	}
	return 0;
}

//function from ATI SDK
/* convert the kernel file into a string */
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
	std::cout<<"Error: failed to open file\n:"<<filename<<std::endl;
	return 1;
}

int opencl_fit_w_err::opencl_start(std::string kernel_filename)
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
		std::cout<<"\nChoose platform: (type the number)"<<std::endl;
		int platform_choice;
		std::string temp_line;
		getline(std::cin,temp_line);
		std::stringstream temp_sstr;
		temp_sstr<<temp_line;
		temp_sstr>>platform_choice;
		platform = platforms[platform_choice];

//		platform=platforms[1];
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
	std::cout<<"\nChoose device: (type the number)"<<std::endl;
	int device_choice;
	std::string temp_line1;
	getline(std::cin,temp_line1);
	std::stringstream temp_sstr1;
	temp_sstr1<<temp_line1;
	temp_sstr1>>device_choice;
	device = devices[device_choice];

//	device=devices[0];

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

	
	return status;
}

int opencl_fit_w_err::opencl_kern_mem()
{
	int status=0;
	
	/*Step 7: Allocate memory */
	
	//data
	res_model_d = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, res_model.size() * sizeof(double),(void *) res_model.data(), &status);
	time_d = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, ntimesteps * sizeof(double),(void *) time.data(), &status);
	wavel_d = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, mes_nspecsteps * sizeof(double),(void *) mes_spec_wavel.data(), &status);
	mes_spec_d = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, mes_nspecsteps * sizeof(double),(void *) mes_spec.data(), &status);
	mes_spec_err_d = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, mes_nspecsteps * sizeof(double),(void *) mes_spec_err.data(), &status);
	mes_spec_mask_d = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, mes_nspecsteps * sizeof(double),(void *) mes_spec_mask.data(), &status);
	


	//buffers to write
	model_d = clCreateBuffer(context, CL_MEM_READ_WRITE,	sizeof(double) * ntimesteps * mes_nspecsteps , NULL, &status);
	result_d = clCreateBuffer(context, CL_MEM_READ_WRITE,	sizeof(double) * mes_nspecsteps , NULL, &status);
	factor1_d = clCreateBuffer(context, CL_MEM_WRITE_ONLY,	sizeof(double) * mes_nspecsteps , NULL, &status);
	factor2_d = clCreateBuffer(context, CL_MEM_WRITE_ONLY,	sizeof(double) * mes_nspecsteps , NULL, &status);
	chi_d = clCreateBuffer(context,  CL_MEM_WRITE_ONLY,		sizeof(double) * mes_nspecsteps , NULL, &status);		
	if (status!=0)
	{
		std::cout<<"ERROR creating buffers: "<<status<<std::endl;
	}

	/*Step 8: Create kernel objects */
	kernel = clCreateKernel(program,"fit_w_err_1", &status);
	if (status!=0)
	{
		std::cout<<"ERROR creating kernel: "<<status<<std::endl;
	}
	kernel2 = clCreateKernel(program,"fit_w_err_2", &status);
	if (status!=0)
	{
		std::cout<<"ERROR creating kernel2: "<<status<<std::endl;
	}

	return status;
}

int opencl_fit_w_err::set_kern_arg()
{
	/*Setting kernel arguments*/
	int status=0;

	//kernel1
	status = clSetKernelArg(kernel, 0, sizeof(cl_mem), &res_model_d);
	status |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &time_d);
	status |= clSetKernelArg(kernel, 2, sizeof(cl_mem), &wavel_d);

	status |= clSetKernelArg(kernel, 3, sizeof(cl_mem), &mes_spec_d);
	status |= clSetKernelArg(kernel, 4, sizeof(cl_mem), &mes_spec_err_d);
	status |= clSetKernelArg(kernel, 5, sizeof(cl_mem), &mes_spec_mask_d);
		
	status |= clSetKernelArg(kernel, 6, sizeof(cl_mem), &model_d);
	status |= clSetKernelArg(kernel, 7, sizeof(cl_mem), &result_d);
	status |= clSetKernelArg(kernel, 8, sizeof(cl_mem), &factor1_d);
	status |= clSetKernelArg(kernel, 9, sizeof(cl_mem), &factor2_d);
	
	status |= clSetKernelArg(kernel, 10, sizeof(int), &mes_nspecsteps);
	status |= clSetKernelArg(kernel, 11, sizeof(int), &ntimesteps);

	//kernel2 
	status |= clSetKernelArg(kernel2, 0, sizeof(cl_mem), &mes_spec_d);
	status |= clSetKernelArg(kernel2, 1, sizeof(cl_mem), &mes_spec_err_d);
	status |= clSetKernelArg(kernel2, 2, sizeof(cl_mem), &mes_spec_mask_d);
	status |= clSetKernelArg(kernel2, 3, sizeof(cl_mem), &result_d);
	status |= clSetKernelArg(kernel2, 4, sizeof(cl_mem), &chi_d);
	status |= clSetKernelArg(kernel2, 6, sizeof(cl_mem), &wavel_d);
	


	if (status!=0)
		std::cout<<"ERROR setting kernel arguments: "<<status<<std::endl;

	return status;
}

int opencl_fit_w_err::set_initial_params(double s_dust_tau_v,
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



int opencl_fit_w_err::change_params(double opt_acc)
{
	int status=0;
	int sign;

	if(acc_ratio.size()>1 && iter%200==1 && iter>3 )
	{
		if (acc_ratio[acc_ratio.size()-1]>opt_acc && sigma < 0.5)
			sigma=sigma+sigma*(acc_ratio[acc_ratio.size()-1]-opt_acc);
		
		if (acc_ratio[acc_ratio.size()-1]<opt_acc)
		sigma=sigma-sigma*(opt_acc-acc_ratio[acc_ratio.size()-1]);

		std::cout<<"\n acc_rate="<<acc_ratio[acc_ratio.size()-1]<<"\n";
	}
	
	if(sigma<0)
	{
		std::cout<<"ERROR sigma < 0";
		return 1;
	}

	std::normal_distribution<double> distribution(0,sigma);

	do{
		status=0;

		//creating random jump
		d_dust_tau_v= dust_tau_v * distribution(generator);
		d_dust_mu= dust_mu * distribution(generator) ;
		d_sfr_tau= sfr_tau *  distribution(generator) ;
		d_age= age *  distribution(generator) ;
		d_metall= metall * distribution(generator) ;
		

		/*Step 9: Set Kernel arguments.*/
		dust_tau_v+=d_dust_tau_v;
		dust_mu+=d_dust_mu;
		sfr_tau+=d_sfr_tau;
		age+=d_age;
		metall+=d_metall;

		if(dust_mu <=0 || dust_mu >1 || dust_tau_v <0 || dust_tau_v>1.5
			|| age<1e+8 || age>2e+10 ||sfr_tau>40e+19
			|| sfr_tau<=1e+7 || metall<0.0001 || metall>0.05)
			
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
	status = clSetKernelArg(kernel, 12, sizeof(double), &dust_tau_v);
	status |= clSetKernelArg(kernel, 13, sizeof(double), &dust_mu);
	status |= clSetKernelArg(kernel, 14, sizeof(double), &sfr_tau);
	status |= clSetKernelArg(kernel, 15, sizeof(double), &age);
	status |= clSetKernelArg(kernel, 16, sizeof(double), &metall);
	status |= clSetKernelArg(kernel, 17, sizeof(int), &modelno);
	if (status!=0)
	{
		std::cout<<"ERROR setting kernel arguments: "<<status<<std::endl;
		return status;
	}

	return status;
}

int opencl_fit_w_err::call_kernels()
{
	cl_int status=0;
	double temp_1,temp_2,factor;

//	cl_event kernel_event[1];
	/*Step 10: Running the kernel.*/
	size_t global_work_size[1] = {mes_nspecsteps};
	status = clEnqueueNDRangeKernel(commandQueue, kernel, 1, NULL, global_work_size, NULL, 0, NULL,NULL);
	if (status!=0)
		std::cout<<"ERROR running kernel: "<<status<<std::endl;

	/*Step 11: Read the result back to host memory.*/
	status = clEnqueueReadBuffer(commandQueue, factor1_d, CL_TRUE, 0, mes_nspecsteps * sizeof(double) , factor1.data(), 0, NULL,NULL);
	status = clEnqueueReadBuffer(commandQueue, factor2_d, CL_TRUE, 0, mes_nspecsteps * sizeof(double) , factor2.data(), 0, NULL, NULL);
	if (status!=0)
	{
		std::cout<<"ERROR reading buffer: "<<status<<std::endl;
		return 1;
	}

	//summing factors
	temp_1=0;
	temp_2=0;
	for(int i=0;i<mes_nspecsteps;i++)
	{
		temp_1+=factor1[i];
		temp_2+=factor2[i];
	}
	factor=temp_1/temp_2;


	//next kernel for chi

	/* Set kernel2 argument.*/
	status |= clSetKernelArg(kernel2, 5, sizeof(double), &factor);
	if (status!=0)
	{
		std::cout<<"ERROR setting kernel2 arguments: "<<status<<std::endl;
		return 1;
	}

	/*Step 10: Running the kernel.*/
	//	size_t global_work_size[1] = {nspecsteps};
	status = clEnqueueNDRangeKernel(commandQueue, kernel2, 1, NULL, global_work_size, NULL, 0, NULL, NULL); 
	if (status!=0)
	{
		std::cout<<"ERROR running kernel2: "<<status<<std::endl;
		return 1;
	}	

	/*Step 11: Read the result back to host memory.*/
	status = clEnqueueReadBuffer(commandQueue, chi_d, CL_TRUE, 0, mes_nspecsteps * sizeof(double) , chis.data(), 0,NULL,NULL);
	if (status!=0)
	{
		std::cout<<"ERROR reading buffer: "<<status<<std::endl;
		return 1;
	}
	
	return status;
}

int opencl_fit_w_err::evaluate_chi(double temp)
{
	//summing chisquare
	chi=0;
	for(int i=0;i<mes_nspecsteps;i++)
	{
		chi+=chis[i];
	}

	//evaluating chi
	if (best_chi > chi)
	{
		accepted=0;
	}
	else if (chi_before>chi)
	{
		accepted=1;
	}
	else //if chi is no better than the one before
	{
		double limit=exp((chi_before-chi)/temp) * RAND_MAX;
		double rand_num=rand();

		if(rand_num > limit )
		{
			//rejection
			accepted=3;
		}
		else
		{
			//acceptance
			accepted=2;
		}
	}
	return 0;
}

int opencl_fit_w_err::record_data()
{
	int status=0;
	
	out_chi_evol.push_back(chi);
	out_best_chi_evol.push_back(best_chi);

	if(accepted==0 || accepted==1 || accepted==2  ) //step was accepted
	{
		chi_before=chi;
		out_acc_chi_evol.push_back(chi);
		acc.push_back(1);

		temp_point[0]=dust_tau_v;
		temp_point[1]=dust_mu;
		temp_point[2]=sfr_tau;
		temp_point[3]=age;
		temp_point[4]=metall;
		
	}
	else //not accepted
	{
		dust_tau_v-=d_dust_tau_v;
		dust_mu-=d_dust_mu;
		sfr_tau-=d_sfr_tau;
		age-=d_age;
		metall-=d_metall;
		out_acc_chi_evol.push_back(0);
		acc.push_back(0);
	}
	if(iter>5000)
	{
		points.push_back(temp_point); //!!!!!!!!!!!!!!
	}

	if (accepted==0) //the best chi 
	{
		best_chi=chi;
		best_dust_tau_v=dust_tau_v;
		best_dust_mu=dust_mu;
		best_sfr_tau=sfr_tau;
		best_age=age;
		best_metall=metall;
					
		status = clEnqueueReadBuffer(commandQueue, result_d, CL_TRUE, 0, mes_nspecsteps * sizeof(double) , result.data(), 0, NULL, NULL);
		if (status!=0)
		{
			std::cout<<"ERROR reading buffer: "<<status<<std::endl;
		}
	}

	if(accepted==2 || accepted==3) //worse step
	{
		worse.push_back(1);
	}
	else //better step
	{
		worse.push_back(0);
	}

	if(accepted==2) //worse but accepted
	{
		worse_acc.push_back(1);
	}
	if(accepted==3) //worse and not accepted
	{
		worse_acc.push_back(0);
	}

	//counting average values
	if(iter%200==0)
	{
		double mean=0;
		if (worse_acc.size()>100)
		{
			for(int i=0;i<100;i++)
			{
				mean+=worse_acc[worse_acc.size()-100+i];
			}
			worse_acc_ratio.push_back(mean/100);
		}

		mean=0;
		if (worse.size()>100)
		{
			for(int i=0;i<100;i++)
			{
				mean+=worse[worse.size()-100+i];
			}
			worse_rate.push_back(mean/100);
		}
	
		mean=0;
		if (acc.size()>100)
		{
			for(int i=0;i<100;i++)
			{
				mean+=acc[acc.size()-100+i];
			}
				
			acc_ratio.push_back(mean/100);
		}
	}

	return 0;
}


int opencl_fit_w_err::write_results()
{
	std::vector<std::vector <double> > output;
	output.push_back(mes_spec_wavel);
	output.push_back(mes_spec);
	output.push_back(mes_spec_err);
	output.push_back(result);

	write_table_col(output,"../output/fit.txt");
	write_table_row(points,"../output/points.dat");

	write_vector(out_chi_evol,"../output/chi_evol.txt");
	write_vector(out_best_chi_evol,"../output/best_chi_evol.txt");
	write_vector(out_acc_chi_evol,"../output/acc_chi_evol.txt");
	write_vector(worse_acc_ratio,"../output/worse-acc-rate.dat");
	write_vector(acc_ratio,"../output/acc-rate.dat");
	write_vector(worse_rate,"../output/worse-rate.dat");
	
	std::cout<<"\nbest params:\n\n";
	std::cout<<"dust_tau_v="<<best_dust_tau_v<<std::endl;
	std::cout<<"dust_mu="<<best_dust_mu<<std::endl;
	std::cout<<"sfr_tau="<<best_sfr_tau<<std::endl;
	std::cout<<"age="<<best_age<<std::endl;
	std::cout<<"metall="<<best_metall<<std::endl;
	std::cout<<"chisquare="<<best_chi<<std::endl;

	std::ofstream outfile("../output/fitted-params.dat");
	//Checking filename
	if(!(outfile))
	{
		std::cout<<"ERROR INVALID OUTPUT FILE: ../output/fitted-params.dat"<<std::endl;
		return 1;
	}

	outfile<<"best params:\n \n";
	outfile<<"dust_tau_v="<<best_dust_tau_v<<std::endl;
	outfile<<"dust_mu="<<best_dust_mu<<std::endl;
	outfile<<"sfr_tau="<<best_sfr_tau<<std::endl;
	outfile<<"age="<<best_age<<std::endl;
	outfile<<"metall="<<best_metall<<std::endl;
	outfile<<"log(P)="<<best_chi<<std::endl;
	
	outfile.close();

	//info out
	std::cout<<"writing succesful: "<<std::endl;
	return 0;
}

int opencl_fit_w_err::clean_resources()
{
	int status=0;

	/*Step 12: Clean the resources.*/
	status = clReleaseKernel(kernel);				//Release kernels.
	status = clReleaseKernel(kernel2);	

	status = clReleaseProgram(program);				//Release the program object.

	status = clReleaseMemObject(res_model_d); //Release mem objects.
	status = clReleaseMemObject(time_d);		
	status = clReleaseMemObject(wavel_d);

	status = clReleaseMemObject(mes_spec_d);
	status = clReleaseMemObject(mes_spec_err_d);
	status = clReleaseMemObject(mes_spec_mask_d);
	
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

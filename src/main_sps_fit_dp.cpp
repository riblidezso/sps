#include <iostream>
#include <sstream>
#include <ctime>

#include <CL/cl.h>

#include <string>
#include <string.h>

#define SUCCESS 0
#define FAILURE 1

#include "read.h"

//function from ATI SDK
/* convert the kernel file into a string */
int convertToString(const char *filename, std::string& s)
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
	return FAILURE;
}

int write_table_row(std::vector< std::vector<double > >& table_to_write, std::string outfilename)
{
	std::ofstream outfile(outfilename.c_str());
	//Checking filename
	if(!(outfile))
	{
		std::cout<<"ERROR INVALID OUTPUT FILE: "<<outfilename<<std::endl;
		return 1;
	}

	for(int j=0; j<table_to_write.size(); j++)
	{
		for(int k=0;k<table_to_write[j].size();k++)
		{
			outfile<<table_to_write[j][k]<<" ";
		}
		//end of lines
		outfile<<"\n";
	}
	outfile.close();

	//info out
	std::cout<<"writing succesful: "<<std::endl;

	return 0;
}
int write_table_col(std::vector< std::vector<double > >& table_to_write, std::string outfilename)
{
	std::ofstream outfile(outfilename.c_str());
	//Checking filename
	if(!(outfile))
	{
		std::cout<<"ERROR INVALID OUTPUT FILE: "<<outfilename<<std::endl;
		return 1;
	}

	for(int j=0; j<table_to_write[0].size(); j++)
	{
		for(int k=0;k<table_to_write.size();k++)
		{
			outfile<<table_to_write[k][j]<<" ";
		}
		//end of lines
		outfile<<"\n";
	}
	outfile.close();

	//info out
	std::cout<<"writing succesful: "<<std::endl;

	return 0;
}



int main()
{
	
	//reading models
	read model;
	model.read_time_bin();
	model.read_wavel_bin();
	model.read_model_bin_all_cont();
	//reading spectrum to fit
	model.usr_read_sample();





	
	/*Step1: Getting platforms and choose an available one.*/
	cl_uint numPlatforms;				//the NO. of platforms
	cl_platform_id* platforms = NULL; 	//id of available platforms
	cl_platform_id 	platform = NULL;	//id of the chosen platform


	//getting NO. of platforms
	cl_int	status = clGetPlatformIDs(0, NULL, &numPlatforms); 
	if (status != CL_SUCCESS)
	{
		std::cout << "Error: Getting platforms!" << std::endl;
		return FAILURE;
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
	std::cout<<"\nChoose device: (type the number)"<<std::endl;
	int device_choice;
	std::string temp_line1;
	getline(std::cin,temp_line1);
	std::stringstream temp_sstr1;
	temp_sstr1<<temp_line1;
	temp_sstr1>>device_choice;
	device = devices[device_choice];

	device=devices[0];

	//starting clock
	clock_t t1, t2;
	t1 = clock();

	/*Step 3: Create context.*/
	cl_context context = clCreateContext(NULL,1, devices,NULL,NULL,&status);
	if (status!=0)
	{
		std::cout<<"ERROR creating context: "<<status<<std::endl;
	}
	 
	/*Step 4: Creating command queue associate with the context.*/
	cl_command_queue commandQueue = clCreateCommandQueue(context, device, 0, &status);
	if (status!=0)
	{
		std::cout<<"ERROR creating commandqueue: "<<status<<std::endl;
	}

	/*Step 5: Create program object */
	const char *filename = "fit_dp.cl";
	std::string sourceStr;
	status = convertToString(filename, sourceStr);
	const char *source = sourceStr.c_str();
	size_t sourceSize[] = {strlen(source)};
	cl_program program = clCreateProgramWithSource(context, 1, &source, sourceSize, &status);
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
	
	/*Step 7: Allocate memory */
	int nspecsteps=model.wavelengths.size();
	int ntimesteps=model.time.size();

	//data
	cl_mem model_without_dust_d = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, model.model_cont.size() * sizeof(double),(void *) model.model_cont.data(), &status);
	cl_mem time_d = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, ntimesteps * sizeof(double),(void *) model.time.data(), &status);
	cl_mem wavel_d = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, nspecsteps * sizeof(double),(void *) model.wavelengths.data(), &status);
	cl_mem sample_spec_d = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, nspecsteps * sizeof(double),(void *) model.sample_spec.data(), &status);
	
	//buffers to write
	cl_mem model_d = clCreateBuffer(context, CL_MEM_READ_WRITE,		sizeof(double) * model.model_cont.size() , NULL, &status);
	cl_mem result_d = clCreateBuffer(context, CL_MEM_READ_WRITE,	sizeof(double) * nspecsteps , NULL, &status);
	cl_mem factor1_d = clCreateBuffer(context, CL_MEM_WRITE_ONLY,	sizeof(double) * nspecsteps , NULL, &status);
	cl_mem factor2_d = clCreateBuffer(context, CL_MEM_WRITE_ONLY,	sizeof(double) * nspecsteps , NULL, &status);
	cl_mem chi_d = clCreateBuffer(context,  CL_MEM_WRITE_ONLY,	sizeof(double) * nspecsteps , NULL, &status);		
	if (status!=0)
	{
		std::cout<<"ERROR creating buffers: "<<status<<std::endl;
	}


	/*Step 8: Create kernel objects */
	cl_kernel kernel = clCreateKernel(program,"fit_dp_a", &status);
	if (status!=0)
	{
		std::cout<<"ERROR creating kernel: "<<status<<std::endl;
	}
	cl_kernel kernel2 = clCreateKernel(program,"fit_dp_b", &status);
	if (status!=0)
	{
		std::cout<<"ERROR creating kernel2: "<<status<<std::endl;
	}

	double dust_tau_v;
	double dust_mu;
	double sfr_tau;
	double age;
	double metall;
	
	double d_dust_tau_v;
	double d_dust_mu;
	double d_sfr_tau;
	double d_age;
	double d_metall;
	
	std::vector<double> factor1(nspecsteps);		//stores one factor 
	std::vector<double> factor2(nspecsteps);		//stores an other

	double factor;
	double temp_1;
	double temp_2;
	double chi;
	double chi_before=DBL_MAX;

	int stay=0;

	std::vector<double> chis(nspecsteps);			//stores the chi sqr values

		
	/*Setting kernel arguments*/

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
	status = clSetKernelArg(kernel2, 0, sizeof(cl_mem), &sample_spec_d);
	status |= clSetKernelArg(kernel2, 1, sizeof(cl_mem), &result_d);
	status |= clSetKernelArg(kernel2, 2, sizeof(cl_mem), &chi_d);

	if (status!=0)
		std::cout<<"ERROR setting kernel arguments: "<<status<<std::endl;
	
	dust_tau_v=1;
	dust_mu=0.3;
	sfr_tau=3e+09;
	age=1e+10;
	metall=0.025;

//later 
	std::string imf="chabrier";
	std::cout<<"\nusing chabrier imf \n\n";
	int offset;
	if( imf == "chabrier")
		offset=0;
	else if(imf=="salpeter")
		offset=6;
	
	double model_metal[6]={0.0001,0.0004,0.004,0.008,0.02,0.05};
	int modelno;
	double sign;

	std::cout<<"no. of iterations:\n";

	for(int i=0; i<1000	;i++)
	{
		//creating random jump
		sign= ( (rand() % 2) - 0.5)*2;
		d_dust_tau_v=sign * dust_tau_v * log(1+ rand() / (5 * double(RAND_MAX)));
		sign= ( (rand() % 2) - 0.5)*2;
		d_dust_mu=sign * dust_mu* log(1+ rand() / (5 * double(RAND_MAX))) ;
		sign= ( (rand() % 2) - 0.5)*2;
		d_sfr_tau=sign * sfr_tau*  log(1+ rand() / (5  * double(RAND_MAX)));
		sign= ( (rand() % 2) - 0.5)*2;
		d_age=sign * age*  log(1+  rand() / (5  * double(RAND_MAX)));
		sign= ( (rand() % 2) - 0.5)*2;
		d_metall=sign * metall * log (1+ rand() / ( 5 * double(RAND_MAX))) ;

		/*Step 9: Set Kernel arguments.*/
		dust_tau_v+=d_dust_tau_v;
		dust_mu+=d_dust_mu;
		sfr_tau+=d_sfr_tau;
		age+=d_age;
		metall+=d_metall;

		if(dust_mu <=0.1 || dust_mu >1 || dust_tau_v <0.5 || dust_tau_v>2
			|| age<1e+9 || age>2e+10 ||sfr_tau>20e+10 
			|| sfr_tau<=1e+8 || metall<0.0001 || metall>0.05)
			
		{
			dust_tau_v-=d_dust_tau_v;
			dust_mu-=d_dust_mu;
			sfr_tau-=d_sfr_tau;
			age-=d_age;
			metall-=d_metall;
			i--;
			continue;
		}
		
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
			break;
		}

		cl_event kernel_event[1];
		/*Step 10: Running the kernel.*/
		size_t global_work_size[1] = {nspecsteps};
		status = clEnqueueNDRangeKernel(commandQueue, kernel, 1, NULL, global_work_size, NULL, 0, NULL, &kernel_event[0]);
		if (status!=0)
		{
			std::cout<<"ERROR running kernel: "<<status<<std::endl;
			break;
		}	


		/*Step 11: Read the result back to host memory.*/
		status = clEnqueueReadBuffer(commandQueue, factor1_d, CL_TRUE, 0, nspecsteps * sizeof(double) , factor1.data(), 1,kernel_event, NULL);
		status = clEnqueueReadBuffer(commandQueue, factor2_d, CL_TRUE, 0, nspecsteps * sizeof(double) , factor2.data(), 0, NULL, NULL);
		if (status!=0)
		{
			std::cout<<"ERROR reading buffer: "<<status<<std::endl;
			break;
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
			break;
		}

		/*Step 10: Running the kernel.*/
	//	size_t global_work_size[1] = {nspecsteps};
		status = clEnqueueNDRangeKernel(commandQueue, kernel2, 1, NULL, global_work_size, NULL, 0, NULL, &kernel_event[0]);
		if (status!=0)
		{
			std::cout<<"ERROR running kernel2: "<<status<<std::endl;
			break;
		}	

		/*Step 11: Read the result back to host memory.*/
		status = clEnqueueReadBuffer(commandQueue, chi_d, CL_TRUE, 0, nspecsteps * sizeof(double) , chis.data(), 1, kernel_event, NULL);
		if (status!=0)
		{
			std::cout<<"ERROR reading buffer: "<<status<<std::endl;
			break;
		}

	//summing chisquare
		chi=0;
		for(int i=0;i<nspecsteps;i++)
		{
			chi+=chis[i];
		}
		
		//if no better than before, we step back
		if(chi_before<chi)
		{
			dust_tau_v-=d_dust_tau_v;
			dust_mu-=d_dust_mu;
			sfr_tau-=d_sfr_tau;
			age-=d_age;
			metall-=d_metall;

			stay++;
		}
		else
		{
			stay=0;
			chi_before=chi;
		}

		if(stay>300)
		{
			std::cout<<"fit converged\n";
			break;
		}

		//	std::cout<<chi<<"\n ";
		if( (i % 100) == 0)
			std::cout<<i<<"\n";
	}
	
	if(stay <= 300)
	{
		std::cout<<"\nfit did not converge\n\n";
	}
	
	std::vector <double> result (nspecsteps);
	std::vector<std::vector <double> > output;
	output.push_back(model.wavelengths);
	output.push_back(model.sample_spec);


	status = clEnqueueReadBuffer(commandQueue, result_d, CL_TRUE, 0, nspecsteps * sizeof(double) , result.data(), 0, NULL, NULL);
	if (status!=0)
	{
		std::cout<<"ERROR reading buffer: "<<status<<std::endl;
	}
	output.push_back(result);
	write_table_col(output,"../output/fit.txt");
	
	std::cout<<"\nParams:\n";
	std::cout<<"dust_tau_v="<<dust_tau_v<<std::endl;
	std::cout<<"dust_mu="<<dust_mu<<std::endl;
	std::cout<<"sfr_tau="<<sfr_tau<<std::endl;
	std::cout<<"age="<<age<<std::endl;
	std::cout<<"metall="<<metall<<std::endl;
	std::cout<<"chisquare="<<chi_before<<std::endl;


	std::cout<<"\nready\n";


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
	{
		std::cout<<"ERROR releasing objects: "<<status<<std::endl;
	}
	delete[] devices;

	t2 = clock();
	double diff = (((double)t2 - (double)t1)/CLOCKS_PER_SEC);
	//some info out
	std::cout<< "It took "<< diff <<" second(s)."<< std::endl;


	if (status==0)
	{
		std::cout<<"Passed!"<<std::endl;
	}
	std::cout<<"done (press enter)";
	std::cin.get();

	return	0;

}
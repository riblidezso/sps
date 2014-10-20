#include "opencl_fit_w_err.h"
#include "read.h"
#include "write.h"

#include <float.h>
#include <string.h>



//initalizing from the read object
opencl_fit_w_err::opencl_fit_w_err(read& model)
{
	//copy data sizes
	nspecsteps=model.wavelengths.size();
	ntimesteps=model.time.size();
	mes_nspecsteps=model.mes_spec_wavel.size();

	//copy small data
	time=model.time;
	mes_spec=model.mes_spec;
	mes_spec_wavel=model.mes_spec_wavel;
	mes_spec_mask=model.mes_spec_mask;
	mes_spec_err=model.mes_spec_err_h;

	//resize the data scturcures that will 
	//be computed at every wavelength
	factor1.resize(mes_nspecsteps);		 
	factor2.resize(mes_nspecsteps);		
	chis.resize(mes_nspecsteps);
	result.resize(mes_nspecsteps);

	//resize the vector is the parameters
	//one vector will strore the parameter
	//values in the markov chain 
	temp_point.resize(6);

	//chi values
	chi_before=DBL_MAX;
	best_chi=DBL_MAX;

	//burnin indicator
	burnin_ended==false;

	//imf, if you want to use salpeter
	//uncomment that line and comment chabrier
	imf="chabrier";
	//imf="salpeter";

	//initial sigma parameter of the normal distribution
	// of steps is the markov chain
	// the step is relative so it is 0.8% now
	sigma=0.008;

}

#include <iostream>
#include <sstream>
int opencl_fit_w_err::read_config(std::string input_filename)
{
	std::ifstream infile(input_filename.c_str(), std::ifstream::in );
	std::string str;
	while(getline(infile,str))
	{
		std::stringstream sstr;
		std::vector<std::string> tempvec;
		sstr<<str;	
		while(sstr>>str)
		{
			tempvec.push_back(str);
		}
		if( tempvec.size()>0)
		{
			std::cout<<tempvec[0]<<"\t\t"<<tempvec[1]<<std::endl;
			config_map.insert( std::pair<std::string,std::string> (tempvec[0],tempvec[1]) );
		}
	}
//	std::cout<<"init_guess for age\t"<<atof(config_map["init_guess_age"].c_str())<<std::endl;

	infile.close();
	return 0;
}


//set initial parameters
int opencl_fit_w_err::set_initial_params()
{
	std::cout<<"\nSetting initial parameter guesses:\n";	
	
	//set dust_tau_v
	if (config_map.count("init_guess_dust_tau_v")==1)
	{
		dust_tau_v=atof(config_map["init_guess_dust_tau_v"].c_str());
		std::cout<<"initial guess for dust_tau_v\t\t"<<dust_tau_v<<std::endl;
	}
	else
	{
		std::cout<<"initial guess for dust_tau_v not found, setting it to 1.0"<<std::endl;
		dust_tau_v=1.0;
	}

	//set dust_mu
	if (config_map.count("init_guess_dust_mu")==1)
	{
		dust_mu=atof(config_map["init_guess_dust_mu"].c_str());
		std::cout<<"initial guess for dust_mu\t\t"<<dust_mu<<std::endl;
	}
	else
	{
		std::cout<<"initial guess for dust_mu not found, setting it to 0.3"<<std::endl;
		dust_mu=0.3;
	}

	//set sfr_tau 
	if (config_map.count("init_guess_sfr_tau")==1)
	{
		sfr_tau=atof(config_map["init_guess_sfr_tau"].c_str());
		std::cout<<"initial guess for sfr_tau\t\t"<<sfr_tau<<std::endl;
	}
	else
	{
		std::cout<<"initial guess for sfr_tau not found, setting it to 3.0e+08"<<std::endl;
		sfr_tau=3.0e+08;
	}

	//set metall 
	if (config_map.count("init_guess_metall")==1)
	{
		metall=atof(config_map["init_guess_metall"].c_str());
		std::cout<<"initial guess for metall\t\t"<<metall<<std::endl;
	}
	else
	{
		std::cout<<"initial guess for metall not found, setting it to 0.01 "<<std::endl;
		metall=0.01;
	}

	//set age 
	if (config_map.count("init_guess_age")==1)
	{
		age=atof(config_map["init_guess_age"].c_str());
		std::cout<<"initial guess for age\t\t\t"<<age<<std::endl;
	}
	else
	{
		std::cout<<"initial guess for age not found, setting it to 2e+09 "<<std::endl;
		age=2e+09;
	}

	//set vdisp 
	if (config_map.count("init_guess_vdisp")==1)
	{
		vdisp=atof(config_map["init_guess_vdisp"].c_str());
		std::cout<<"initial guess for vdisp\t\t"<<vdisp<<std::endl;
	}
	else
	{
		std::cout<<"initial guess for vdisp not found, setting it to 0.0003 "<<std::endl;
		metall=0.0003;
	}
	std::cout<<"\n";	

	return 0;
}

//fix parameters if needed
int opencl_fit_w_err::fix_params()
{
	std::cout<<"\nFixing parameters :\n";	
	
	//set dust_tau_v
	if (config_map.count("fix_dust_tau_v")==1)
	{
		fix_dust_tau_v=true;
		std::cout<<"dust_tau_v\tfixed"<<std::endl;
	}
	else
	{
		fix_dust_tau_v=false;
		std::cout<<"dust_tau_v\tnot fixed"<<std::endl;
	}

	//set dust_mu
	if (config_map.count("fix_dust_mu")==1)
	{
		fix_dust_mu=true;
		std::cout<<"dust_tau_v\t\tfixed"<<std::endl;
	}
	else
	{
		fix_dust_mu=false;
		std::cout<<"dust_mu\t\tnot fixed"<<std::endl;
	}

	//set sfr_tau 
	if (config_map.count("fix_sfr_tau")==1)
	{
		fix_sfr_tau=true;
		std::cout<<"sfr_tau\t\tfixed"<<std::endl;
	}
	else
	{
		fix_sfr_tau=false;
		std::cout<<"sfr_tau\t\tnot fixed"<<std::endl;
	}

	//set metall 
	if (config_map.count("fix_metall")==1)
	{
		fix_metall=true;
		std::cout<<"metall\t\tfixed"<<std::endl;
	}
	else
	{
		fix_metall=false;
		std::cout<<"metall\t\tnot fixed"<<std::endl;
	}

	//set age 
	if (config_map.count("fix_age")==1)
	{
		fix_age=true;
		std::cout<<"age\t\tfixed"<<std::endl;
	}
	else
	{
		fix_age=false;
		std::cout<<"age\t\tnot fixed"<<std::endl;
	}

	//set vdisp 
	if (config_map.count("fix_vdisp")==1)
	{
		fix_vdisp=true;
		std::cout<<"vdisp\t\tfixed"<<std::endl;
	}
	else
	{
		fix_vdisp=false;
		std::cout<<"vdisp\t\tnot fixed"<<std::endl;
	}
	std::cout<<"\n";	

	return 0;
}



//this function gets 
			//original models from read object
			//and measuremant data

//and then resamples all the models to the wavelengths of
//the measurement, this reduces computation time 6900->3-4000
int opencl_fit_w_err::resample_models_2_mes(read& model)
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
		return 1;
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
		std::cerr<<"ERROR creating context: "<<status<<std::endl;
	 
	//Creating command queue associate with the context
	commandQueue = clCreateCommandQueue(context, device, 0, &status);
	if (status!=0)
		std::cerr<<"ERROR creating commandqueue: "<<status<<std::endl;

	//open kernel file and convert it to char array
	const char *filename = kernel_filename.c_str();
	std::string sourceStr;
	status = convertToString(filename, sourceStr);
	const char *source = sourceStr.c_str();
	size_t sourceSize[] = {strlen(source)};

	//Create program object
	program = clCreateProgramWithSource(context, 1, &source, sourceSize, &status);
	if (status!=0)
		std::cout<<"ERROR creating program: "<<status<<std::endl;

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
	resampled_model_d = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, resampled_model.size() * sizeof(double),(void *) resampled_model.data(), &status);
	time_d = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, ntimesteps * sizeof(double),(void *) time.data(), &status);
	wavel_d = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, mes_nspecsteps * sizeof(double),(void *) mes_spec_wavel.data(), &status);
	mes_spec_d = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, mes_nspecsteps * sizeof(double),(void *) mes_spec.data(), &status);
	mes_spec_err_d = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, mes_nspecsteps * sizeof(double),(void *) mes_spec_err.data(), &status);
	mes_spec_mask_d = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, mes_nspecsteps * sizeof(double),(void *) mes_spec_mask.data(), &status);
	


	//buffers to write
	model_d = clCreateBuffer(context, CL_MEM_READ_WRITE,	sizeof(double) * ntimesteps * mes_nspecsteps , NULL, &status);
	result_no_vel_d = clCreateBuffer(context, CL_MEM_READ_WRITE,	sizeof(double) * mes_nspecsteps , NULL, &status);
	result_d = clCreateBuffer(context, CL_MEM_READ_WRITE,	sizeof(double) * mes_nspecsteps , NULL, &status);
	factor1_d = clCreateBuffer(context, CL_MEM_WRITE_ONLY,	sizeof(double) * mes_nspecsteps , NULL, &status);
	factor2_d = clCreateBuffer(context, CL_MEM_WRITE_ONLY,	sizeof(double) * mes_nspecsteps , NULL, &status);
	chi_d = clCreateBuffer(context,  CL_MEM_WRITE_ONLY,		sizeof(double) * mes_nspecsteps , NULL, &status);		

	//error check
	if (status!=0)
		std::cerr<<"ERROR creating buffers: "<<status<<std::endl;


	// Create kernel objects
	kernel_spec_gen = clCreateKernel(program,"spec_gen", &status);
	if (status!=0)
		std::cerr<<"ERROR creating kernel_spec_gen: "<<status<<std::endl;

	kernel_vel_disp = clCreateKernel(program,"mask_veloc_disp", &status);
	if (status!=0)
		std::cerr<<"ERROR creating kernel_vel_disp: "<<status<<std::endl;

	kernel_chi_calc = clCreateKernel(program,"chi_calculation", &status);
	if (status!=0)
		std::cerr<<"ERROR creating kernel_chi_calc: "<<status<<std::endl;

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



//change parameters
//this will be called at every iteration
//in the markov chain
//
//opt acc is the optimal acceptace ratio
int opencl_fit_w_err::change_params(double opt_acc)
{
	//error variable
	int status=0;

	//control the step size
	//better control mechanism should be used
	if(acc_ratio.size()>1 && iter%200==1 && iter>3 )
	{
		if (acc_ratio[acc_ratio.size()-1]>opt_acc && sigma < 0.5)
			sigma=sigma+sigma*(acc_ratio[acc_ratio.size()-1]-opt_acc);
		
		if (acc_ratio[acc_ratio.size()-1]<opt_acc)
			sigma=sigma-sigma*(opt_acc-acc_ratio[acc_ratio.size()-1]);

		//std::cout<<"\n acc_rate="<<acc_ratio[acc_ratio.size()-1]<<"\n";
	}

	//this should not happen	
	if(sigma<0)
	{
		std::cout<<"ERROR sigma < 0";
		return 1;
	}


	//create jump

	//intialize normal distribution random generator
	std::normal_distribution<double> distribution(0,sigma);
	do{
		status=0;

		//creating normal distribution random jump
		if (fix_dust_tau_v==false)
			d_dust_tau_v= dust_tau_v * distribution(generator);
		else
			d_dust_tau_v=0;
		dust_tau_v+=d_dust_tau_v;

		if (fix_dust_mu==false)
			d_dust_mu= dust_mu * distribution(generator) ;
		else
			d_dust_mu=0;
		dust_mu+=d_dust_mu;

		if (fix_sfr_tau==false)
			d_sfr_tau= sfr_tau *  distribution(generator) ;
		else
			d_sfr_tau=0;
		sfr_tau+=d_sfr_tau;

		if (fix_age==false)
			d_age= age *  distribution(generator) ;
		else
			d_age=0;
		age+=d_age;

		if (fix_metall==false)
			d_metall= metall * distribution(generator) ;
		else
			d_metall=0;
		metall+=d_metall;

		if (fix_vdisp==false)
			d_vdisp= vdisp * distribution(generator) ;
		else
			d_vdisp=0;
		vdisp+=d_vdisp;

		//check boundaries
		if(	dust_mu <=0 || dust_mu >1 || 
			dust_tau_v <0 || dust_tau_v>1.5 ||
			age<1e+8 || age>2e+10 ||
			sfr_tau>40e+19	|| sfr_tau<=1e+7 || 
			metall<0.0001 || metall>0.05 || 
			vdisp > 0.002)
			
		{
			dust_tau_v-=d_dust_tau_v;
			dust_mu-=d_dust_mu;
			sfr_tau-=d_sfr_tau;
			age-=d_age;
			metall-=d_metall;
			vdisp-=d_vdisp;

			status=1;
		}
	}while(status==1);


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
	status = clSetKernelArg(kernel_spec_gen, 12, sizeof(double), &dust_tau_v);
	status |= clSetKernelArg(kernel_spec_gen, 13, sizeof(double), &dust_mu);
	status |= clSetKernelArg(kernel_spec_gen, 14, sizeof(double), &sfr_tau);
	status |= clSetKernelArg(kernel_spec_gen, 15, sizeof(double), &age);
	status |= clSetKernelArg(kernel_spec_gen, 16, sizeof(double), &metall);
	status |= clSetKernelArg(kernel_spec_gen, 17, sizeof(int), &modelno);
	if (status!=0)
	{
		std::cerr<<"ERROR setting kernel_spec_gen arguments: "<<status<<std::endl;
		return status;
	}

	status = clSetKernelArg(kernel_vel_disp, 9, sizeof(double), &vdisp);
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
	double temp_1,temp_2,factor;

	//generating spectrum (no velocity dispersion)
	// Running the kernel.
	size_t global_work_size[1] = {mes_nspecsteps};
	status = clEnqueueNDRangeKernel(commandQueue, kernel_spec_gen, 1, NULL, global_work_size, NULL, 0, NULL,NULL);
	if (status!=0)
		std::cerr<<"ERROR running kernel_spec_gen: "<<status<<std::endl;


	//next kernel for velocity dispersion
	// Running the kernel
	status = clEnqueueNDRangeKernel(commandQueue, kernel_vel_disp, 1, NULL, global_work_size, NULL, 0, NULL,NULL);
	if (status!=0)
	{
		std::cerr<<"ERROR running kernel_vel_disp: "<<status<<std::endl;
		return 1;
	}

	//Read the result back to host memory
	status = clEnqueueReadBuffer(commandQueue, factor1_d, CL_TRUE, 0, mes_nspecsteps * sizeof(double) , factor1.data(), 0, NULL,NULL);
	status = clEnqueueReadBuffer(commandQueue, factor2_d, CL_TRUE, 0, mes_nspecsteps * sizeof(double) , factor2.data(), 0, NULL, NULL);
	if (status!=0)
	{
		std::cerr<<"ERROR reading buffer,from kernel_vel: "<<status<<std::endl;
		return 1;
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
	status |= clSetKernelArg(kernel_chi_calc, 5, sizeof(double), &factor);
	if (status!=0)
	{
		std::cerr<<"ERROR setting kernel_chi_calc arguments: "<<status<<std::endl;
		return 1;
	}
	//Running the kernel
	status = clEnqueueNDRangeKernel(commandQueue, kernel_chi_calc, 1, NULL, global_work_size, NULL, 0, NULL, NULL); 
	if (status!=0)
	{
		std::cerr<<"ERROR running kernel_chi_calc: "<<status<<std::endl;
		return 1;
	}	

	//Read the chis to host memory.
	//now we dont read back the fittes spectrum
	//only when it is the best so far
	status = clEnqueueReadBuffer(commandQueue, chi_d, CL_TRUE, 0, mes_nspecsteps * sizeof(double) , chis.data(), 0,NULL,NULL);
	if (status!=0)
	{
		std::cerr<<"ERROR reading chi buffer: "<<status<<std::endl;
		return 1;
	}
	
	return status;
}

int opencl_fit_w_err::evaluate_chi(double temp)
{
	//summing chisquare
	chi=0;
	for(int i=0;i<mes_nspecsteps;i++)
		chi+=chis[i];

	//evaluating chi
	if (best_chi > chi)
		accepted=0;
	else if (chi_before>chi)
		accepted=1;
	//if chi is no better than the one before
	//Metropolis Hastings rejection/acceptance step
	else 
	{
		double limit=exp((chi_before-chi)/temp) * RAND_MAX;
		double rand_num=rand();

		if(rand_num > limit )
			accepted=3;	//rejection
		else
			accepted=2;	//acceptance
	}
	return 0;
}


//this function is called in everz iteration and records 
//the parameter values in the step, and the accceptance rate
//and other diagnostic data
int opencl_fit_w_err::record_data()
{
	int status=0;
	
	out_chi_evol.push_back(chi);
	out_best_chi_evol.push_back(best_chi);

	//check if burnin has ended
	if (burnin_ended==false )
	{
		int window=20000;
		if ( iter > window )
		{
			if (chi > out_chi_evol[iter-window])
			{
				burnin_ended=true;
				std::cout<<"\nBurnin section ended at: "<<iter<<"\n"<<std::endl;
			}	
		}
	}
	

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
		temp_point[5]=vdisp;

		//record temp data to parameter chain
		//now i do not record points arter
		//rejected step
		//that might would result in 
		//a bit different histograms
		if(burnin_ended)
			points.push_back(temp_point); 
		
	}
	else //not accepted
	{
		dust_tau_v-=d_dust_tau_v;
		dust_mu-=d_dust_mu;
		sfr_tau-=d_sfr_tau;
		age-=d_age;
		metall-=d_metall;
		vdisp-=d_vdisp;
		out_acc_chi_evol.push_back(0);
		acc.push_back(0);
	}

	if (accepted==0) //the best chi 
	{
		best_chi=chi;
		best_dust_tau_v=dust_tau_v;
		best_dust_mu=dust_mu;
		best_sfr_tau=sfr_tau;
		best_age=age;
		best_metall=metall;
		best_vdisp=vdisp;
					
		status = clEnqueueReadBuffer(commandQueue, result_d, CL_TRUE, 0, mes_nspecsteps * sizeof(double) , result.data(), 0, NULL, NULL);
		if (status!=0)
			std::cout<<"ERROR reading buffer: "<<status<<std::endl;
	}

	if(accepted==2 || accepted==3) //worse step
		worse.push_back(1);
	else //better step
		worse.push_back(0);

	if(accepted==2) //worse but accepted
		worse_acc.push_back(1);
	if(accepted==3) //worse and not accepted
		worse_acc.push_back(0);


	//counting average values

	//now not counting for time
	if(iter%200==0)
	{
		double mean=0;
		if (worse_acc.size()>100)
		{
			for(int i=0;i<100;i++)
				mean+=worse_acc[worse_acc.size()-100+i];

			worse_acc_ratio.push_back(mean/100);
		}

		mean=0;
		if (worse.size()>100)
		{
			for(int i=0;i<100;i++)
				mean+=worse[worse.size()-100+i];

			worse_rate.push_back(mean/100);
		}
	
		mean=0;

		if (acc.size()>100)
		{
			for(int i=0;i<100;i++)
				mean+=acc[acc.size()-100+i];
				
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
	std::cout<<"veloc_disp="<<best_vdisp<<std::endl;
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
	outfile<<"veloc_disp="<<best_vdisp<<std::endl;
	outfile<<"log(P)="<<best_chi<<std::endl;
	
	outfile.close();

	//info out
	std::cout<<"writing succesful: "<<std::endl;
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

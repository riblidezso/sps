#include "spectrum_generator.h"

spectrum_generator::spectrum_generator(sps_data& input_data,std::string kernel_filename,int input_platform,int input_device){
    
    //copy and move data from model
    copy_data(input_data);
    
    //initialize everything with opencl
    opencl_initialize(kernel_filename,input_platform,input_device);
}


/////////////////////////////////////////////////////////////////////////////
// functions for initialization
/////////////////////////////////////////////////////////////////////////////


/*
 copies and moves data from sps data
 */
int spectrum_generator::copy_data(sps_data& input_data){
    //copy data sizes
    this->ntimesteps=(int) input_data.time.size();
    this->mes_nspecsteps=(int) input_data.mes_spec_wavel.size();
    
    //copy (and there is type conversion too) data
    this->time=std::vector<cl_float>(input_data.time.begin(),input_data.time.end());
    this->resampled_model=std::vector<cl_float>(input_data.resampled_model_cont.begin(),input_data.resampled_model_cont.end());
    
    this->mes_spec=std::vector<cl_float>(input_data.mes_spec.begin(),input_data.mes_spec.end());
    this->mes_spec_wavel=std::vector<cl_float>(input_data.mes_spec_wavel.begin(),input_data.mes_spec_wavel.end());
    this->mes_spec_mask=std::vector<cl_float>(input_data.mes_spec_mask.begin(),input_data.mes_spec_mask.end());
    this->mes_spec_err=std::vector<cl_float>(input_data.mes_spec_err_h.begin(),input_data.mes_spec_err_h.end());
    
    return 0;
}

/////////////////////////////////////////////////////////////////////////////
// functions for opencl init
/////////////////////////////////////////////////////////////////////////////


/*
 do all the opencl initializations
 */
int spectrum_generator::opencl_initialize(std::string kernel_filename,int input_device, int input_platform){
    select_opencl_platform(input_platform);
    select_opencl_device(input_device);
    opencl_create_context();
    opencl_create_command_queue();
    opencl_build_program(kernel_filename);
    opencl_create_buffers();
    opencl_create_kernels();
    set_initial_kernel_args();
    
    return 0;
}



//////////////////////////////////////////////////

/*
 select opencl platform
 */
int spectrum_generator::select_opencl_platform(int input_platform){
    query_opencl_platforms(this->n_platforms,this->platforms);
    
    //check platform choice and accept it
    if (input_platform>=0 && input_platform < (int) n_platforms){
        this->platform=platforms[input_platform];
    }
    else{
        std::cerr<<"ERROR invalid platform number: "<<input_platform<<std::endl;
        exit(1);
    }
    return 0;
}


/*
 query the opencl platforms available
 */

int spectrum_generator::query_opencl_platforms(cl_uint& n_platforms,cl_platform_id*& platforms){
    //error variable
    cl_int	status=0;
    
    //query the number of platforms
    status = clGetPlatformIDs(0, NULL, &n_platforms);
    if (status != CL_SUCCESS){
        std::cerr << "Error: Getting the number of platforms!: "<<status<< std::endl;
        exit(1);
    }
    
    //Query platform info
    if(n_platforms > 0){
        //get platform ids
        platforms =  new cl_platform_id[n_platforms];
        status = clGetPlatformIDs(n_platforms, platforms, NULL);
        if (status != CL_SUCCESS){
            std::cerr << "Error: Getting platforms!: "<<status<< std::endl;
            exit(1);
        }
        
        //query platform info
        std::cout<<"\nPlatform info:"<<std::endl;
        for(unsigned int i=0;i<n_platforms;i++){
            query_opencl_platform_info(i,platforms[i]);
        }
    }
    return 0;
}

/*
 query information about an opencl platform
 */

int spectrum_generator::query_opencl_platform_info(int i, cl_platform_id platform){
    //error variable
    cl_int	status=0;
    
    //get platform name
    size_t platform_name_size;
    status = clGetPlatformInfo( platform , CL_PLATFORM_NAME ,0, NULL, &platform_name_size);
    char* platform_name = new char[platform_name_size];
    status |= clGetPlatformInfo( platform , CL_PLATFORM_NAME ,platform_name_size, platform_name,NULL);
    if (status != CL_SUCCESS){
        std::cerr << "Error: Getting platform name!: "<<status<< std::endl;
        exit(1);
    }
    
    //get platform version
    size_t platform_version_size;
    status = clGetPlatformInfo( platform, CL_PLATFORM_VERSION, 0,NULL,&platform_version_size);
    char* platform_version = new char[platform_version_size];
    status |= clGetPlatformInfo( platform, CL_PLATFORM_VERSION , platform_version_size, platform_version,NULL);
    if (status != CL_SUCCESS){
        std::cerr << "Error: Getting platform version!: "<<status<< std::endl;
        exit(1);
    }
    
    //print info
    std::cout<<i<<". platform:\t"<<platform_name<<"\n";
    std::cout<<"    version:\t"<<platform_version<<"\n";
    std::cout<<std::endl;
    
    delete[] platform_name;
    delete[] platform_version;
    
    return 0;
}


//////////////////////////////////////////////////
/*
 query the devices on an opencl platform
 */

int spectrum_generator::query_opencl_devices(cl_platform_id platform,cl_uint& n_devices,cl_device_id*& devices){
    //error variable
    cl_int	status=0;
    
    //get number of devices
    status = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 0, NULL, &n_devices);
    if (status != CL_SUCCESS){
        std::cerr << "Error: Getting the number of devices!: "<<status<< std::endl;
        exit(1);
    }
    
    if(n_devices>0){
        //getting device ids
        devices = new cl_device_id[n_devices];
        status = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, n_devices, devices, NULL);
        if (status != CL_SUCCESS){
            std::cerr << "Error: Getting devices!: "<<status<< std::endl;
            exit(1);
        }
        
        //query platform info
        std::cout<<"\nDevice info:"<<std::endl;
        for(unsigned int i=0;i<n_devices;i++){
            query_opencl_device_info(i,devices[i]);
        }
    }
    return 0;
}

//////////////////////////////////////////////////////
/*
 select opencl device
 
 */
int spectrum_generator::select_opencl_device(int input_device){
    //query devices
    query_opencl_devices(platform,this->n_devices,this->devices);
    
    //check device choice and accept it
    if(input_device>=0 && input_device < (int) n_devices ){
        this->device=devices[input_device];
    }
    else{
        std::cerr<<"ERROR invalid device number: "<<input_device<<std::endl;
        exit(1);
    }
    return 0;
}



/*
query information about an opencl device
*/

int spectrum_generator::query_opencl_device_info(int i, cl_device_id device){
    //error variable
    cl_int	status=0;
    
    //get device vendor
    size_t device_vendor_size;
    status = clGetDeviceInfo( device, CL_DEVICE_VENDOR, 0,NULL,&device_vendor_size);
    char* device_vendor = new char[device_vendor_size];
    status |= clGetDeviceInfo( device, CL_DEVICE_VENDOR, device_vendor_size,device_vendor,NULL);
    if (status != CL_SUCCESS){
        std::cerr << "Error: Getting device vendor!: "<<status<< std::endl;
        exit(1);
    }
    
    //get device name
    size_t device_name_size;
    status = clGetDeviceInfo( device, CL_DEVICE_NAME, 0,NULL,&device_name_size);
    char* device_name = new char[device_name_size];
    status |= clGetDeviceInfo( device, CL_DEVICE_NAME, device_name_size,device_name,NULL);
    if (status != CL_SUCCESS){
        std::cerr << "Error: Getting device name!: "<<status<< std::endl;
        exit(1);
    }
    
    //get device type
    cl_device_type device_type;
    status = clGetDeviceInfo( device, CL_DEVICE_TYPE, sizeof(cl_device_type),&device_type,NULL);
    if (status != CL_SUCCESS){
        std::cerr << "Error: Getting device type!: "<<status<< std::endl;
        exit(1);
    }
    
    //get device version
    size_t device_version_size;
    status = clGetDeviceInfo( device, CL_DEVICE_VERSION, 0,NULL,&device_version_size);
    char* device_version = new char[device_version_size];
    status |= clGetDeviceInfo( device, CL_DEVICE_VERSION , device_version_size, device_version,NULL);
    if (status != CL_SUCCESS){
        std::cerr << "Error: Getting device version!: "<<status<< std::endl;
        exit(1);
    }
    
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
    //version
    std::cout<<"         version:\t"<<device_version<<std::endl;
    
    delete[] device_vendor;
    delete[] device_name;
    delete[] device_version;
    
    return 0;
}

//////////////////////////////////////////////////

int spectrum_generator::opencl_create_context(){
    //error variable
    cl_int	status=0;
    
    //Create context
    this->context = clCreateContext(NULL,n_devices, devices,NULL,NULL,&status);
    if (status!=0){
        std::cerr<<"ERROR creating context: "<<status<<std::endl;
        exit(1);
    }
    return 0;
}

int spectrum_generator::opencl_create_command_queue(){
    //error variable
    cl_int	status=0;
    
    //Creating command queue associated with the context
    this->commandQueue = clCreateCommandQueue(this->context, device, 0, &status);
    if (status!=0){
        std::cerr<<"ERROR creating commandqueue: "<<status<<std::endl;
        exit(1);
    }
    return 0;
}



//////////////////////////////////////////////////

/*
 load kernels from file and build them
 */
int spectrum_generator::opencl_build_program(std::string kernel_filename){
    //error variable
    cl_int	status=0;
    
    //load kernel file to array
    //get kernel file full path
    std::string path=getenv("SPSFAST_PATH");
    std::stringstream ss;
    std::string kernel_full_path;
    ss<<path<<"/bin/"<<kernel_filename;
    ss>>kernel_full_path;
    //read it
    std::string sourceStr;
    status = convert_file_to_string(kernel_full_path, sourceStr);
    const char* source = sourceStr.c_str();
    size_t sourceSize[] = {strlen(source)};
    
    //Create program object
    this->program = clCreateProgramWithSource(this->context, 1, &source, sourceSize, &status);
    if (status!=0){
        std::cout<<"ERROR creating program: "<<status<<std::endl;
        exit(1);
    }
    
    //Build program, print log only if there was an error
    status=clBuildProgram(this->program,this->n_devices,this->devices,NULL,NULL,NULL);
    if (status!=0){
        std::cerr<<"ERROR building program: "<<status<<std::endl;
        //Print build log
        size_t logsize=0;
        clGetProgramBuildInfo(program,device,CL_PROGRAM_BUILD_LOG,0,NULL,&logsize);
        char* log = new char[logsize];
        clGetProgramBuildInfo(program,device,CL_PROGRAM_BUILD_LOG,logsize,log,NULL);
        std::cout<<"log:\n "<<log<<std::endl;
        delete[] log;
        exit(1);
    }
    return 0;
}


/*
 load a file into a string
 */
int spectrum_generator::convert_file_to_string(std::string infilename, std::string& str){
    //open file in binary
    std::ifstream infile(infilename.c_str(),  std::ios::binary |std::ios::ate);
    if(!infile.is_open()){
        std::cout<<"\nERROR CAN'T OPEN FILE: "<<infilename<<"\n"<<std::endl;
        exit(1);
    }
    
    //get the size of the file and resize container
    std::ifstream::pos_type size;
    size = infile.tellg();
    str.resize(size);
    
    //read file
    infile.seekg (0, std::ios::beg);
    infile.read((char*)(str.c_str()), size);
    
    return 0;
}


//////////////////////////////////////////////////

/*
 create buffers on devices
 */
int spectrum_generator::opencl_create_buffers(){
    //error variable
    cl_int status=0;
    
    //read only data
    resampled_model_d = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, resampled_model.size() * sizeof(cl_float),(void *) resampled_model.data(), &status);
    if (status!=0){
        std::cerr<<"ERROR creating buffers: "<<status<<std::endl;
        exit(1);
    }
    time_d = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, ntimesteps * sizeof(cl_float),(void *) time.data(), &status);
    if (status!=0){
        std::cerr<<"ERROR creating buffers: "<<status<<std::endl;
        exit(1);
    }
    wavel_d = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, mes_nspecsteps * sizeof(cl_float),(void *) mes_spec_wavel.data(), &status);
    if (status!=0){
        std::cerr<<"ERROR creating buffers: "<<status<<std::endl;
        exit(1);
    }
    mes_spec_d = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, mes_nspecsteps * sizeof(cl_float),(void *) mes_spec.data(), &status);
    if (status!=0){
        std::cerr<<"ERROR creating buffers: "<<status<<std::endl;
        exit(1);
    }
    mes_spec_err_d = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, mes_nspecsteps * sizeof(cl_float),(void *) mes_spec_err.data(), &status);
    if (status!=0){
        std::cerr<<"ERROR creating buffers: "<<status<<std::endl;
        exit(1);
    }
    mes_spec_mask_d = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, mes_nspecsteps * sizeof(cl_float),(void *) mes_spec_mask.data(), &status);
    if (status!=0){
        std::cerr<<"ERROR creating buffers: "<<status<<std::endl;
        exit(1);
    }
    
    
    //read and write
    model_d = clCreateBuffer(context, CL_MEM_READ_WRITE,	sizeof(cl_float) * ntimesteps * mes_nspecsteps , NULL, &status);
    if (status!=0){
        std::cerr<<"ERROR creating buffers: "<<status<<std::endl;
        exit(1);
    }
    result_no_vel_d = clCreateBuffer(context, CL_MEM_READ_WRITE,	sizeof(cl_float) * mes_nspecsteps , NULL, &status);
    if (status!=0){
        std::cerr<<"ERROR creating buffers: "<<status<<std::endl;
        exit(1);
    }
    result_d = clCreateBuffer(context, CL_MEM_READ_WRITE,	sizeof(cl_float) * mes_nspecsteps , NULL, &status);
    if (status!=0){
        std::cerr<<"ERROR creating buffers: "<<status<<std::endl;
        exit(1);
    }
    factor1_d = clCreateBuffer(context, CL_MEM_WRITE_ONLY,	sizeof(cl_float) * mes_nspecsteps , NULL, &status);
    if (status!=0){
        std::cerr<<"ERROR creating buffers: "<<status<<std::endl;
        exit(1);
    }
    factor2_d = clCreateBuffer(context, CL_MEM_WRITE_ONLY,	sizeof(cl_float) * mes_nspecsteps , NULL, &status);
    if (status!=0){
        std::cerr<<"ERROR creating buffers: "<<status<<std::endl;
        exit(1);
    }
    chi_d = clCreateBuffer(context,  CL_MEM_WRITE_ONLY,		sizeof(cl_float) * mes_nspecsteps , NULL, &status);		
    if (status!=0){
        std::cerr<<"ERROR creating buffers: "<<status<<std::endl;
        exit(1);
    }
    return 0;
}

//////////////////////////////////////////////////

/*
 create kernel objects
 */
int spectrum_generator::opencl_create_kernels(){
    //error variable
    cl_int status=0;
    
	// Create kernel objects
	this->kernel_spec_gen = clCreateKernel(this->program,"spec_gen", &status);
	if (status!=0){
		std::cerr<<"ERROR creating kernel_spec_gen: "<<status<<std::endl;
		exit(1);
	}
	this->kernel_vel_disp = clCreateKernel(this->program,"mask_veloc_disp", &status);
	if (status!=0){
		std::cerr<<"ERROR creating kernel_vel_disp: "<<status<<std::endl;
		exit(1);
	}
    this->kernel_get_factors = clCreateKernel(this->program,"get_factors", &status);
    if (status!=0){
        std::cerr<<"ERROR creating kernel_get_factors: "<<status<<std::endl;
        exit(1);
    }
	this->kernel_chi_calc = clCreateKernel(this->program,"chi_calculation", &status);
	if (status!=0){
		std::cerr<<"ERROR creating kernel_chi_calc: "<<status<<std::endl;
		exit(1);
	}	

	return 0;
}


//////////////////////////////////////////////////

/*
 Setting kernel arguments
 with the buffers on the GPU (device)
 this need not be done again, even if
 data changes the kernel will read the new data
 */
int spectrum_generator::set_initial_kernel_args()
{
	//error variable
	cl_int status=0;

	//not all kernel arguments are set now
	//some will be changed/calculated later
	//those will be set later

	//kernel_spec_gen
    cl_kernel kern = this->kernel_spec_gen;
	status = clSetKernelArg(kern, 0, sizeof(cl_mem), &this->resampled_model_d);
	status |= clSetKernelArg(kern, 1, sizeof(cl_mem), &this->time_d);
	status |= clSetKernelArg(kern, 2, sizeof(cl_mem), &this->wavel_d);

	status |= clSetKernelArg(kern, 3, sizeof(cl_mem), &this->mes_spec_d);
	status |= clSetKernelArg(kern, 4, sizeof(cl_mem), &this->mes_spec_err_d);
	status |= clSetKernelArg(kern, 5, sizeof(cl_mem), &this->mes_spec_mask_d);
		
	status |= clSetKernelArg(kern, 6, sizeof(cl_mem), &this->model_d);
	status |= clSetKernelArg(kern, 7, sizeof(cl_mem), &this->result_no_vel_d);
	status |= clSetKernelArg(kern, 8, sizeof(cl_mem), &this->factor1_d);
	status |= clSetKernelArg(kern, 9, sizeof(cl_mem), &this->factor2_d);
	
	status |= clSetKernelArg(kern, 10, sizeof(int), &this->mes_nspecsteps);
	status |= clSetKernelArg(kern, 11, sizeof(int), &this->ntimesteps);
	//error check
	if (status!=0){
		std::cerr<<"ERROR setting kernel spec_gen arguments: "<<status<<std::endl;
		exit(1);
	}


	//kernel_vel_disp
    kern = this->kernel_vel_disp;
	status = clSetKernelArg(kernel_vel_disp, 0, sizeof(cl_mem), &this->wavel_d);

	status |= clSetKernelArg(kern, 1, sizeof(cl_mem), &this->mes_spec_d);
	status |= clSetKernelArg(kern, 2, sizeof(cl_mem), &this->mes_spec_err_d);
	status |= clSetKernelArg(kern, 3, sizeof(cl_mem), &this->mes_spec_mask_d);
		
	status |= clSetKernelArg(kern, 4, sizeof(cl_mem), &this->result_no_vel_d);
	status |= clSetKernelArg(kern, 5, sizeof(cl_mem), &this->result_d);
	
	status |= clSetKernelArg(kern, 6, sizeof(int), &this->mes_nspecsteps);
	//error check
	if (status!=0){
		std::cerr<<"ERROR setting kernel vel_disp arguments: "<<status<<std::endl;
		exit(1);
	}

    //kernel_chi_calc
    kern = this->kernel_get_factors;
    status = clSetKernelArg(kern, 0, sizeof(cl_mem), &this->mes_spec_d);
    status |= clSetKernelArg(kern, 1, sizeof(cl_mem), &this->mes_spec_err_d);
    status |= clSetKernelArg(kern, 2, sizeof(cl_mem), &this->mes_spec_mask_d);
    status |= clSetKernelArg(kern, 3, sizeof(cl_mem), &this->result_d);
    status |= clSetKernelArg(kern, 4, sizeof(cl_mem), &this->factor1_d);
    status |= clSetKernelArg(kern, 5, sizeof(cl_mem), &this->factor2_d);
    //error check
    if (status!=0){
        std::cerr<<"ERROR setting kernel get factors arguments: "<<status<<std::endl;
        exit(1);
    }
    
    
	//kernel_chi_calc
    kern = this->kernel_chi_calc;
	status = clSetKernelArg(kern, 0, sizeof(cl_mem), &this->mes_spec_d);
	status |= clSetKernelArg(kern, 1, sizeof(cl_mem), &this->mes_spec_err_d);
	status |= clSetKernelArg(kern, 2, sizeof(cl_mem), &this->mes_spec_mask_d);
	status |= clSetKernelArg(kern, 3, sizeof(cl_mem), &this->result_d);
	status |= clSetKernelArg(kern, 4, sizeof(cl_mem), &this->chi_d);
	status |= clSetKernelArg(kern, 6, sizeof(cl_mem), &this->wavel_d);
	//error check
	if (status!=0){
		std::cerr<<"ERROR setting kernel chi calc arguments: "<<status<<std::endl;
		exit(1);
	}

	return 0;
}



/////////////////////////////////////////////////////////////////////////////
// functions during operation
/////////////////////////////////////////////////////////////////////////////

/*
 set model parameters
 */
int spectrum_generator::set_params( std::map<std::string,double>& parameters ){
    //set params if they are in the input map
    std::map<std::string,double>::iterator it;
    it= parameters.find("dust_tau_v");
    if(it != parameters.end()){
        this->dust_tau_v = parameters["dust_tau_v"];
    }
    it= parameters.find("dust_mu");
    if(it != parameters.end()){
        this->dust_mu = parameters["dust_mu"];
    }
    it= parameters.find("sfr_tau");
    if(it != parameters.end()){
        this->sfr_tau = parameters["sfr_tau"];
    }
    it= parameters.find("age");
    if(it != parameters.end()){
        this->age = parameters["age"];
    }
    it= parameters.find("metall");
    if(it != parameters.end()){
        this->metall = parameters["metall"];
    }
    it= parameters.find("vdisp");
    if(it != parameters.end()){
        this->vdisp = parameters["vdisp"];
    }
    
    //change kernel params too
    change_kernel_params();

	return 0;
}

/*
 update the kernel parameters
 */
int spectrum_generator::change_kernel_params(){
	//error variable
	cl_int status=0;

    //select metallicity model
    //the index of the model wich is lower than metall
    //this will be passed to the kernel:
	double model_metal[6]={0.0001,0.0004,0.004,0.008,0.02,0.05};
	int modelno;
	for(int j=0;j<6;j++){
		if(model_metal[j] > metall){
			modelno=j-1;
			break;
		}
	}

	//set  the kernel arguments that has changed
    cl_kernel kern = this->kernel_spec_gen;
	status = clSetKernelArg(kern, 12, sizeof(cl_float), &this->dust_tau_v);
	status |= clSetKernelArg(kern, 13, sizeof(cl_float), &this->dust_mu);
	status |= clSetKernelArg(kern, 14, sizeof(cl_float), &this->sfr_tau);
	status |= clSetKernelArg(kern, 15, sizeof(cl_float), &this->age);
	status |= clSetKernelArg(kern, 16, sizeof(cl_float), &this->metall);
	status |= clSetKernelArg(kern, 17, sizeof(int), &modelno);
	if (status!=0){
		std::cerr<<"ERROR setting kernel_spec_gen arguments: "<<status<<std::endl;
		exit(1);
	}
    
	status = clSetKernelArg(this->kernel_vel_disp, 7, sizeof(cl_float), &this->vdisp);
	if (status!=0){
		std::cerr<<"ERROR setting kernel_vel_disp arguments: "<<status<<std::endl;
		exit(1);
	}
    
	return 0;
}

/////////////////////////////////////////////////////////////////////////////

/*
 generate spectrum on device
 */
int spectrum_generator::generate_spectrum(){
	//error variable
	cl_int status=0;

	//generate spectrum (no velocity dispersion)
	size_t global_work_size[1] = {(size_t) mes_nspecsteps};
	status = clEnqueueNDRangeKernel(commandQueue, kernel_spec_gen, 1, NULL, global_work_size, NULL, 0, NULL,NULL);
	if (status!=0){
		std::cerr<<"ERROR running kernel_spec_gen: "<<status<<std::endl;
		exit(1);
	}

	//next kernel for velocity dispersion
	status = clEnqueueNDRangeKernel(commandQueue, kernel_vel_disp, 1, NULL, global_work_size, NULL, 0, NULL,NULL);
	if (status!=0){
		std::cerr<<"ERROR running kernel_vel_disp: "<<status<<std::endl;
        exit(1);
	}
    
    return 0;
}

/////////////////////////////////////////////////////////////////////////////

/*
 compare generated spectrum to measurement
 */
int spectrum_generator::compare_to_measurement(){
    //get factor to pull the generated spec to the measurement
    double scale_factor=get_factor_to_scale_spectra_to_measurement();
    
    //get factor to pull the generated spec to the measurement
    this->chi=get_chi_square(scale_factor);

	return 0;
}

/*
 calculate the scaliung factor to pull spectra to measurement
 */
double spectrum_generator::get_factor_to_scale_spectra_to_measurement(){
    //error variable
    cl_int status=0;
    
    //weighted squared errors
    std::vector<cl_float> factor1(this->mes_nspecsteps);
    std::vector<cl_float> factor2(this->mes_nspecsteps);
    
    //temp1, temp2 are sums of vectors factor1,factor2
    //they are use to calculate the "factor" that pulls together observed
    //and model spectra
    cl_float temp_1,temp_2,scale_factor;
    
    //Read the factors back to host memory
    status = clEnqueueReadBuffer(this->commandQueue, this->factor1_d, CL_TRUE, 0, this->mes_nspecsteps * sizeof(cl_float) , factor1.data(), 0, NULL,NULL);
    if (status!=0){
        std::cerr<<"ERROR reading buffer,from kernel_vel: "<<status<<std::endl;
        exit(1);
    }
    status = clEnqueueReadBuffer(this->commandQueue, this->factor2_d, CL_TRUE, 0, this->mes_nspecsteps * sizeof(cl_float) , factor2.data(), 0, NULL, NULL);
    if (status!=0){
        std::cerr<<"ERROR reading buffer,from kernel_vel: "<<status<<std::endl;
        exit(1);
    }
    
    //summing factors, to pull spectra together
    //summing is sequential on CPU paralell summing would an overkill for 3-4000 numbers i guess
    temp_1=0;
    temp_2=0;
    for(int i=0;i<this->mes_nspecsteps;i++){
        temp_1+=factor1[i];
        temp_2+=factor2[i];
    }
    scale_factor=temp_1/temp_2;
    
    return scale_factor;
}

/*
 calculate the sum of weighted squared errors
 */
double spectrum_generator::get_chi_square(double scale_factor){
    //error variable
    cl_int status=0;
    
    //weighted squared errors
    std::vector<cl_float> chis(this->mes_nspecsteps);
    
    //set scale factor for the kernel
    status |= clSetKernelArg(this->kernel_chi_calc, 5, sizeof(cl_float), &scale_factor);
    if (status!=0){
        std::cerr<<"ERROR setting kernel_chi_calc arguments: "<<status<<std::endl;
        exit(1);
    }
    
    //calculate weigthed squared errors
    size_t global_work_size[1] = {(size_t) this->mes_nspecsteps};
    status = clEnqueueNDRangeKernel(this->commandQueue, this->kernel_chi_calc, 1, NULL, global_work_size, NULL, 0, NULL, NULL);
    if (status!=0){
        std::cerr<<"ERROR running kernel_chi_calc: "<<status<<std::endl;
        exit(1);
    }
    
    //Read the errors to host memory.
    status = clEnqueueReadBuffer(commandQueue, chi_d, CL_TRUE, 0, mes_nspecsteps * sizeof(cl_float) , chis.data(), 0,NULL,NULL);
    if (status!=0){
        std::cerr<<"ERROR reading chi buffer: "<<status<<std::endl;
        exit(1);
    }
    
    //summing chi squares in host
    double chi=0;
    for(int i=0;i<mes_nspecsteps;i++){
        chi+=chis[i];
    }
    
    return chi;
    
}

/////////////////////////////////////////////////////////////////////////////

/*
 read from device to host and return the modeled spectrum
 */
std::vector<cl_float> spectrum_generator::get_result(){
    cl_int status=0;
    
    //result
    std::vector<cl_float> res(this->mes_nspecsteps);
    
    //read from device
    status = clEnqueueReadBuffer(this->commandQueue, this->result_d, CL_TRUE, 0, this->mes_nspecsteps * sizeof(cl_float) , res.data(), 0, NULL, NULL);
    if (status!=0){
        std::cout<<"ERROR reading buffer: "<<status<<std::endl;
        exit(1);
    }
    
    return res;
}


/////////////////////////////////////////////////////////////////////////////
// write results
/////////////////////////////////////////////////////////////////////////////


/*
 write spectra to tsv file
 */
int spectrum_generator::write_specs(std::vector< std::vector<cl_float> >& results,
                                    std::string out_fname){
    std::vector<std::vector <double> > output;
    output.push_back(std::vector<double>(this->mes_spec_wavel.begin(),this->mes_spec_wavel.end()));
    for (auto res : results){
        output.push_back(std::vector<double>(res.begin(),res.end()));
    }
    write_table_col(output,out_fname);
    return 0;
}

/*
int spectrum_generator::write_fit_result()
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
	return 0;
}
*/

/////////////////////////////////////////////////////////////////////////////
// clean up
/////////////////////////////////////////////////////////////////////////////

int spectrum_generator::clean_resources(){
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
    
    //delet platform and device arrays
    delete[] this->platforms;
    delete[] this->devices;
    
	return status;
}

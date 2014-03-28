#include "opencl_spec_gen.h"
#include "read.h"

#include <iostream>
#include <sstream>
#include <ctime>

#include <CL/cl.h>

#include <string>
#include <string.h>

#define SUCCESS 0
#define FAILURE 1

void opencl_spec_gen::cpy(read& input)
{
	wavelengths=input.wavelengths;
	nspecsteps=wavelengths.size();

	time=input.time;
	ntimesteps=time.size();
}


//gets dust parameters and modifies data
//
//gets dust_tauv, dust_mu and modifies model data
//calls dust_modif
//
int opencl_spec_gen::usr_get_dust()
{
	bool tryagain;	
	std::cout<<"\nDust"<<std::endl;
	do{
		tryagain=false;
		try
		{
			//get option
			std::string temp_line;
			std::cout<<"include attenuation by dust?: y/[n] ";
			getline(std::cin,temp_line);
			std::stringstream temp_sstr;
			temp_sstr<<temp_line;
			char option='0';
			temp_sstr>>option;
			
			//read parameters 
			if(option=='y' || option=='Y')
			{
				std::cout<<"Using parametrization described by Charlot & Fall (see documentation for more info)\n ";

				//get the line of ages
				std::cout<<"\nTau_v parameter: (default is: 1) ";
				std::string temp_line;
				getline(std::cin,temp_line);
				std::stringstream sstr;
				sstr<<temp_line;
				sstr>>dust_tau_v;
				//if no value was given we use default
				if (temp_line.size()==0) dust_tau_v=1;

				//get the line of ages
				std::cout<<"Mu parameter: (default is: 0.3 ) ";
				std::string temp_line1;
				getline(std::cin,temp_line1);
				std::stringstream sstr1;
				sstr1<<temp_line1;
				sstr1>>dust_mu;
				//if no value was given we use default
				if (temp_line1.size()==0) dust_mu=0.3;
				dust_option=0;

			}
			else if(option=='n' || option=='N')
			{
				dust_option=1;			
			}
			else
			{
				std::cout<<"\nERROR: WRONG OPTION\n";
				throw 1;
			}
		}
		catch (int ex)
		{
			if (ex==1){tryagain=true;}
		}
	}while(tryagain);

	return 0;
}

///
//sets dust parameters 
//calls dust_modif
//
void opencl_spec_gen::set_dust(double tau_v_value, double mu_value)
{
	dust_tau_v=tau_v_value;
	dust_mu=mu_value;
	dust_modif();
	return;
}


//gets the ages of galaxies
//
//either from a file or from stdin
//
int opencl_spec_gen::usr_get_age()
{
	bool tryagain;	
	std::cout<<"\nGetting age to convolve to"<<std::endl;
	do{
		tryagain=false;
		try
		{
			//get the line of ages
			std::cout<<"write the ages to convolve to ( in Gyrs): ";
			std::string temp_line;
			getline(std::cin,temp_line);
			std::stringstream sstr;
			sstr<<temp_line;
			std::string token;
			double temp_d;
			if(sstr>>temp_d)
				age=temp_d*1e+9;
			else 
				throw 1;
		}
		catch (int ex)
		{
			if (ex==1){tryagain=true;}
		}
	}while(tryagain);

	return 0;
}

void opencl_spec_gen::set_age(double age_value)
{
	age=age_value*1e+9;
}

//gets sfr parameters
//
int opencl_spec_gen::usr_get_sfr()
{
	std::cout<<"\nGetting star formation rate"<<std::endl;

	bool tryagain;
	do
	{
		tryagain=false;
		try
		{
			std::cout<<"Give the Tau of the exponential star formation rate, in Gy: ";
			std::string temp_line;
			getline(std::cin,temp_line);
			std::stringstream temp_sstr;
			temp_sstr<<temp_line;
			double temp_d=0;
			if(temp_sstr>>temp_d)
				sfr_tau=temp_d*1e+9;
			else
				throw 1;			

		}catch(int e)
		{
		if (e==1) tryagain=true;
		}

	}while(tryagain);

	return 0;
}


void opencl_spec_gen::set_sfr(double sfr_tau_value)
{
	sfr_tau=sfr_tau_value*1e+9;
}

//returns star formation rate
//
double opencl_spec_gen::sfr(double time, double tau)
{
 	return exp(-time/tau);
}

int opencl_spec_gen::usr_get_imf()
{
	std::cout<<"\nGetting initial mass function"<<std::endl;

	bool tryagain;
	do
	{
		tryagain=false;
		try
		{
			std::cout<<"Choose the IMF to use: [chabrier/sapleter] :";
			std::string temp_line;
			getline(std::cin,temp_line);
			std::stringstream temp_sstr;
			temp_sstr<<temp_line;
			if(!(temp_sstr>>imf))
				throw 1;

			if(imf != "salpeter" && imf!="chabrier")
				throw 1;

		}catch(int e)
		{
			if (e==1) tryagain=true;
		}
		

	}while(tryagain);

	return 0;
}

//gets metallicity parameter
//
int opencl_spec_gen::usr_get_metall()
{
	std::cout<<"\nGetting metallicity"<<std::endl;

	bool tryagain;
	do
	{
		tryagain=false;
		try
		{
			std::cout<<"Give the metallicity (Z) of the star population: (range: ( 0.0001< z <0.05 )";
			std::string temp_line;
			getline(std::cin,temp_line);
			std::stringstream temp_sstr;
			temp_sstr<<temp_line;
			double temp_d=0;
			if( temp_sstr>>temp_d )
				metall=temp_d;
			else
				throw 1;
			if(metall<0.0001 || metall>0.05)
				throw 2;

		}catch(int e)
		{
			if (e==1) 
			{
				tryagain=true;
				std::cout<<"running again\n";
			}
			if (e==2) 
			{
				tryagain=true;
				std::cout<<"ERROR metallicity is out of range!"<<std::endl;
			}
		}
		

	}while(tryagain);

	return 0;
}

//problem on the edges!!!!!!!!!!!!!!!
int opencl_spec_gen::metall_interpol(std::vector<std::vector<double> >& original_models)
{
	int offset;

	if( imf =="chabrier")
		offset=0;
	else if(imf=="salpeter")
		offset=6;

	if (metall!=0.05)
	{
		double model_metal[6]={0.0001,0.0004,0.004,0.008,0.02,0.05};
	
		int intpol_model;
		for(int i=0;i<6;i++)
		{
			if(model_metal[i] > metall)
			{
				intpol_model=i-1;
				break;
			}
		}

		double wd,wu,delta;

		//interpolating
	
		delta=log(model_metal[intpol_model+1] / model_metal[intpol_model]);
		wd=log(model_metal[intpol_model+1]/metall) / delta;
		wu=log(metall/model_metal[intpol_model]) / delta;

		model.resize(original_models[offset].size());
		//interpolating models around given metallicity

		for(int i=0;i<original_models[offset].size();i++)
		{
			model[i]= wd * original_models[offset+intpol_model][i] +  wu * original_models[offset+intpol_model+1][i] ;
		}
	}
	else 
	{
		//if metall==0.5 there is no upper model to interpolate with
		model.resize(original_models[offset].size());

		for(int i=0;i<original_models[offset].size();i++)
		{
			model[i]= original_models[offset+5][i];
		}
	}
	
	return 0;

}

int opencl_spec_gen::chose_model(std::vector<std::vector<double> >& original_models, int no_model)
{

	//copying

	model.resize(original_models[no_model].size());
	//interpolating models around given metallicity

	for(int i=0;i<original_models[no_model].size();i++)
	{
		model[i]= original_models[no_model][i];
	}

	return 0;

}

///
//modifies model, due to dust
//
void opencl_spec_gen::dust_modif()
{
	if(dust_option==1) return;

	//Dust factors:
	double dust_tau; 			// tau_v or mu*tau_v
	double exponent; 			// lambda/5500

	//mu is 0.3 by default in C-B
	//tau_v is 1 by default in C-B

	for(int j=0;j<nspecsteps;j++)
	{
		// exponent is const for a wavelength
		exponent=pow(wavelengths[j]/5500,-0.7);
	
		for(int i=0;i<ntimesteps;i++)
		{
			//dust_tau depends on the age of the stellar pop
			if(time[i]<1e7)
			{
				dust_tau= -dust_tau_v;
			}	
			if(time[i]>1e7)
			{
				dust_tau= -dust_mu*dust_tau_v;
			}
			
			//the modification
			model[j+i*(nspecsteps)] *= exp(dust_tau*exponent);
		}
	}
	return;
} 

//writes the result in a table format
//from stdin input filename
//
//separated with whitespaces
//no " " at the end of file
//no /n at theend of file
//
int opencl_spec_gen::usr_write_result()
{
	bool tryagain;  // indicator for loop
	int err;

	std::cout<<"\nWriting results "<<std::endl;
	do
	{
		//setting indicator
		tryagain=false;
	
		try
		{
			std::string temp_line;
			//getting output filename
			std::cout<<"\noutput filename: ";
			getline(std::cin,temp_line);
			std::stringstream temp_sstr;
			temp_sstr<<temp_line;
			std::string outfilename;
			temp_sstr>>outfilename;
	
			err= write_result(outfilename);
			if( err != 0 )
				throw 1;
		
		}catch(int error)
		{
			if(error==1)	
			{
				tryagain=true;
				std::cout<<"ERROR\n";
			}
	
		}

	}while(tryagain);
	
	return 0;
}

///
//writes the result in a table format
//
//separated with whitespaces
//no " " at the end of file
//no /n at theend of file
//
int opencl_spec_gen::write_result(std::string outfilename)
{
	int err;
	err=write_vector(conv_to_age_res,outfilename);
	if (err==0)
		std::cout<<"spectrum written in: '"<<outfilename<<"'"<<std::endl;

	std::stringstream temp_sstr;
	temp_sstr<<outfilename<<"_w";
	temp_sstr>>outfilename;

	std::vector<std::vector<double> > vecvec;
	vecvec.push_back(wavelengths);
	vecvec.push_back(conv_to_age_res);
	err |= write_table_col(vecvec,outfilename);
	if (err==0)
		std::cout<<"wavelengths and spectrum written in: '"<<outfilename<<"'"<<std::endl;
	
	return err;
}


///
//writes a vector to a file
//
//one column
int opencl_spec_gen::write_vector(std::vector<double>& vec_to_write, std::string outfilename)
{
	std::ofstream outfile(outfilename.c_str());
	//Checking filename
	if(!(outfile))
	{
		std::cout<<"ERROR INVALID OUTPUT FILE: "<<outfilename<<std::endl;
		return 1;
	}

	//writing numbers 
	for(int j=0; j<vec_to_write.size(); j++)
    {
		outfile<<vec_to_write[j]<<"\n";
    }
	outfile.close();

	//info out
	std::cout<<"writing succesful: "<<std::endl;

	return 0;
}

///
//writes a table (vector of vectors) to a file
//
//the outer vectors are in rows
//
int opencl_spec_gen::write_table_row(std::vector< std::vector<double > >& table_to_write, std::string outfilename)
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

///
//writes a table (vector of vectors) to a file
//
//the outer vectors are in columns
//each vector should be of same length
//
int opencl_spec_gen::write_table_col(std::vector< std::vector<double > >& table_to_write, std::string outfilename)
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


//The c++ version of the convolution:
//
//to only one age
//

int opencl_spec_gen::conv_to_age()
{
	//setting the size of the result vec
	conv_to_age_res.resize(nspecsteps); 

	for(int i=0;i<nspecsteps;i++)
	{
		double temp=0;		
		for(int j=1; (j < ntimesteps) && (time[j]<=age) ;j++) 
		{
			//the actual integrating
			temp+= (time[j]-time[j-1] ) * model[i+(j-1)*nspecsteps] * sfr(age-time[j],sfr_tau);
		}
		//tau is a contant so we only divide at the end
		conv_to_age_res[i]=temp/sfr_tau;
	}

	return 0;
}


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

int opencl_spec_gen::opencl_conv_to_age(std::vector<double>&  model_cont)
{
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
	const char *filename = "spec_gen.cl";
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
	int nspecsteps=wavelengths.size();
	int ntimesteps=time.size();

	//data
	cl_mem model_without_dust_d = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, model_cont.size() * sizeof(double),(void *) model_cont.data(), &status);
	cl_mem time_d = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, ntimesteps * sizeof(double),(void *) time.data(), &status);
	cl_mem wavel_d = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, nspecsteps * sizeof(double),(void *) wavelengths.data(), &status);

	//buffers to write
	cl_mem model_d = clCreateBuffer(context, CL_MEM_READ_WRITE,		sizeof(double) * ntimesteps * nspecsteps , NULL, &status);
	cl_mem result_d = clCreateBuffer(context, CL_MEM_READ_WRITE,	sizeof(double) * nspecsteps , NULL, &status);	
	if (status!=0)
	{
		std::cout<<"ERROR creating buffers: "<<status<<std::endl;
	}

	/*Step 8: Create kernel objects */
	cl_kernel kernel = clCreateKernel(program,"spec_gen", &status);
	if (status!=0)
	{
		std::cout<<"ERROR creating kernel: "<<status<<std::endl;
	}

	std::vector <double> result (nspecsteps);
		
	/*Setting kernel arguments*/

	//kernel1
	status = clSetKernelArg(kernel, 0, sizeof(cl_mem), &model_without_dust_d);
	status |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &time_d);
	status |= clSetKernelArg(kernel, 2, sizeof(cl_mem), &wavel_d);
		
	status |= clSetKernelArg(kernel, 3, sizeof(cl_mem), &model_d);
	status |= clSetKernelArg(kernel, 4, sizeof(cl_mem), &result_d);

	status |= clSetKernelArg(kernel, 5, sizeof(int), &nspecsteps);
	status |= clSetKernelArg(kernel, 6, sizeof(int), &ntimesteps);


	if (status!=0)
		std::cout<<"ERROR setting kernel arguments: "<<status<<std::endl;

	//models imf 
	int offset;
	if( imf == "chabrier")
		offset=0;
	else if(imf=="salpeter")
		offset=6;
	

	//finding models to interpolate metall
	double model_metal[6]={0.0001,0.0004,0.004,0.008,0.02,0.05};
	int modelno;
	for(int j=0;j<6;j++)
	{
		if(model_metal[j] > metall)
		{
			modelno=j-1+offset;
			break;
		}
	}

	/*Step 9: Set Kernel arguments.*/
	
//already reached max no of constant arguments...
	status = clSetKernelArg(kernel, 7, sizeof(double), &dust_tau_v);
	status |= clSetKernelArg(kernel, 8, sizeof(double), &dust_mu);
	status |= clSetKernelArg(kernel, 9, sizeof(double), &sfr_tau);
	status |= clSetKernelArg(kernel, 10, sizeof(double), &age);
	status |= clSetKernelArg(kernel, 11, sizeof(double), &metall);
	status |= clSetKernelArg(kernel, 12, sizeof(int), &modelno);
	if (status!=0)
		std::cout<<"ERROR setting kernel arguments: "<<status<<std::endl;
	

	cl_event kernel_event[1];
	/*Step 10: Running the kernel.*/
	size_t global_work_size[1] = {nspecsteps};
	status = clEnqueueNDRangeKernel(commandQueue, kernel, 1, NULL, global_work_size, NULL, 0, NULL, &kernel_event[0]);
	if (status!=0)
		std::cout<<"ERROR running kernel: "<<status<<std::endl;
	
	conv_to_age_res.resize(nspecsteps);
	/*Step 11: Read the result back to host memory.*/
	status = clEnqueueReadBuffer(commandQueue, result_d, CL_TRUE, 0, nspecsteps * sizeof(double) , conv_to_age_res.data(), 0, NULL, NULL);
	if (status!=0)
	{
		std::cout<<"ERROR reading buffer: "<<status<<std::endl;
	}

	/*Step 12: Clean the resources.*/
	status = clReleaseKernel(kernel);				//Release kernels.	

	status = clReleaseProgram(program);				//Release the program object.

	status = clReleaseMemObject(model_without_dust_d); //Release mem objects.
	status = clReleaseMemObject(time_d);		
	status = clReleaseMemObject(wavel_d);
	status = clReleaseMemObject(result_d);
	status = clReleaseMemObject(model_d);	

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

	return 0;
}
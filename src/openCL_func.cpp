#include "cb_data.h"
#include "table.h"

#include <sstream> 


//header for openCL
#include <CL/cl.hpp>

#include <string.h>
#include <string>

#define SUCCESS 0
#define FAILURE 1

using namespace std;

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
	cout<<"Error: failed to open file\n:"<<filename<<endl;
	return FAILURE;
}



//the OpenCl convolution:
int cb_data::opencl_convolve()
{
	//rows and columns have to be given
	conv_result.rows=nspecsteps;
	conv_result.columns=n_ages;
 
	//the whole result vector needs to be allocated so enqueuebuffread can write on it
	conv_result.container.resize(n_ages*nspecsteps); 
	
	/*Step1: Getting platforms and choose an available one.*/
	cl_uint numPlatforms;				//the NO. of platforms
	cl_platform_id* platforms = NULL; 	//id of available platforms
	cl_platform_id 	platform = NULL;	//id of the chosen platform


	//getting NO. of platforms
	cl_int	status = clGetPlatformIDs(0, NULL, &numPlatforms); 
	if (status != CL_SUCCESS)
	{
		cout << "Error: Getting platforms!" << endl;
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

	/*Step 3: Create context.*/
	cl_context context = clCreateContext(NULL,1, devices,NULL,NULL,NULL);
	
	/*Step 4: Creating command queue associate with the context.*/
	cl_command_queue commandQueue = clCreateCommandQueue(context, device, 0, NULL);

	/*Step 5: Create program object */
	const char *filename = "conv_opencl.cl";
	string sourceStr;
	status = convertToString(filename, sourceStr);
	const char *source = sourceStr.c_str();
	size_t sourceSize[] = {strlen(source)};
	cl_program program = clCreateProgramWithSource(context, 1, &source, sourceSize, NULL);
	
	/*Step 6: Build program. */
	status=clBuildProgram(program, 1,devices,NULL,NULL,NULL);
	
	/*Step 7: Allocate memory */
	cl_mem rawdata_d = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, rawdata.size() * sizeof(double),(void *) rawdata.data(), NULL);
	cl_mem time_d = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, ntimesteps * sizeof(double),(void *) time.data(), NULL);
	cl_mem age_vector_d = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, n_ages * sizeof(double) , (void *) ages.data(), NULL);
	cl_mem outputBuffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY , nspecsteps * n_ages * sizeof(double) , NULL, NULL);

	/*Step 8: Create kernel object */
	cl_kernel kernel = clCreateKernel(program,"conv_opencl", NULL);

	/*Step 9: Set Kernel arguments.*/
	status = clSetKernelArg(kernel, 0, sizeof(cl_mem), &rawdata_d);
	status = clSetKernelArg(kernel, 1, sizeof(cl_mem), &time_d);
	status = clSetKernelArg(kernel, 2, sizeof(cl_mem), &outputBuffer);
	status = clSetKernelArg(kernel, 3, sizeof(cl_mem), &age_vector_d);
	status = clSetKernelArg(kernel, 4, sizeof(int), &nspecsteps);
	status = clSetKernelArg(kernel, 5, sizeof(int), &ntimesteps);
	status = clSetKernelArg(kernel, 6, sizeof(int), &offset);
	status = clSetKernelArg(kernel, 7, sizeof(int), &nfillingdata);
	status = clSetKernelArg(kernel, 8, sizeof(double), &tau);
	status = clSetKernelArg(kernel, 9, sizeof(int), &n_ages);

	/*Step 10: Running the kernel.*/

	size_t global_work_size[1] = {static_cast<size_t>(nspecsteps)};
	status = clEnqueueNDRangeKernel(commandQueue, kernel, 1, NULL, global_work_size, NULL, 0, NULL, NULL);
	
	/*Step 11: Read the result back to host memory.*/
	status = clEnqueueReadBuffer(commandQueue, outputBuffer, CL_TRUE, 0, n_ages  * nspecsteps * sizeof(double) , conv_result.container.data(), 0, NULL, NULL);

	/*Step 12: Clean the resources.*/

	status = clReleaseKernel(kernel);				//Release kernel.
	status = clReleaseProgram(program);				//Release the program object.
	status = clReleaseMemObject(rawdata_d);			//Release mem objects.
	status = clReleaseMemObject(time_d);
	status = clReleaseMemObject(outputBuffer);
	status = clReleaseMemObject(age_vector_d);
	status = clReleaseCommandQueue(commandQueue);	//Release  Command queue.
	status = clReleaseContext(context);				//Release context.

	delete[] devices;

	std::cout<<"Passed!\n";
	return SUCCESS;
} 

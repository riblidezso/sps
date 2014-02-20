#include <iostream>

#include "cb_data.h"
#include "table.h"


int main()
{
	cb_data model;

	//reads model from an ASCII file
	model.read_buff_atof();

	//gets some important data from model
	model.get_data_from_model();

	//gets dust parameters and modify data
	model.dust();

	//gets the ages of galaxies
	model.get_age();

	//gets sfr parameters
	model.get_sfr();

	//The openCL convolution:
	model.opencl_convolve();	

	//writes the result in a table format
	model.write_convresult();

	return 0;
}

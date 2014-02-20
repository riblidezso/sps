#include <iostream>

#include "cb_data.h"
#include "table.h"


int main()
{
	cb_data model;

	//Reading model from an ASCII file
	model.read_buff_atof();

	//gets some important data from model
	model.get_data_from_model();

	//gets dust parameters and modify data
	model.dust();

	//gets the ages of galaxies
	model.get_age();

	//gets sfr parameters
	model.get_sfr();

	//The c++ version of the convolution:
	model.conv_to_age_vector();	

	//writes the result in a table format
	model.write_convresult();

	return 0;
}

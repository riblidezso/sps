#include <iostream>
#include <cstring>

#include "cb_data.h"
#include "table.h"


int main(int argc, char** argv)
{
	//Quick test mode check
	if(argc>1)
	{
		if(strlen(argv[1])==2)
		{
			if(argv[1][0]=='-' && argv[1][1]=='t')
			{
				int test_err=0;
				cb_data test_model;
				test_err=test_model.quick_test();
				if (test_err==1)
				{
					std::cout<<"ERROR something went wrong"<<std::endl;
					return 1;
				}
				else 
				{
					std::cout<<"quick test passed"<<std::endl;
					return 0;
				}
			}
		}
	}

	cb_data model;

	//Reading model from an ASCII file
	model.usr_read_buff_atof();

	//gets dust parameters and modify data
	model.usr_get_dust();

	//gets the ages of galaxies
	model.usr_get_age();

	//gets sfr parameters
	model.usr_get_sfr();

	//The c++ version of the convolution:
	model.conv_to_age_vector();	

	//writes the result in a table format
	model.usr_write_convresult();

	return 0;
}
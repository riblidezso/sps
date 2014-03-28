#include <iostream>

#include "read.h"
#include "opencl_spec_gen.h"


int main()
{
	//reading models
	read models;
	models.read_time_bin();
	models.read_wavel_bin();
	models.read_model_bin_all_cont();

	opencl_spec_gen generator;

	generator.cpy(models);

	generator.usr_get_dust();
	generator.usr_get_sfr();
	generator.usr_get_age();

	generator.usr_get_imf();
	generator.usr_get_metall();

	generator.opencl_conv_to_age(models.model_cont);

	generator.usr_write_result();

//	std::cin.get();

	return	0;

}
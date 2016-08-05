#include "read.h"
#include "spec_gen.h"

int main()
{
	read models;

	models.read_model_bin_all();
	models.read_time_bin();
	models.read_wavel_bin();

	spec_gen generator;

	generator.cpy(models);

	generator.usr_get_dust();
	generator.usr_get_sfr();
	generator.usr_get_age();

	generator.usr_get_imf();
	generator.usr_get_metall();

	generator.metall_interpol(models.models);

//	generator.chose_model(models.models,6);
	generator.dust_modif();

	generator.conv_to_age();

	generator.usr_write_result();

//	std::cin.get();
	return 0;

}
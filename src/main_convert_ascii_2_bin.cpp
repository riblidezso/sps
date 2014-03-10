#include <cstring>
#include <iostream>
#include <sstream>

#include "convert_ascii_2_bin.h"

#define CHAB_PATH "../input/chab_hr/" 
#define SALP_PATH "../input/salp_hr/" 
#define EXTENSION ".ised_ASCII"

#define OUTPATH "../input/bin/"
#define NO_METALL 6


int main(int argc, char** argv)
{
	std::cout<<"Converting ASCII models to binary..."<<std::endl;

	convert_ascii_2_bin conversion;

	int err;
	char metall;
	std::string inpath;
	std::string outpath;
	std::string filename;
	std::string imf;
	std::string extension;

	//converting models with different IMF 
	outpath=OUTPATH;
	extension=EXTENSION;

	//first path:
	inpath=CHAB_PATH;
	imf="chab";

	for(int i=0;i<2;i++)
	{
		//converting models with different metallicity
		for(int j=0;j<NO_METALL;j++)
		{
			//setting filename
			std::stringstream infilename_sstr;
			metall='2'+j;
			infilename_sstr<<inpath<<"bc2003_hr_m"<<metall<<"2_"<<imf<<"_ssp"<<extension;
			infilename_sstr>>filename;

			//reading and conversion
			err=conversion.read_buff_atof(filename);
			if (err==1)
			{
				std::cout<<"ERROR: could not find: '"<<filename<<"'"<<std::endl;
				return 1;
				std::cin.get();
			}
			conversion.get_data_from_raw(imf);
			
			//writing binary time
			//if there is already a time bin file,
			//we just simply overwrite it
			std::stringstream time_filename_sstr;
			time_filename_sstr<<outpath<<"time.bin";
			time_filename_sstr>>filename;
			conversion.write_vector_bin(conversion.time,filename);
			
			//writing binary wavelength
			std::stringstream wavel_filename_sstr;
			wavel_filename_sstr<<outpath<<"wavel.bin";
			wavel_filename_sstr>>filename;
			conversion.write_vector_bin(conversion.wavelengths, filename);	

			//writing binary model
			std::stringstream outfilename_sstr;
			outfilename_sstr<<outpath<<imf<<metall<<".bin";
			outfilename_sstr>>filename;
			conversion.write_vec_vec_bin(conversion.model, filename);
		
		}
		//second path:
		inpath=SALP_PATH;
		imf="salp";
	}
	return 0;
}
#include "read.h"


///
//Reading all models from binary file 
//
int read::read_model_bin_all_cont()
{
	#define NO_METALL 6
	#define PATH "../input/bin/"

	char metall;
	std::string path;
	std::string imf;
	std::string filename;
	
	imf="chab";
	path=PATH;
	models.resize(2*NO_METALL);

	for(int i=0;i<2;i++)
	{
		for(int j=0;j<NO_METALL;j++)
		{
			metall='2'+j;
			//reading binary model
			std::stringstream filename_sstr;
			filename_sstr<<path<<imf<<metall<<".bin";
			filename_sstr>>filename;
			read_model_bin_cont( filename);
		}
		//second path:
		imf="salp";
	}
	std::cout<<"models read"<<std::endl;
	return 0;
}


///
//Reading all models from binary file 
//
int read::read_model_bin_all()
{
	#define NO_METALL 6
	#define PATH "../input/bin/"

	char metall;
	std::string path;
	std::string imf;
	std::string filename;
	
	imf="chab";
	path=PATH;
	models.resize(2*NO_METALL);

	for(int i=0;i<2;i++)
	{
		for(int j=0;j<NO_METALL;j++)
		{
			metall='2'+j;
			//reading binary model
			std::stringstream filename_sstr;
			filename_sstr<<path<<imf<<metall<<".bin";
			filename_sstr>>filename;
			read_model_bin(i*NO_METALL+j, filename);
		}
		//second path:
		imf="salp";
	}
	std::cout<<"models read"<<std::endl;
	return 0;
}

///
//Reading one model from binary file 
//
int read::read_model_bin(int i, std::string infilename)
{
	std::ifstream infile(infilename.c_str(), std::ios::binary |std::ios::ate);
	//	CHECKING FILENAME
	if(!(infile))
	{
		std::cout<<"ERROR CAN'T OPEN FILE: "<<infilename<<std::endl;
		return 1;
	}
	std::ifstream::pos_type size;
	size = infile.tellg();

	models[i].resize(size/sizeof(double));
	
	infile.seekg (0, std::ios::beg);
	infile.read (reinterpret_cast<char*>(&models[i][0]), size);
	infile.close();
	std::cout<<"'"<<infilename<<"'  model read"<<std::endl;

	return 0;
}

///
//Reading one model from binary file 
//
int read::read_model_bin_cont( std::string infilename)
{
	std::ifstream infile(infilename.c_str(), std::ios::binary |std::ios::ate);
	//	CHECKING FILENAME
	if(!(infile))
	{
		std::cout<<"ERROR CAN'T OPEN FILE: "<<infilename<<std::endl;
		return 1;
	}
	std::ifstream::pos_type size;
	size = infile.tellg();

	size_t begin=model_cont.size();
	model_cont.resize(begin + size/sizeof(double));
	
	infile.seekg (0, std::ios::beg);
	infile.read (reinterpret_cast<char*>(&model_cont[begin]), size);
	infile.close();
	std::cout<<"'"<<infilename<<"'  model read"<<std::endl;

	return 0;
}


///
//Reading wavelengths from binary file 
//
int read::read_wavel_bin()
{
	#define PATH "../input/bin/"

	std::string path;
	std::string filename;

	path=PATH;
	std::stringstream filename_sstr;
	filename_sstr<<path<<"wavel.bin";
	filename_sstr>>filename;

	std::ifstream infile(filename.c_str(), std::ios::binary |std::ios::ate);
	//	CHECKING FILENAME
	if(!(infile))
	{
		std::cout<<"ERROR CAN'T OPEN FILE: "<<filename<<std::endl;
		return 1;
	}
	std::ifstream::pos_type size;
	size = infile.tellg();

	wavelengths.resize(size/sizeof(double));
	
	infile.seekg (0, std::ios::beg);
	infile.read (reinterpret_cast<char*>(&wavelengths[0]), size);
	infile.close();

	std::cout<<"times read"<<std::endl;
	return 0;
}

///
//Reading timesteps from binary file 
//
int read::read_time_bin()
{
	#define PATH "../input/bin/"

	std::string path;
	std::string filename;

	path=PATH;
	std::stringstream filename_sstr;
	filename_sstr<<path<<"time.bin";
	filename_sstr>>filename;

	std::ifstream infile(filename.c_str(), std::ios::binary |std::ios::ate);
	//	CHECKING FILENAME
	if(!(infile))
	{
		std::cout<<"ERROR CAN'T OPEN FILE: "<<filename<<std::endl;
		return 1;
	}
	std::ifstream::pos_type size;
	size = infile.tellg();

	time.resize(size/sizeof(double));
	
	infile.seekg (0, std::ios::beg);
	infile.read (reinterpret_cast<char*>(&time[0]), size);
	infile.close();

	std::cout<<"wavelenghts read"<<std::endl;
	return 0;
}

///
//reading spectrum to fit
//
int read::usr_read_sample()
{
	bool tryagain;	
	std::cout<<"\nGetting spectrum to fit"<<std::endl;
	do{
		tryagain=false;
		try
		{
			// get filename
			/*
			modif for testing!

			std::cout<<"input filename: ";
			std::string temp_line;
			getline(std::cin,temp_line);
			std::stringstream temp_sstr;
			temp_sstr<<temp_line;
			
			temp_sstr>>infilename;
			*/
			std::string infilename="../output/fm";

			std::ifstream infile(infilename.c_str());
		
			// check filename
			if(!(infile))										
			{
				std::cout<<"\nERROR CAN'T OPEN FILE: "<<infilename<<"\n"<<std::endl;
				throw 1;
			}
			
			// read spectrum
			std::cout<<"reading file: "<<infilename<<std::endl;
			std::string str;
			double temp;
			while(infile>>str)
			{
				std::stringstream s;
				s<<str;
				if(s>>temp)
				{
					sample_spec.push_back(temp);
		       	}
			}
			infile.close();
		
			//check no of ages
			if(sample_spec.size()==0)
			{
				std::cout<<"\nERROR: no spectrum read\n"<<std::endl;
				throw 1;
			}
		}catch (int ex)
		{
			if (ex==1){tryagain=true;}
		}
	}while(tryagain);

	std::cout<<"spectrum read\n";
	return 0;
}

int read::usr_read_sdss_csv()
{
	bool tryagain;	
	std::cout<<"\nGetting sdss csv spectrum to fit"<<std::endl;
	
	do{
		tryagain=false;
		try
		{
			// get filename
			std::cout<<"input filename: ";
			std::string temp_line,templ2;
			getline(std::cin,temp_line);
			std::stringstream temp_sstr;
			temp_sstr<<temp_line;
		
			std::string infilename;
			temp_sstr>>infilename;
		
			std::ifstream infile(infilename.c_str());
		
			// check filename
			if(!(infile))										
			{
				std::cout<<"\nERROR CAN'T OPEN FILE: "<<infilename<<"\n"<<std::endl;
				throw 1;
			}
			
			// read spectrum
			std::cout<<"reading file: "<<infilename<<std::endl;
			int row=0;
			double temp=0;
			double temp2=0;
			while(getline(infile,temp_line))
			{
				//gagyi
				for(int j=0;j<temp_line.size();j++)
				{
					if (temp_line[j]==',')
					{
						temp_line[j]=' ';
					}
				}

				std::stringstream temp_sstr;
				temp_sstr<<temp_line;

				if (temp_line[0] != '#' )
				{
					temp_sstr>>temp;
					mes_spec_wavel.push_back(temp);
					temp_sstr>>temp;
					mes_spec.push_back(temp);
					temp_sstr>>temp;
					mes_spec_err_l.push_back(temp);
					temp_sstr>>temp;
					mes_spec_err_h.push_back(temp);
					temp_sstr>>temp;
					mes_spec_mask.push_back(temp);
				}
				row++;
			}
			infile.close();

		}catch (int ex)
		{
			if (ex==1){tryagain=true;}
		}
	}while(tryagain);

	std::cout<<"spectrum read\n";
	return 0;
}
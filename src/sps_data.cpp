#include "sps_data.h"

/*
    read the binary sps model data
*/
int sps_data::read_binary_sps_model(){
    
    //read time data
    if( this->read_time_bin() != 0 ){
        exit(1);
    }
    //read wavelength data
    if( this->read_wavel_bin() != 0 ){
        exit(1);
    }
    //read models
    if( this->read_model_bin_all_cont() != 0 ){
        exit(1);
    }
    
    return 0;
}


/*
    read the binary sps model data
 */
int sps_data::read_measurement(std::string input_fname){
    if(input_fname!="none"){
        this->read_sdss_measurement(input_fname);
    }
    else{
        this->usr_read_sdss_csv();
    }
    
    return 0;
}


/*
    resamples all the models to the wavelengths of the measurement
 */
int sps_data::resample_models_2_mes(std::string imf)
{
    //calculate model offset from imf
    int offset=0;
    if( imf == "chabrier")
        offset=0;
    else if(imf=="salpeter")
        offset=6;
    
    int nspecsteps = (int) this->wavelengths.size();
    int ntimesteps = (int) this->time.size();
    int mes_nspecsteps = (int) this->mes_spec_wavel.size();
    
    //resize the vector that will store the resampled model
    //this is a huge contigous vector because this makes it
    //easy to pass it to GPU (only have to give pointer and size)
    resampled_model_cont.resize(ntimesteps * mes_nspecsteps * 6);
    
    //wd, wu are weigths for interpolation, delta is wavelength distance
    double wd,wu,delta;
    
    //these will store temporary positions
    int low,high,place,place1;
    high=0;
    
    
    //the resampling
    //loop over measurement points
    for(int i=0;i<mes_nspecsteps;i++){
        //find nearest model wavelength points
        
        //find the first model wavelength that is bigger
        //than measurement wavelength
        while(mes_spec_wavel[i] > this->wavelengths[high]){
            high++;
            //there might be problems if the one measured data wavelength is
            //larger than last model data, but this seems unlikely
            if (high==nspecsteps){
                std::cerr<<"\nERROR measurement wavelength"<<mes_spec_wavel[i]<<"\n";
                std::cerr<<"can't be interpolated from model wavelength points\n\n";
                return 1;
            }
        }
        
        //there might be problems if the first measured wavelength is
        //smaller than first model data, but this seems also unlikely
        if (high==0){
            std::cerr<<"\nERROR measurement wavelength"<<mes_spec_wavel[i]<<"\n";
            std::cerr<<"can't be interpolated from model wavelength points\n\n";
            return 1;
        }
        
        //the one before the first bigger is definitely smaller
        low=high-1;
        
        //calculate distance and weights
        delta=this->wavelengths[high] - this->wavelengths[low];
        wd= (this->wavelengths[high]-mes_spec_wavel[i])/delta;
        wu= (mes_spec_wavel[i]- this->wavelengths[low])/delta;
        
        //the intepolation
        //loop over different metallicity models
        for(int k=offset;k<offset+6;k++){
            //loop over timesteps
            for(int j=0;j<ntimesteps;j++){
                //calculate positions in the big continous models vector
                place=mes_nspecsteps*ntimesteps*k + mes_nspecsteps*j;
                place1=nspecsteps*ntimesteps*k + nspecsteps*j;
                //interpolation
                resampled_model_cont[place+i]=wd*this->model_cont[low+place1]+wu*this->model_cont[high+place1];
            }
        }
    }
    return 0;
}


///////////////////////////////////////////////////////////////


/*
    read sdss measurement with user input
 */
int sps_data::usr_read_sdss_csv()
{
    bool tryagain;
    std::cout<<"\nGetting sdss csv spectrum to fit"<<std::endl;
    
    do{
        tryagain=false;
        try{
            // get filename
            std::cout<<"input filename: (current directory is \"sps/input/\")\n";
            std::string temp_line;
            getline(std::cin,temp_line);
            std::stringstream temp_sstr;
            temp_sstr<<"../input/";
            temp_sstr<<temp_line;
            
            std::string infilename;
            temp_sstr>>infilename;
            this->read_sdss_measurement(infilename);
            
        }catch (int ex){
            if (ex==1){tryagain=true;}
        }
    }while(tryagain);
    
    std::cout<<"spectrum read\n";
    return 0;
}



int sps_data::read_sdss_measurement(std::string infilename ){
    std::cout<<"reading measurement ... "<<std::endl;
    
    std::ifstream infile(infilename.c_str());
    // check filename
    if(!(infile)){
        std::cout<<"\nERROR CAN'T OPEN FILE: "<<infilename<<"\n"<<std::endl;
        throw 1;
    }
    
    std::string temp_line;
    while(getline(infile,temp_line)){
        //this is lame
        for(size_t j=0;j<temp_line.size();j++){
            if (temp_line[j]==','){
                temp_line[j]=' ';
            }
        }
        
        std::stringstream temp_sstr;
        temp_sstr<<temp_line;
        if (temp_line[0] != '#' ){
            double temp=0;
            temp_sstr>>temp;
            this->mes_spec_wavel.push_back(temp);
            temp_sstr>>temp;
            this->mes_spec.push_back(temp);
            temp_sstr>>temp;
            this->mes_spec_err_l.push_back(temp);
            temp_sstr>>temp;
            this->mes_spec_err_h.push_back(temp);
            temp_sstr>>temp;
            this->mes_spec_mask.push_back(temp);
        }
    }
    infile.close();
    
    return 0;
}


///
//Reading all models from binary file 
//
int sps_data::read_model_bin_all_cont(){
	#define NO_METALL 6
	#define PATH "../input/bin/"

	char metall;
	std::string path;
	std::string imf;
	std::string filename;
	
	imf="chab";
	path=PATH;
	models.resize(2*NO_METALL);

    std::cout<<"reading models ";
	for(int i=0;i<2;i++){
		for(int j=0;j<NO_METALL;j++){
			metall='2'+j;
			//reading binary model
			std::stringstream filename_sstr;
			filename_sstr<<path<<imf<<metall<<".bin";
			filename_sstr>>filename;
			read_model_bin_cont(filename);
            std::cout<<"..."<< std::flush;
		}
		//second path:
		imf="salp";
	}
	std::cout<<std::endl;
	return 0;
}




///
//Reading one model from binary file
//
int sps_data::read_model_bin_cont( std::string infilename){
    std::ifstream infile(infilename.c_str(), std::ios::binary |std::ios::ate);
    //	CHECKING FILENAME
    if(!(infile)){
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
    
    return 0;
}


///
//Reading wavelengths from binary file
//
int sps_data::read_wavel_bin(){
    std::cout<<"reading wavelengths ... "<<std::endl;
    
#define PATH "../input/bin/"
    
    std::string path;
    std::string filename;
    
    path=PATH;
    std::stringstream filename_sstr;
    filename_sstr<<path<<"wavel.bin";
    filename_sstr>>filename;
    
    std::ifstream infile(filename.c_str(), std::ios::binary |std::ios::ate);
    //	CHECKING FILENAME
    if(!(infile)){
        std::cout<<"ERROR CAN'T OPEN FILE: "<<filename<<std::endl;
        return 1;
    }
    std::ifstream::pos_type size;
    size = infile.tellg();
    
    wavelengths.resize(size/sizeof(double));
    
    infile.seekg (0, std::ios::beg);
    infile.read (reinterpret_cast<char*>(&wavelengths[0]), size);
    infile.close();
    
    return 0;
}

///
//Reading timesteps from binary file
//
int sps_data::read_time_bin(){
    std::cout<<"reading timesteps ... "<<std::endl;
    
#define PATH "../input/bin/"
    
    std::string path;
    std::string filename;
    
    path=PATH;
    std::stringstream filename_sstr;
    filename_sstr<<path<<"time.bin";
    filename_sstr>>filename;
    
    std::ifstream infile(filename.c_str(), std::ios::binary |std::ios::ate);
    //	CHECKING FILENAME
    if(!(infile)){
        std::cout<<"ERROR CAN'T OPEN FILE: "<<filename<<std::endl;
        return 1;
    }
    std::ifstream::pos_type size;
    size = infile.tellg();
    
    time.resize(size/sizeof(double));
    
    infile.seekg (0, std::ios::beg);
    infile.read (reinterpret_cast<char*>(&time[0]), size);
    infile.close();
    
    return 0;
}






///
//Reading all models from binary file 
//
int sps_data::read_model_bin_all(){
	#define NO_METALL 6
	#define PATH "../input/bin/"

	char metall;
	std::string path;
	std::string imf;
	std::string filename;
	
	imf="chab";
	path=PATH;
	models.resize(2*NO_METALL);

	for(int i=0;i<2;i++){
		for(int j=0;j<NO_METALL;j++){
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
	return 0;
}

///
//Reading one model from binary file 
//
int sps_data::read_model_bin(int i, std::string infilename){
	std::ifstream infile(infilename.c_str(), std::ios::binary |std::ios::ate);
	//	CHECKING FILENAME
	if(!(infile)){
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
//reading spectrum to fit
//
int sps_data::usr_read_sample(){
	bool tryagain;	
	std::cout<<"\nGetting spectrum to fit"<<std::endl;
	do{
		tryagain=false;
		try{
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
			if(!(infile))										{
				std::cout<<"\nERROR CAN'T OPEN FILE: "<<infilename<<"\n"<<std::endl;
				throw 1;
			}
			
			// read spectrum
			std::cout<<"reading file: "<<infilename<<std::endl;
			std::string str;
			double temp;
			while(infile>>str){
				std::stringstream s;
				s<<str;
				if(s>>temp){
					sample_spec.push_back(temp);
		       	}
			}
			infile.close();
		
			//check no of ages
			if(sample_spec.size()==0){
				std::cout<<"\nERROR: no spectrum read\n"<<std::endl;
				throw 1;
			}
		}catch (int ex){
			if (ex==1){tryagain=true;}
		}
	}while(tryagain);

	std::cout<<"spectrum read\n";
	return 0;
}
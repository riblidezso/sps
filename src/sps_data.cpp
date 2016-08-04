#include "sps_data.h"

/*
 initialise data
    - read the model data
    - read the measurement
    - resample models to measuremement wavelengths
 */
sps_data::sps_data(std::string measurement_fname){
    std::string imf="chabrier";
    
    //read model
    this->read_binary_sps_model(imf);
    
    //read a measurement file
    this->read_measurement(measurement_fname);
    
    //resample models to measurement file
    this->resample_models_2_mes(imf);
    
}

//////////////////////////////////////////

/*
    read the binary sps model data
*/
int sps_data::read_binary_sps_model(std::string imf){
    //read time data
    this->read_time_bin();
    
    //read wavelength data
    this->read_wavel_bin();
    
    //read models
    this->read_model_bin_all_cont(imf);
    
    return 0;
}



/*
 read measurement from sdss csv file
 */
int sps_data::read_measurement(std::string infilename ){
    std::cout<<"reading measurement ... "<<std::endl;
    
    //open file
    std::ifstream infile(infilename.c_str());
    if(!infile.is_open()){
        std::cout<<"\nERROR CAN'T OPEN FILE: "<<infilename<<"\n"<<std::endl;
        exit(1);
    }
    
    //loop over lines
    std::string line;
    while(getline(infile,line)){
        //change seperator to space from comma
        for(size_t j=0;j<line.size();j++){
            if (line[j]==','){
                line[j]=' ';
            }
        }
        //parse numbers in the line
        std::stringstream temp_sstr;
        temp_sstr<<line;
        if (line[0] != '#' ){ //skip header
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


/*
    Resample all the models to the wavelengths of the measurement
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
                exit(1);
            }
        }
        
        //there might be problems if the first measured wavelength is
        //smaller than first model data, but this seems also unlikely
        if (high==0){
            std::cerr<<"\nERROR measurement wavelength"<<mes_spec_wavel[i]<<"\n";
            std::cerr<<"can't be interpolated from model wavelength points\n\n";
            exit(1);
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
Reading models with every metallicity from binary files into one contiguous vector
*/
int sps_data::read_model_bin_all_cont(std::string imf){
	#define NO_METALL_MODELS 6

    std::cout<<"reading models ";

    //loop over metallicities
    for(int metall=2;metall<NO_METALL_MODELS+2;metall++){
        //create filename
        std::string path=getenv("SPSFAST_PATH");
        std::stringstream filename_sstr;
        std::string filename;
        filename_sstr<<path<<"/input/bin/"<<imf<<metall<<".bin";
        filename_sstr>>filename;
        //read file
        read_model_bin_cont(filename);
        //report
        std::cout<<"..."<<std::flush;
	}
	std::cout<<std::endl;
	return 0;
}



/*
 Read one model from binary file
*/
int sps_data::read_model_bin_cont( std::string infilename){
    //open file
    std::ifstream infile(infilename.c_str(), std::ios::binary |std::ios::ate);
    if(!infile.is_open()){
        std::cout<<"ERROR CAN'T OPEN FILE: "<<infilename<<std::endl;
        exit(1);
    }
    //get size of file, and expand original container
    std::ifstream::pos_type size = infile.tellg();
    size_t begin = model_cont.size();
    model_cont.resize(begin + size/sizeof(double));
    
    //read whole file
    infile.seekg (0, std::ios::beg);
    infile.read(reinterpret_cast<char*>(&model_cont[begin]), size);
    infile.close();
    
    return 0;
}


/*
//Read wavelengths from binary file
*/
int sps_data::read_wavel_bin(){
    std::cout<<"reading wavelengths ... "<<std::endl;
    
    //create filename
    std::string path=getenv("SPSFAST_PATH");
    std::string filename;
    std::stringstream filename_sstr;
    filename_sstr<<path<<"/input/bin/wavel.bin";
    filename_sstr>>filename;
    
    //open
    std::ifstream infile(filename.c_str(), std::ios::binary |std::ios::ate);
    if(!infile.is_open()){
        std::cout<<"ERROR CAN'T OPEN FILE: "<<filename<<std::endl;
        exit(1);
    }
    
    //get size and resize container
    std::ifstream::pos_type size;
    size = infile.tellg();
    wavelengths.resize(size/sizeof(double));
    
    //read whole file
    infile.seekg (0, std::ios::beg);
    infile.read (reinterpret_cast<char*>(&wavelengths[0]), size);
    infile.close();
    
    return 0;
}

/*
 Read timesteps from binary file
*/
int sps_data::read_time_bin(){
    std::cout<<"reading timesteps ... "<<std::endl;
    
    //create filename
    std::string path=getenv("SPSFAST_PATH");
    std::string filename;
    std::stringstream filename_sstr;
    filename_sstr<<path<<"/input/bin/time.bin";
    filename_sstr>>filename;
    
    //open file
    std::ifstream infile(filename.c_str(), std::ios::binary |std::ios::ate);
    if(!infile.is_open()){
        std::cout<<"ERROR CAN'T OPEN FILE: "<<filename<<std::endl;
        exit(1);
    }
    
    //get size and resize container
    std::ifstream::pos_type size;
    size = infile.tellg();
    time.resize(size/sizeof(double));
    
    //read whole file
    infile.seekg (0, std::ios::beg);
    infile.read (reinterpret_cast<char*>(&time[0]), size);
    infile.close();
    
    return 0;
}
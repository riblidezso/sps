#include "sps_options.h"

/*
 initially set platfrom and device and measurement to  0
 set measurement file to a default one
 */
sps_options::sps_options(){
    this->device= 0;
    this->platform= 0;
    
    std::string path=getenv("SPSFAST_PATH");
    std::stringstream sstr;
    sstr<<path<<"/input/testspec1.csv";
    sstr>>this->sdss_measurement_fname;
}

/*
 reads config file
*/
int sps_options::read_config(std::string input_filename)
{
    //open file
    std::ifstream infile(input_filename.c_str(), std::ifstream::in );
    if(!infile.is_open()){
        std::cerr<<"\nERROR CAN'T OPEN CONFIG FILE: "<<input_filename<<"\n"<<std::endl;
        exit(1);
    }
    
    //loop over lines and get params
    std::string line,temp_str;
    std::map<std::string,std::string> params;
    while(getline(infile,line)){
        std::stringstream sstr;
        std::vector<std::string> temp_vec;
        //read whole line into vector
        sstr<<line;
        while(sstr>>temp_str){
            temp_vec.push_back(temp_str);
        }
        //if its a key pair save it
        if (temp_vec.size() == 2){
            params.insert(std::pair<std::string,std::string>(temp_vec[0],temp_vec[1]));
        }
    }
    
    //save the ones i need, fall back to deafult value if not set
    std::map<std::string,std::string>::iterator it;
    it= params.find("platform");
    if(it != params.end()){
        this->platform=(int) strtol( params["platform"].c_str(),NULL,10);
    }
    it= params.find("device");
    if(it != params.end()){
        this->device=(int) strtol( params["device"].c_str(),NULL,10);
    }
    it= params.find("measurement");
    if(it != params.end()){
        this->sdss_measurement_fname = params["measurement"];

    }

    infile.close();
    return 0;
}




/*
 read parameter file into a vector of maps
*/
std::vector<std::map<std::string,double> > sps_options::read_param_file(std::string infilename){
    //open file
    std::ifstream infile(infilename.c_str());
    if(! infile.is_open() ){
        std::cerr<<"\nERROR CAN'T OPEN PARAMETER FILE: "<<infilename<<"\n"<<std::endl;
        exit(1);
    }
    
    //read param names for the first line
    std::string line,param_name;
    std::stringstream temp_sstr;
    std::vector<std::string> param_names;
    getline(infile,line);
    temp_sstr<<line;
    while(temp_sstr>>param_name){
        param_names.push_back(param_name);
    }
    
    //read params from the other lines
    std::vector<std::map<std::string,double> > params;
    while ( getline (infile,line) ){
        std::map<std::string,double> temp_params;
        double value;
        
        temp_sstr.clear();
        temp_sstr<<line;
        
        int i=0;
        while(temp_sstr>>value){
            temp_params.insert(std::pair<std::string,double>(param_names[i],value));
            i++;
        }
        params.push_back(temp_params);
    }
    
    infile.close();
    return params;
    
}


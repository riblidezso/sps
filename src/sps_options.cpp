#include "sps_options.h"

/*
 initially set platfrom and device and measurement to  0
 set measurement file to a default one
 */
sps_options::sps_options(){
    //check enviroment variable
    if (getenv("SPSFAST_PATH") == NULL){
        std::cerr<<"ERROR!: SPSFAST_PATH enviroment variable not found."<<std::endl;
        exit(1);
    }
    
    this->device= 0;
    this->platform= 0;
    this->imf="chabrier";
    
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
    it= params.find("imf");
    if(it != params.end()){
        this->imf= params["imf"];
        
    }
    it= params.find("measurement");
    if(it != params.end()){
        this->sdss_measurement_fname = params["measurement"];

    }

    infile.close();
    return 0;
}




/*
 read parameter file
*/
int sps_options::read_param_file(std::string infilename){
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
    
    //check params
    check_params(param_names);
    
    //read params from the other lines
    while ( getline (infile,line) ){
        std::map<std::string,double> temp_params;
        std::string temp_str;
        
        temp_sstr.clear();
        temp_sstr<<line;
        
        int i=0;
        while(temp_sstr>>temp_str){
            //read numerical params
            if (param_names[i]!="sfr_filename"){
                temp_params.insert(std::pair<std::string,double>(param_names[i],strtod(temp_str.c_str(),NULL)));
            }
            //read sfr fileanem param
            else {
                this->sfr_filenames.push_back(temp_str);
            }
            i++;
        }
        //save params
        this->num_params.push_back(temp_params);
    }
    
    //read sfr files
    read_sfr_file_list();
    
    infile.close();
    return 0;
    
}


/*
 check if correct input parameters are used
 */
int sps_options::check_params(std::vector<std::string> param_names){
    std::set<std::string> param_name_set(param_names.begin(),param_names.end());
    std::set<std::string>::iterator it;
    
    //check if all necessary params are used
    std::vector<std::string> necessary_params={"age","dust_tau_v","dust_mu","metall","vdisp"};
    for (auto param : necessary_params){
        it = param_name_set.find(param);
        
        //not used: error
        if(it == param_name_set.end()){
            std::cerr<<"\nERROR "<<param<<"NOT USED IN PARAM FILE\n"<<std::endl;
            exit(1);
        }
    }
    
    //check star formation mode
    check_sfr_mode(param_names);
    
    return 0;
}


/*
 check sfr mode
 */
int sps_options::check_sfr_mode(std::vector<std::string> param_names){
    std::set<std::string> param_name_set(param_names.begin(),param_names.end());
    std::set<std::string>::iterator it1,it2;
    it1 = param_name_set.find("sfr_tau");
    it2 = param_name_set.find("sfr_filename");
    
    //both used: error
    if(it1 != param_name_set.end() && it2 != param_name_set.end()){
        std::cerr<<"\nERROR BOTH SFR_TAU AND SFR_FILENAME IN PARAM FILE"<<"\n"<<std::endl;
        std::cerr<<"\nPLEASE ONLY USE ONE OF THEM"<<"\n"<<std::endl;
        exit(1);
    }
    //exponential start formation
    else if(it1 != param_name_set.end() ){
        std::cout<<"Using exponential star formation rate\n";
        this->sfr_mode="exponential";
    }
    //start formation  from file
    else if(it2 != param_name_set.end() ){
        std::cout<<"Using star formation rate from file\n";
        this->sfr_mode="file";
    }
    //no start formation: error
    else{
        std::cerr<<"\nERROR NO STAR FORMATION RATE SPECIFIED"<<"\n"<<std::endl;
        exit(1);
    }
    
    return 0;
}


/*
 read sfr files
 */
int sps_options::read_sfr_file_list(){
    for (auto sfr_fname : sfr_filenames){
        this->sfr_list.push_back(read_sfr_file(sfr_fname));
    }
    return 0;
}

/*
 read one sfr from a file
 */
std::vector<double> sps_options::read_sfr_file(std::string infilename){
    //open file
    std::ifstream infile(infilename.c_str());
    if(! infile.is_open() ){
        std::cerr<<"\nERROR CAN'T OPEN SFR FILE: "<<infilename<<"\n"<<std::endl;
        exit(1);
    }
    
    //read sfr from each line
    std::string line;
    std::vector<double> temp_sfr;
    while ( getline (infile,line) ){
        temp_sfr.push_back(strtod(line.c_str(),NULL));
    }
    
    //check sfr length
    if (temp_sfr.size()!=NUM_TIME_STEPS){
        std::cerr<<"\nERROR SFR LENGTH AND MODEL TIMESTEP DONT MATCH\n";
        std::cerr<<"\tMODEL: "<<NUM_TIME_STEPS<<std::endl;
        std::cerr<<"\t"<<infilename<<": "<<temp_sfr.size()<<std::endl;
        exit(1);
    }

    return temp_sfr;
}


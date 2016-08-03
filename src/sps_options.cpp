#include "sps_options.h"

/*
 initially set platfrom and device and measurement to no choice
 */
sps_options::sps_options(){
    this->device= -1;
    this->platform= -1;
    this->sdss_measurement_fname="none";
}

/*
 reads config file
*/
int sps_options::read_config(std::string input_filename)
{
    //open file
    std::ifstream infile(input_filename.c_str(), std::ifstream::in );
    //check file
    if(!(infile))
    {
        std::cout<<"\nERROR CAN'T OPEN CONFIG FILE: "<<input_filename<<"\n"<<std::endl;
        return 1;
    }
    
    std::string str;
    while(getline(infile,str))
    {
        std::stringstream sstr;
        std::vector<std::string> tempvec;
        
        sstr<<str;
        while(sstr>>str)
        {
            tempvec.push_back(str);
        }
        
        if (tempvec.size() !=0 )
        {
            //reading parameters
            if( tempvec[0]=="platform"){
                this->platform= (int) strtol(tempvec[1].c_str(),NULL,10);
            }
            else if( tempvec[0]=="device"){
                this->device= (int) strtol(tempvec[1].c_str(),NULL,10);
            }
            else if( tempvec[0]=="measurement"){
                this->sdss_measurement_fname = tempvec[1];
            }
        }
        
    }
    infile.close();
    
    return 0;
}




//read parameter file
std::vector<std::map<std::string,double> > sps_options::read_param_file(std::string infilename){
    //open file
    std::ifstream infile(infilename.c_str());
    //check file
    if(! infile.is_open() )
    {
        std::cerr<<"\nERROR CAN'T OPEN PARAMETER FILE: "<<infilename<<"\n"<<std::endl;
        exit(1);
    }
    
    //read param names
    std::string line,param_name;
    std::stringstream temp_sstr;
    std::vector<std::string> param_names;
    getline(infile,line);
    temp_sstr<<line;
    while(temp_sstr>>param_name){
        param_names.push_back(param_name);
    }
    
    //readi params
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
    
    //for(auto v : params){
    //    std::cout<<v["a"]<<" "<<v["b"]<<" "<<v["c"]<<std::endl;
    //}
    
    infile.close();
    
    return params;
    
}


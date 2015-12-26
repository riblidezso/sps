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
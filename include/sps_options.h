#ifndef SPS_OPTIONS_H
#define SPS_OPTIONS_H

#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <string>
#include <map>

/*
    class for options, and reading config file
 */

class sps_options{
public:
    //initialize  choices
    sps_options();
    
    
    //read config file
    int read_config(std::string config_filename);
    //read param file
    std::vector<std::map<std::string,double> > read_param_file(std::string infilename);
    
    
    // opencl platform and device choices
    int platform, device;
    //imf choice (chabrier,salpeter)
    std::string imf;
    //filename of sdss csv spectrum
    std::string sdss_measurement_fname;
};

#endif

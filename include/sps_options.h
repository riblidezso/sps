#ifndef SPS_OPTIONS_H
#define SPS_OPTIONS_H

#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <string>

/*
    class for options, and reading config file
 */

class sps_options{
public:
    // opencl platform and device choices
    int platform;
    int device;
    
    // filename of sdss csv spectrum
    std::string sdss_measurement_fname;
    
    //initializes  choices
    sps_options();
    
    //read config file
    int read_config(std::string config_filename);
};

#endif

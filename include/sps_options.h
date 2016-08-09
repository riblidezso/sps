#ifndef SPS_OPTIONS_H
#define SPS_OPTIONS_H

#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <string>
#include <map>
#include <set>

#define NUM_TIME_STEPS 221

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
    int read_param_file(std::string infilename);
    int check_params(std::vector<std::string> param_names);
    int check_sfr_mode(std::vector<std::string> param_names);
    int read_sfr_file_list();
    std::vector<double> read_sfr_file(std::string infilename);
    
    
    // opencl platform and device choices
    int platform, device;
    //imf choice (chabrier,salpeter)
    std::string imf;
    //filename of sdss csv spectrum
    std::string sdss_measurement_fname;
    //imf choice (chabrier,salpeter)
    std::string sfr_mode;
    
    //numerical params
    std::vector<std::map<std::string,double> > num_params;
    //sfr filename params
    std::vector<std::string> sfr_filenames;
    //sfr vectors
    std::vector<std::vector<double> > sfr_list;

};

#endif

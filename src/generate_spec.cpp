#include <iostream>

#include "sps_options.h"
#include "sps_data.h"
#include "spectrum_generator.h"

int main(int argc, char* argv[])
{
    int error=0;
    
    ///////////////////////////////////////////////////////////////
    /*
     Check command line arguments
     */
    if (argc!=4){
        std::cerr<<"\nERROR please use 3 command line arguments"<<std::endl;
        std::cerr<<"\t1: congfig file (e.g.: test.cfg)"<<std::endl;
        std::cerr<<"\t2: param file (e.g.: test_params.tsv)"<<std::endl;
        std::cerr<<"\t3: output file (e.g.: results.tsv)"<<std::endl;
        exit(1);
    }
    
    ///////////////////////////////////////////////////////////////
    /*
     Read options from config file
     */
    
    sps_options my_sps_options;
    
    //read config file
    my_sps_options.read_config(argv[1]);
    
    
    ///////////////////////////////////////////////////////////////
    /*
     Read param file
     */
    std::vector<std::map<std::string,double> > param_list;
    param_list=my_sps_options.read_param_file(argv[2]);
    

    ///////////////////////////////////////////////////////////////
    /*
     Read data
     - read the models and int the input measurement data
     - resample models to measurement wavelengths
     */
    
    sps_data my_sps_data;
    
    //read sps model
    my_sps_data.read_binary_sps_model();
    //read measurement data
    my_sps_data.read_measurement(my_sps_options.sdss_measurement_fname);
    
    //resample models to measurement wavelengths
    my_sps_data.resample_models_2_mes("chabrier");
    
    
    ///////////////////////////////////////////////////////////////
    /*
     Specturm generator class
     */
    
    spectrum_generator my_spec_gen(my_sps_data);
    
    //basic stuff
    my_spec_gen.opencl_initialize("fit_w_err.cl",my_sps_options.device,my_sps_options.platform);
    //kernel and memory
    my_spec_gen.opencl_kern_mem();
    //setting the device memories of kernels
    my_spec_gen.set_kern_arg();
    
    
    ///////////////////////////////////////////////////////////////
    /*
    Generate spectra
     */
    
    std::vector< std::vector<cl_float> > results;
    int i=0;
    for (auto params : param_list){
        //set params
        my_spec_gen.set_params(params);
        
        //generate spectrum in device
        my_spec_gen.generate_spectrum();
        
        //get the result
        results.push_back(my_spec_gen.get_result());
        
        //report
        i++;
        if( (i % 1000) == 0 )
            std::cout<<i<<" spectra generated "<<std::endl;
        if( (i % 50) == 0 ){
            std::cout<<".";
            std::cout.flush();
        }
    }
    
    ///////////////////////////////////////////////////////////////
    /*
     Write results
     */
    my_spec_gen.write_specs(results, argv[3]);
    
    
    ///////////////////////////////////////////////////////////////
    /*
     Free resources
     */
    my_spec_gen.clean_resources();
    
    return	error;
}

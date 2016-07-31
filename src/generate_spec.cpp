#include <iostream>

#include "sps_options.h"
#include "mcmc.h"
#include "sps_data.h"
#include "spectrum_generator.h"


int main(int argc, char* argv[])
{
    int error=0;
    
    ///////////////////////////////////////////////////////////////
    /*
     Check command line arguments
     */
    if (argc!=2){
        std::cerr<<"\nERROR please use 1 command line argument"<<std::endl;
        std::cerr<<"\t congfig file (e.g.: test.cfg)"<<std::endl;
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
     MCMC fitter class
     */
    
    mcmc mcmc_fitter;
    
    //read mcmc fitting parameters from config file
    mcmc_fitter.read_config(argv[1]);
    
    
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
    my_spec_gen.opencl_initialize("fit_w_err.cl",my_sps_options.platform,my_sps_options.device);
    //kernel and memory
    my_spec_gen.opencl_kern_mem();
    //setting the device memories of kernels
    my_spec_gen.set_kern_arg();
    
    
    ///////////////////////////////////////////////////////////////
    /*
    Generate spectrum
     */

    //set params
    my_spec_gen.set_params(mcmc_fitter.parameters);
        
    //generate spectrum
    my_spec_gen.generate_spectrum();
    
    //read back model to host
    my_spec_gen.read_best_result();
    
    
    ///////////////////////////////////////////////////////////////
    /*
     Write results
     */
    my_spec_gen.write_fit_result();
    
    
    ///////////////////////////////////////////////////////////////
    /*
     Free resources
     */
    my_spec_gen.clean_resources();
    
    return	error;
}

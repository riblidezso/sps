#include <iostream>

#include "sps_options.h"
#include "sps_data.h"
#include "spectrum_generator_cpu.h"

int main(int argc, char* argv[])
{
    //beggining and end of execution times
    std::chrono::high_resolution_clock::time_point begin, end;
    begin = std::chrono::high_resolution_clock::now();
    
    int error=0;
    
    ///////////////////////////////////////////////////////////////
    /*
     Check command line arguments
     */
    if (argc!=4){
        std::cerr<<"\nERROR please use 3 command line arguments"<<std::endl;
        std::cerr<<"\t1: config file (e.g.: test.cfg)"<<std::endl;
        std::cerr<<"\t2: param file (e.g.: test_params.tsv)"<<std::endl;
        std::cerr<<"\t3: output file (e.g.: results.tsv)"<<std::endl;
        exit(1);
    }
    
    ///////////////////////////////////////////////////////////////
    /*
     Read options from config file
     */
    sps_options my_sps_options;
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
     - reads the models and the input measurement data
     - resamples models to measurement wavelengths
     */
    sps_data my_sps_data(my_sps_options.sdss_measurement_fname,my_sps_options.imf);
    
    
    ///////////////////////////////////////////////////////////////
    /*
     Spectrum generator class
     */
    spectrum_generator_cpu my_spec_gen(my_sps_data);
    
    ///////////////////////////////////////////////////////////////
    /*
    Generate spectra
     */
    
    std::vector< std::vector<double> > results;
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
    
    
    //stop clock
    end = std::chrono::high_resolution_clock::now();
    std::chrono::microseconds ms = std::chrono::duration_cast<std::chrono::microseconds >(end - begin);
    std::cout<<"\nIt took: "<<ms.count()/double(1e6)<<" s"<< std::endl;
    
    return	error;
}

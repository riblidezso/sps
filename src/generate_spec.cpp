#include <iostream>

#include "sps_options.h"
#include "sps_data.h"
#include "spectrum_generator.h"

int main(int argc, char* argv[]){
    int error=0;
    
    //Record execution time
    std::chrono::high_resolution_clock::time_point begin, end;
    begin = std::chrono::high_resolution_clock::now();
    
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
    my_sps_options.read_param_file(argv[2]);
    

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
    spectrum_generator my_spec_gen(my_sps_data,"spsfast_kernels.cl",my_sps_options.platform,my_sps_options.device);
    
    ///////////////////////////////////////////////////////////////
    /*
    Generate spectra
     */
    std::cout<<"\nGenerating spectra ...:\n";
    std::vector< std::vector<cl_float> > results;
    for (size_t i=0;i<my_sps_options.num_params.size();i++){
        //generate spec
        if (my_sps_options.sfr_mode=="file")
            my_spec_gen.generate_spectrum(my_sps_options.num_params[i],my_sps_options.sfr_list[i]);
        else
            my_spec_gen.generate_spectrum(my_sps_options.num_params[i]);

        //get the result
        results.push_back(my_spec_gen.get_result());
        
        //report
        if( (i % 1000) == 0 && i!=0 )
            std::cout<<i<<" spectra generated "<<std::endl;
        if( (i % 50) == 1 ){
            std::cout<<".";
            std::cout.flush();
        }
    }
    
    ///////////////////////////////////////////////////////////////
    /*
     Write results
     */
    std::cout<<"\nWriting results ...\n";
    my_spec_gen.write_specs(results, argv[3]);
    
    
    ///////////////////////////////////////////////////////////////
    /*
     Free resources
     */
    my_spec_gen.clean_resources();
    
    
    ///////////////////////////////////////////////////////////////
    /*
     Report exectution time
     */
    end = std::chrono::high_resolution_clock::now();
    std::chrono::microseconds ms = std::chrono::duration_cast<std::chrono::microseconds >(end - begin);
    std::cout<<"\nDone \nIt took: "<<ms.count()/double(1e6)<<" s"<< std::endl;
    
    return	error;
}

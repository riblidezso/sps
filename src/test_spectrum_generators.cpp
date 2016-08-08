#include <iostream>

#include "sps_options.h"
#include "sps_data.h"
#include "spectrum_generator.h"
#include "spectrum_generator_cpu.h"

int main(int argc, char* argv[]){
    int error=0;
    
    //Record execution time
    std::chrono::high_resolution_clock::time_point begin, end;
    begin = std::chrono::high_resolution_clock::now();
    
    ///////////////////////////////////////////////////////////////
    /*
     Check command line arguments
     */
    if (argc!=3){
        std::cerr<<"\nERROR please use 3 command line arguments"<<std::endl;
        std::cerr<<"\t1: config file (e.g.: test.cfg)"<<std::endl;
        std::cerr<<"\t2: param file (e.g.: test_params.tsv)"<<std::endl;
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
    spectrum_generator my_spec_gen(my_sps_data,"fit_w_err.cl",my_sps_options.platform,my_sps_options.device);

    
    ///////////////////////////////////////////////////////////////
    /*
     Spectrum generator class cpu
     */
    spectrum_generator_cpu my_spec_gen_cpu(my_sps_data);

    ///////////////////////////////////////////////////////////////
    /*
    Generate spectra
     */
    
    //results
    std::vector< double> result_opencl,result_cpu;
    std::vector<cl_float> temp_res_opencl;
    
    int i=0;
    for (auto params : param_list){
        //set params
        my_spec_gen_cpu.set_params(params);
        //generate spectrum in device
        my_spec_gen_cpu.generate_spectrum();
        //get the result
        result_cpu=my_spec_gen_cpu.get_result();
        
        //set params
        my_spec_gen.set_params(params);
        //generate spectrum in device
        my_spec_gen.generate_spectrum();
        //get the result
        temp_res_opencl=my_spec_gen.get_result();
        result_opencl = std::vector<double>(temp_res_opencl.begin(),temp_res_opencl.end());
        
        //compare them
        double diff=0;
        for(size_t j=0;j<result_cpu.size();j++){
            diff+=(result_cpu[j] - result_opencl[j] ) * (result_cpu[j] - result_opencl[j] ) /(result_cpu[j] * result_opencl[j] );
        }
        diff=diff/result_cpu.size();
//std::cout<<"relative average squred error is: " <<diff<<" "<<std::endl;
        
        //fail if difference is too big
        if (diff>1e-10){
            std::cerr<<"\nERROR! cpu only and opencl implementations give different results"<<std::endl;
            std::cout<<"relative average squred error is: " <<diff<<" "<<std::endl;
            exit(1);
        }
        
        //report
        i++;
        if( (i % 1000) == 0 )
            std::cout<<i<<" spectra generated "<<std::endl;
        if( (i % 50) == 1 ){
            std::cout<<".";
            std::cout.flush();
        }
    }
    
    //print if passed
    std::cout<<"Passed"<<std::endl;
    
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

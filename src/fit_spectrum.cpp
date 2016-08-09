#include <iostream>
#include <chrono>

#include "sps_options.h"
#include "mcmc.h"
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
    if (argc!=3){
        std::cerr<<"\nERROR please use 2 command line argument"<<std::endl;
        std::cerr<<"\t congfig file (e.g.: test.cfg)"<<std::endl;
        std::cerr<<"\t output prefix (e.g.: test_out_prefix)"<<std::endl;
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
        MCMC fitter class
        -read mcmc fitting parameters from config file
    */
    
	mcmc mcmc_fitter(argv[1],argv[2]);
	

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
    spectrum_generator my_spec_gen(my_sps_data,"spsfast_kernels.cl",my_sps_options.platform,my_sps_options.device,"exponential");


    ///////////////////////////////////////////////////////////////
    /*
        Do the monte carlo markov chain
     */
	std::cout<<"\nFitting with MCMC ...";
	for(mcmc_fitter.iter= 0; mcmc_fitter.iter<(mcmc_fitter.maxiter);mcmc_fitter.iter++){
        //do one step in parameter space
		mcmc_fitter.change_params();
        my_spec_gen.set_params(mcmc_fitter.parameters);

        //generate spectrum
        my_spec_gen.generate_spectrum();
        //compare it to measurement
        double chi = my_spec_gen.compare_to_measurement();
		
        //evaluate the step
		mcmc_fitter.evaluate_step( chi );

        //report
        if( (mcmc_fitter.iter % 1000) == 0 )
            std::cout<<"\n"<<mcmc_fitter.iter<<" iterations done ";
        if( (mcmc_fitter.iter % 50) == 0 )
            std::cout<<"."<<std::flush;
		if( (mcmc_fitter.iter % 10000) == 0 && mcmc_fitter.iter>0)
			std::cout<<"\n";	
	}
    
    ///////////////////////////////////////////////////////////////
    /*
        Repeat the best fit
     */
    //set best params
    my_spec_gen.set_params(mcmc_fitter.best_parameters);
    //generate spectrum
    my_spec_gen.generate_spectrum();
    //compare it to measurement (necessary for scaling)
    my_spec_gen.compare_to_measurement();
    //read back model to host
    std::vector<std::vector<cl_float> > results;
    results.push_back(my_spec_gen.get_result());
    
    
    ///////////////////////////////////////////////////////////////
    /*
        Write results
     */
    std::cout<<"\nWriting results ...\n";
	mcmc_fitter.write_results();
    my_spec_gen.write_specs(results, std::string(argv[2]) + "_best_fit.tsv");
    
    
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

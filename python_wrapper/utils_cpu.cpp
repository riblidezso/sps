#include "sps_data.h"
#include "spectrum_generator_cpu.h"

#define NUM_TIME_STEPS 221
    
extern "C" {
    spectrum_generator_cpu* spectrum_generator_cpu_new(char* measurement_file, char* imf){
        //load data
        sps_data my_sps_data = sps_data(measurement_file,imf);
        //spec gen
        spectrum_generator_cpu* my_spec_gen = new spectrum_generator_cpu(my_sps_data);
        
        return my_spec_gen;
    }
    
    double* generate_spec_cpu_exp(spectrum_generator_cpu* my_spec_gen,
                         double age, double vdisp, double metall, double sfr_tau,
                         double dust_tau_v, double dust_mu, int* result_size){
        //params
        std::map<std::string,double> params;
        params.insert(std::pair<std::string,double> ("age",age));
        params.insert(std::pair<std::string,double> ("sfr_tau",sfr_tau));
        params.insert(std::pair<std::string,double> ("metall",metall));
        params.insert(std::pair<std::string,double> ("vdisp",vdisp));
        params.insert(std::pair<std::string,double> ("dust_tau_v",dust_tau_v));
        params.insert(std::pair<std::string,double> ("dust_mu",dust_mu));
        
        //generate spectrum
        my_spec_gen->generate_spectrum(params);
        
        //get the result
        std::vector<double> result = my_spec_gen->get_result();
        *result_size =  (int) result.size();
        
        //copy it to new array
        //unfortunately you cannot "steal" data from vector
        double* res_arr=new double[result.size()];
        std::copy(result.begin(), result.end(), res_arr);
            
        return res_arr;
    }
    
    double* generate_spec_cpu_sfr_vec(spectrum_generator_cpu* my_spec_gen,
                         double age, double vdisp, double metall, double sfr_tau,
                         double dust_tau_v, double dust_mu, double* sfr, int* result_size){
        //params
        std::map<std::string,double> params;
        params.insert(std::pair<std::string,double> ("age",age));
        params.insert(std::pair<std::string,double> ("sfr_tau",sfr_tau));
        params.insert(std::pair<std::string,double> ("metall",metall));
        params.insert(std::pair<std::string,double> ("vdisp",vdisp));
        params.insert(std::pair<std::string,double> ("dust_tau_v",dust_tau_v));
        params.insert(std::pair<std::string,double> ("dust_mu",dust_mu));
        std::vector<double> sfr_vec(sfr,sfr+NUM_TIME_STEPS);
        
        //generate spectrum
        my_spec_gen->generate_spectrum(params,sfr_vec);
        
        //get the result
        std::vector<double> result = my_spec_gen->get_result();
        *result_size =  (int) result.size();
        
        //copy it to new array
        //unfortunately you cannot "steal" data from vector
        double* res_arr=new double[result.size()];
        std::copy(result.begin(), result.end(), res_arr);
            
        return res_arr;
    }
    
    void freeme(double *ptr){
        printf("freeing address: %p\n", ptr);
        delete[] ptr;
    }
    
    void free_spec_gen_cpu(spectrum_generator_cpu* ptr){
        printf("freeing address: %p\n", ptr);
        delete ptr;
    }

}
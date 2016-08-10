#include "sps_data.h"
#include "spectrum_generator.h"
   
#define NUM_TIME_STEPS 221
 
extern "C" {
    spectrum_generator* spectrum_generator_new(char* measurement_file, char* imf, int platform, int device){
        //load data
        sps_data my_sps_data = sps_data(measurement_file,imf);
        //spec gen
        spectrum_generator* my_spec_gen = new spectrum_generator(my_sps_data,"spsfast_kernels.cl",platform,device);
        
        return my_spec_gen;
    }
    
    double* generate_spec_exp(spectrum_generator* my_spec_gen,
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
        std::vector<cl_float> result = my_spec_gen->get_result();
        *result_size =  (int) result.size();
        
        //copy it to new array, conversion too
        double* res_arr=new double[result.size()];
        for( int i=0;i<(int) result.size();i++){
            res_arr[i]=(double) result[i];
        }
            
        return res_arr;
    }
    
    double* generate_spec_sfr_vec(spectrum_generator* my_spec_gen,
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
        std::vector<cl_float> result = my_spec_gen->get_result();
        *result_size =  (int) result.size();
        
        //copy it to new array
        double* res_arr=new double[result.size()];
        for( int i=0;i<(int) result.size();i++){
            res_arr[i]=(double) result[i];
        }
            
        return res_arr;
    }
    
    void freeme(double *ptr){
        printf("freeing address: %p\n", ptr);
        delete[] ptr;
    }
    
    void free_spec_gen(spectrum_generator* ptr){
        printf("freeing address: %p\n", ptr);
        delete ptr;
    }

}
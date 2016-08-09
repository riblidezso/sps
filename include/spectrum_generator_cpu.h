#ifndef SPECTRUM_GENERATOR_CPU_H
#define SPECTRUM_GENERATOR_CPU_H

#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <map>
#include <string.h>
#include <math.h>
#include "sps_data.h"
#include "sps_write.h"


class spectrum_generator_cpu{
public:

//constructor
    spectrum_generator_cpu(sps_data& input_data,std::string sfr_mode);

/*
 FUNCTIONS
 */
    
public:
    
//intial functions
    
    //copy and move data from the sps_data object
    int copy_and_move_data(sps_data& model);


//functions called during operation
    
public:
    int set_params( std::map<std::string,double>& parameters, std::vector<double>& sfr  );
    int set_params( std::map<std::string,double>& parameters );

public:
	int generate_spectrum();
private:
    int metall_interpol(int wave);
    int conv_model_w_sfh(int wave);
    int conv_model_w_sfh_exp(int wave);
    int conv_model_w_sfh_vector(int wave);
    int convol_vel_disp(int wave);
    
public:
    double compare_to_measurement();
private:
    double get_factor_to_scale_spectra_to_measurement();
    double get_chi_square();
    
public:
    std::vector<double> get_result();

//functions called at the end
    
public:
    int write_specs(std::vector< std::vector<double> >& results,std::string out_fname);
	
	//writes the fitted, and the measured model
	//int write_fit_result();
    
    
/*
 DATA
 */
    
private:
    
//sps model data
    
    //ages where model data is available
    std::vector<double> time;
    //wavelengths of the model
    std::vector<double> wavelengths;
    //stores the resampled model
    std::vector<double> resampled_model;
    //stores the metallicity resampled model
    std::vector<double> model;
    //sizes of model data points
    int ntimesteps;
    
    
//measured spectrum data
    
	//the intensity
	std::vector<double> mes_spec;
	//error is pessimistic, uses the larger 
	//of low and high errors
	std::vector<double> mes_spec_err;
	//mask from measurement
	//bad data points, and emission lines are not fitted
	std::vector<double> mes_spec_mask;
	//the wavelengths of the measurement (in Angstroms)
	std::vector<double> mes_spec_wavel;
	//number of measured points
	int mes_nspecsteps;


//model parameters
    
    //sfr from file of exponential
    std::string sfr_mode;
    
	//-age is the beggining time of the sytnhesis
	//-sfr_tau: now SFR is an exponential decay
	//sfr_tau is the time constant of the decay
	//-metall : is the metallicity interpolated from the
	//available metallicity models
	//-dust_mu, dust_tau_v are the parameters of the
	// Charlot & Fall dust model
	//-vdisp is the velocity dispersion in velocity of light units
	double dust_tau_v,dust_mu,sfr_tau, age, metall, vdisp;
    
    //start formation in vector
    std::vector<double> sfr;


//result
    std::vector<double> result_no_vel,result;
    //aggregated value of weighted squared errors
	double chi;

};

#endif

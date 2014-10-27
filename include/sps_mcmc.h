#ifndef SPS_MCMC_H
#define SPS_MCMC_H

#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <random>
#include <map>
#include <float.h>

#include "sps_write.h" 

class sps_mcmc{
public:

	sps_mcmc();

public:
//functions

	int set_initial_params();

	int fix_params();

	int change_params(double opt_acc);

	int evaluate_chi(double input_chi);

	int record_data();

	int write_results();

	//test config file read	
	int read_config(std::string input_filename);

//data
	int iter;

//alpha version
	//fitted params
	double dust_tau_v,dust_mu,sfr_tau, age, metall, vdisp;
	//proposed steps
	double d_dust_tau_v, d_dust_mu, d_sfr_tau, d_age, d_metall, d_vdisp;
	//best params
	double best_dust_tau_v, best_dust_mu, best_sfr_tau, best_age, best_metall, best_vdisp;

	//chi values
	double chi,chi_before, best_chi;

// diagnostics, results 
	//all the chi values
	std::vector<double> out_chi_evol;
	//best chi values
	std::vector<double> out_best_chi_evol;
	//accepted chi values
	std::vector<double> out_acc_chi_evol;
	//worse than before, but accepted chi values
	std::vector<double> out_worse_acc_chi_evol;
	


	std::vector<double> acc;		//stores the last some accept or nots (0,1)
	std::vector<double> acc_ratio;	// stores averaged acceptance rates
	
	std::vector<double> worse;		//stores the last some worse ot nots (0,1)
	std::vector<double> worse_rate;	// stores averaged rates

	std::vector<double> worse_acc;		//stores the last some worse accept or nots (0,1)
	std::vector<double> worse_acc_ratio;	// stores averaged worse acceptance rates

	//random generator
	std::default_random_engine generator;


	//indicating acceptance
	int accepted;

	//all the points in the markov chain
	std::vector< std::vector <double> > points;

	//parameter fixing variables
	double fix_dust_tau_v;
	double fix_dust_mu;
	double fix_sfr_tau;
	double fix_age;
	double fix_metall;
	double fix_vdisp;

	//burnin indicator
	bool burnin_ended;

	//step size
	double sigma;

	//map for config info
	std::map<std::string,std::string> config_map;
};

#endif
	

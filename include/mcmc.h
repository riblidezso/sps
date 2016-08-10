#ifndef mcmc_H
#define mcmc_H

#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <random>
#include <map>
#include <float.h>

#include "sps_write.h" 

#define BEST_STEP 0
#define BETTER_STEP 1
#define WORSE_ACCEPTED_STEP 2
#define REJECTED_STEP 3


#define CONTROL_WINDOW 500

class mcmc{
public:

    mcmc(std::string config_filename,std::string output_prefix);

public:
//functions

	//config file read	
	int read_config(std::string input_filename);
    int print_config_params();

	int control_step_size(double opt_acc);
    int calculate_acc_ratio();

	int change_params();
    int get_next_param_to_change();
    int generate_step();
    int check_boundaries(double step);

	int evaluate_step(double input_logp);

	int record_data();

	int write_results();


//data
    int iter,maxiter;
    std::string output_prefix;

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

	//burnin indicator
	bool burnin_ended;

	//maps for parameters
	std::map<std::string,double> parameters;
	std::map<std::string,double> best_parameters;

	//maps for controlling step sizes	
	std::map<std::string,double> sigmas;
	std::map<std::string,std::vector<double> > sigmas_evol;
	std::map<std::string,std::vector<double> > acc_s;
	std::map<std::string,std::vector<double> > acc_ratio_s;
	//an iterator which tells the changer subroutine
	//wich parameter to change
	std::map<std::string,double>::iterator param_iter;
	//an iterator to help it
	int change_iter;

	std::map<std::string,bool> fix_parameters;

	std::map<std::string,double> param_upper_bound;
	std::map<std::string,double> param_lower_bound;

	//map for parameter evolution
	std::map<std::string,std::vector<double> > parameter_evol;
};

#endif
	

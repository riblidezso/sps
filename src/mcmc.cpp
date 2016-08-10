#include "mcmc.h"

//initalize
mcmc::mcmc(std::string config_filename,std::string output_prefix){
    //output prefix
    this->output_prefix=output_prefix;
    
    //chi values
	chi_before=DBL_MAX;
	best_chi=DBL_MAX;

	//burnin indicator
	burnin_ended=false;

    //number of iterations
	change_iter=0;
    
    //read config file
    read_config(config_filename);
    
    //initialize parameter iterator
    param_iter=parameters.begin();
    
    //initial report on stdout
    print_config_params();
}


/*
    read config file
 */
int mcmc::read_config(std::string input_filename){
    //open file
    std::cout<<"Reading config file ... "<<std::endl;
	std::ifstream infile(input_filename.c_str(), std::ifstream::in );
	if(!infile.is_open()) {
		std::cout<<"\nERROR CAN'T OPEN CONFIG FILE: "<<input_filename<<"\n"<<std::endl; 
		exit(1);
	}
    
    //loop over lines
	std::string line,temp_str;
	while(getline(infile,line)){
		std::stringstream sstr;
		std::vector<std::string> tempvec;
		std::vector<double> emptyvec;

        //get everything from the line into a vector
		sstr<<line;
		while(sstr>>temp_str)
			tempvec.push_back(temp_str);

        //read initial parameter guesses
		if (tempvec.size() == 3 ){
            if( tempvec[0]=="parameter"){
				parameters.insert(std::pair<std::string,double> (tempvec[1],atof(tempvec[2].c_str())));
				best_parameters.insert(std::pair<std::string,double> (tempvec[1],atof(tempvec[2].c_str())));
				parameter_evol.insert(std::pair<std::string,std::vector<double> > (tempvec[1],emptyvec));

				//stuff for controlling step size, initial step size hardcoded
				sigmas.insert(std::pair<std::string,double> (tempvec[1],0.2));
				sigmas_evol.insert(std::pair<std::string,std::vector<double> > (tempvec[1],emptyvec));
				acc_s.insert(std::pair<std::string,std::vector<double> > (tempvec[1],emptyvec));
				acc_ratio_s.insert(std::pair<std::string,std::vector<double> > (tempvec[1],emptyvec));
			}
        }
        
        //read the parameter boundaries
        if (tempvec.size()==4){
            if( tempvec[0]=="bounds"){
				param_lower_bound.insert(std::pair<std::string,double> (tempvec[1],atof(tempvec[2].c_str())));
				param_upper_bound.insert(std::pair<std::string,double> (tempvec[1],atof(tempvec[3].c_str())));
			}
        }
        
        //read fixed parameters
        if (tempvec.size()==3){
            if( tempvec[0]=="fix"){
				if (tempvec[2]=="yes")
					fix_parameters.insert(std::pair<std::string,double> (tempvec[1],true));
                else {
					fix_parameters.insert(std::pair<std::string,double> (tempvec[1],false));
                }
            }
        }
        
        //read the maximum number of iterations
        if (tempvec.size()==2){
            if (tempvec[0]=="maxiter") {
                this->maxiter = (int) strtol(tempvec[1].c_str(),NULL,10);
            }
        }
	}
	infile.close();
    return 0;
}

/*
 initial report to stdout
 */
int mcmc::print_config_params(){
    //initial param guesses
	std::cout<<"\nInitial parameter guesses:"<<std::endl;
    for (auto& param: parameters ){
        std::string output = std::string(param.first);
        output.resize(20,' ');
        std::cout<<output<<param.second<<std::endl;
    }
    
    //print which params are fixed
	std::cout<<"\nFixed parameters:"<<std::endl;
    for (auto& param: fix_parameters ){
        if (param.second==1){
            std::cout<<param.first<<std::endl;
        }
    }
    
    //boundaries for parameters
	std::cout<<"\nParameter bounds :"<<std::endl;
    for (auto& param: param_lower_bound ){
        std::string output = std::string(param.first);
        output.resize(20,' ');
		std::cout<<output<<param.second<<"\t"<<param_upper_bound[param.first]<<std::endl;
    }

	return 0;
}


/*
 control the step size to achieve the desired acceptance ratio
    -control mechanism is now pure proportional
        - integral just makes stability worse
        - differential is said to be hard to do right
 */
int mcmc::control_step_size(double opt_acc){
    //caluclate acceptance ratio
    calculate_acc_ratio();
	
	for (auto& param : acc_ratio_s){
		if( param.second.size()>1 && iter%CONTROL_WINDOW ==1 && iter>3 ){
			double weigth_prop=0.6;
			//weigth int is 0, means no integral control now
			double weigth_int=0;
			double weigth_all=1;

			//proportional error value
			double prop_err=param.second[param.second.size()-1]-opt_acc;

			//calculate integral error value
			double int_err=0;
			for(size_t i=100;i<param.second.size();i++)
				int_err+=(param.second[i]-opt_acc);

			//modify control variable
			//now there is the scale of the control var
			sigmas[param.first]+=weigth_all*sigmas[param.first]*(weigth_prop*prop_err+weigth_int*int_err);

			//this should not happen	
			if(sigmas[param.first]>1){
				std::cout<<"WARNING sigma > 1 ";
			}

			//this should not happen	
			if(sigmas[param.first]<0){
				std::cout<<"ERROR sigma < 0 ";
				exit(1);
			}
		}
		//record the sigma for diagnostics
		sigmas_evol[param.first].push_back(sigmas[param.first]);
	}

	return 0;
}


/*
 calculate parameter wise acceptance ratio
 */
int mcmc::calculate_acc_ratio(){
    //the equality is not neccesary
    size_t int_window=CONTROL_WINDOW ;
    
    if(iter%CONTROL_WINDOW ==1 && iter > (int) int_window){
        for (auto& param : acc_s ){
            double mean=0;
            if (param.second.size()>int_window){
                for(size_t i=0;i<int_window;i++)
                    mean+=param.second[param.second.size()-int_window+i];
                acc_ratio_s[param.first].push_back(mean/int_window);
            }
        }
    }
    
    return 0;
    
}





/*
 change parameters
*/
int mcmc::change_params(){
    //first control the step size
    this->control_step_size(0.5);
	
	//get next param to change
    get_next_param_to_change();
    
    //generate a valid step
    generate_step();
    
    return 0;
}

/*
 update parameter iterator to give the next parameter to change
 */
int mcmc::get_next_param_to_change(){
    int noparams=(int) parameters.size();
    
    //iterates parameters in a loop until
    //it finds a paramtere that is not fixed
    do{
        if ( change_iter % noparams == 0 )
            param_iter=parameters.begin();
        else
            ++param_iter;
        change_iter++;
        
    }while (fix_parameters[param_iter->first]==true);
    
    return 0;
}

/*
 generate a valid step in parameter space
 */
int mcmc::generate_step(){
    //intialize normal distribution random generator
    std::normal_distribution<double> distribution(0,sigmas[param_iter->first]);
    
    //loop create steps until acceptable step is created
    int status=0;
    do{
        //create step
        double step = param_iter->second * distribution(generator);
        
        //check boundaries
        status=check_boundaries(step);
        
    }while(status==1);
    
    return 0;
    
}

/*
 check if a proposed step falls within the parameter boundaries
 */
int mcmc::check_boundaries(double step){
    //check boundaries
    if (	param_iter->second + step > param_lower_bound[param_iter->first] ||
            param_iter->second + step < param_upper_bound[param_iter->first] ){
        param_iter->second += step;
        return 0;
    }
    //reject step if it is outside the boundaries
    else{
        return 1;
    }
}


/*
 evaluate the logp (chi) withg Metropolis Hasting algorithm
 */
int mcmc::evaluate_step(double chi){
    this->chi=chi;
    
    //best step until now
	if (chi < best_chi)
		accepted=BEST_STEP;
    
    //better than the one before
	else if (chi < chi_before )
		accepted=BETTER_STEP;
    
	//worse than the one before
	else {
		double limit=exp(- (chi - chi_before) ) * RAND_MAX;
		double rand_num=rand();

		if(rand_num < limit )
            accepted=WORSE_ACCEPTED_STEP;
		else
			accepted=REJECTED_STEP;
	}
    
    //record the point parameter space
    this->record_data();
    
	return 0;
}


/*
 records all kind of data in every step
 */
int mcmc::record_data(){
    //record the chi values
	out_chi_evol.push_back(chi);
	out_best_chi_evol.push_back(best_chi);

	//check if burnin has ended
	if (burnin_ended==false ){
		int window=5000;
		if ( iter > window ){
			if (chi > out_chi_evol[iter-window]){
				burnin_ended=true;
				std::cout<<"\nBurnin finished at iteration: "<<iter<<"\n"<<std::endl;
			}	
		}
	}

    //step was accepted
	if(accepted==BEST_STEP || accepted==BETTER_STEP || accepted==WORSE_ACCEPTED_STEP ){
		chi_before=chi;
		out_acc_chi_evol.push_back(chi);
		acc_s[param_iter->first].push_back(1);

		//record parameters to evolution vectors
        for (auto& param : parameter_evol)
            param.second.push_back(parameters[param.first]);
	}
    //not accepted
	else {
		//step back to last accepted step
        param_iter->second = parameter_evol[param_iter->first][parameter_evol[param_iter->first].size()-1];
		
		out_acc_chi_evol.push_back(0);
		acc_s[param_iter->first].push_back(0);
	}
    
    //if current step is the best step record it
    if (accepted==BEST_STEP){
		for (auto& param : parameters)
			best_parameters[param.first] = param.second;
		best_chi=chi;
	}

    //some more diag
	if(accepted==WORSE_ACCEPTED_STEP || accepted==REJECTED_STEP)
		worse.push_back(1);
	else
		worse.push_back(0);
	if(accepted==WORSE_ACCEPTED_STEP)
		worse_acc.push_back(1);
	if(accepted==REJECTED_STEP)
		worse_acc.push_back(0);
	return 0;
}



int mcmc::write_results(){
    //write parameter points
    write_map(parameter_evol,output_prefix+"_mcmc_points.dat");
    
    //write logp evolution
    write_vector(out_chi_evol,output_prefix+"_logp_evolution.dat");
    
    //Write some diagnostic data (some not)
	//write acc_rates
	for (auto& param : acc_ratio_s)
		write_vector(param.second,output_prefix+"_diagnostic_acc_ratios_"+param.first+".dat");

	//write sigmas 
	for (auto& param : sigmas_evol)
		write_vector(param.second,output_prefix+"_diagnostic_sigmas_evol_"+param.first+".dat");

    //Write best fit parameters
	//to stdout
	std::cout<<"\nBest params:\n";
    for (auto& param : best_parameters){
        std::string output = std::string(param.first);
        output.resize(20,' ');
		std::cout<<output<<"=\t"<<param.second<<std::endl;
    }
	std::cout<<"log(P)              =\t"<<best_chi<<std::endl;
	//to file
	std::ofstream outfile(output_prefix+"_best_fit_params.dat");
	outfile<<"best fit params:\n \n";
    for (auto& param : best_parameters)
		outfile<<param.first<<"\t"<<param.second<<std::endl;
	outfile<<"log(P)\t"<<best_chi<<std::endl;
	outfile.close();

	return 0;
}
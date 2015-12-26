#include "mcmc.h"

//initalize
mcmc::mcmc(){
    
	//chi values
	chi_before=DBL_MAX;
	best_chi=DBL_MAX;

	//burnin indicator
	burnin_ended=false;

	change_iter=0;
}


/*
    reads config file
 */
int mcmc::read_config(std::string input_filename){
    std::cout<<"\nreading config file ... "<<std::endl;
    
    //open and check file
	std::ifstream infile(input_filename.c_str(), std::ifstream::in );
	if(!(infile)) {
		std::cout<<"\nERROR CAN'T OPEN CONFIG FILE: "<<input_filename<<"\n"<<std::endl; 
		return 1; 
	}
    
	std::string str;
	while(getline(infile,str)){
		std::stringstream sstr;
		std::vector<std::string> tempvec;
		std::vector<double> emptyvec;

		sstr<<str;	
		while(sstr>>str)
			tempvec.push_back(str);

		if (tempvec.size() !=0 ){
            if( tempvec[0]=="parameter"){ //initial parameters
				parameters.insert(std::pair<std::string,double> (tempvec[1],atof(tempvec[2].c_str())));
				best_parameters.insert(std::pair<std::string,double> (tempvec[1],atof(tempvec[2].c_str())));
				parameter_evol.insert(std::pair<std::string,std::vector<double> > (tempvec[1],emptyvec));
				steps.insert(std::pair<std::string,double> (tempvec[1],0));

				//stuff for controlling step size, initial step size hardcoded
				sigmas.insert(std::pair<std::string,double> (tempvec[1],0.2));
				sigmas_evol.insert(std::pair<std::string,std::vector<double> > (tempvec[1],emptyvec));
				acc_s.insert(std::pair<std::string,std::vector<double> > (tempvec[1],emptyvec));
				acc_ratio_s.insert(std::pair<std::string,std::vector<double> > (tempvec[1],emptyvec));
			}
            else if( tempvec[0]=="bounds"){ //parameter boundaries
				param_lower_bound.insert(std::pair<std::string,double> (tempvec[1],atof(tempvec[2].c_str())));
				param_upper_bound.insert(std::pair<std::string,double> (tempvec[1],atof(tempvec[3].c_str())));
			}
            else if( tempvec[0]=="fix"){ //fixing params
				if (tempvec[2]=="yes")
					fix_parameters.insert(std::pair<std::string,double> (tempvec[1],true));
				else if (tempvec[2]=="no")
					fix_parameters.insert(std::pair<std::string,double> (tempvec[1],false));
			}
            else if (tempvec[0]=="maxiter") // maximum number of iterations
                this->maxiter=(int) strtol(tempvec[1].c_str(),NULL,10);
            else if (tempvec[0]=="output_dir") //output directory
                this->output_dir= tempvec[1];
		}
	}
	infile.close();
    
	//initialize parameter iterator
	param_iter=parameters.begin();
    
    //initial report on stdout
    this->print_config_params();
    
    return 0;
}

/*
 initial report to stdout
 */
int mcmc::print_config_params(){

	//print info
	std::cout<<"\nInitial parameters guesses:"<<std::endl;
	for (auto& param: parameters )
		std::cout<<param.first<<"\t\t"<<param.second<<std::endl;

	std::cout<<"\nFix parameters:"<<std::endl;
	for (auto& param: fix_parameters )
		std::cout<<param.first<<"\t\t"<<param.second<<std::endl;

	std::cout<<"\nbounds :"<<std::endl;
	for (auto& param: param_lower_bound )
		std::cout<<param.first<<"\t\t"<<param.second<<"\t\t"<<param_upper_bound[param.first]<<std::endl;

    std::cout<<"\n";
	return 0;
}


//opt acc is the optimal acceptace ratio
int mcmc::control_step_size(double opt_acc){

	int control_window=500;
	///////////////////////////////////////////////////
	//counting average values
	///////////////////////////////////////////////////

	//the equality is not neccesary
	size_t int_window=control_window;

	//count some average values for diagnostics
	if(iter%control_window==1 && iter > (int) int_window)
	{
		double mean=0;
		if ( worse_acc.size()>int_window)
		{
			for(size_t i=0;i<int_window;i++)
				mean+=worse_acc[worse_acc.size()-int_window+i];

			worse_acc_ratio.push_back(mean/int_window);
		}

		mean=0;
		if (worse.size()>int_window)
		{
			for(size_t i=0;i<int_window;i++)
				mean+=worse[worse.size()-int_window+i];

			worse_rate.push_back(mean/int_window);
		}
	

		for (auto& param : acc_s )
		{
			mean=0;
			if (param.second.size()>int_window)
			{
				for(size_t i=0;i<int_window;i++)
					mean+=param.second[param.second.size()-int_window+i];
				acc_ratio_s[param.first].push_back(mean/int_window);
			}
		}
	}


	///////////////////////////////////////////////////
	//control the step size
	///////////////////////////////////////////////////
	//control mechanism is now pure proportional
	//integral just makes stability worse
	//differential is said to be hard to do right

	for (auto& param : acc_ratio_s){
		if( param.second.size()>1 && iter%control_window==1 && iter>3 ){
			double weigth_prop=0.6;
			//weigth int is 0, means no integral control!!
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
				return 1;
			}
		}
		//record the sigma for diagnostics
		sigmas_evol[param.first].push_back(sigmas[param.first]);
	}
	

	return 0;
}


//change parameters
//this will be called at every iteration
//in the markov chain
int mcmc::change_params(){
    int status=0;
    
    //first control the step size
    this->control_step_size(0.5);
	
	///////////////////////////////////////////////////
	//create jump	
	///////////////////////////////////////////////////

	//choose parameter to change
	int noparams=(int) parameters.size();
	//iterates parameters in a loop until 
	//it finds a paramteres that is not fixed
	do{ 
		if ( change_iter % noparams == 0 )
			param_iter=parameters.begin();
		else
			++param_iter;
	
		change_iter++;
	
	}while (fix_parameters[param_iter->first]==true);
	//std::cout<<"parameter chosen is "<<param_iter->first<<std::endl;


	//jump

	//intialize normal distribution random generator
	std::normal_distribution<double> distribution(0,sigmas[param_iter->first]);
	do{
		status=0;

		//create step 
		steps[param_iter->first] = param_iter->second * distribution(generator);
		//add
		param_iter->second += steps[param_iter->first];

		//check boundaries
		if (	param_iter->second < param_lower_bound[param_iter->first] ||
			param_iter->second > param_upper_bound[param_iter->first] )
		{
			//std::cout<<"Warning, boundary reached: "<< param.first<< "\t"<<param.second<<std::endl;
			status=1;
		}

		//if boundary reached step back	
		if (status==1)
			param_iter->second -= steps[param_iter->first];

	//repeat until acceptable step is created
	}while(status==1);

	//std::cout<<"step succesfully generated "<<std::endl;
	return status;
}

//evaluate the chi (P) parameters, with Metropolis-Hastings
//should change cryptic acceptance codes
int mcmc::evaluate_step(double input_logp){
//	std::cout<<"chi arrived"<<std::endl;
	chi=input_logp;

	//evaluating chi
	if (best_chi > chi)
		accepted=0;
	else if (chi_before>chi)
		accepted=1;
	//if chi is no better than the one before
	//Metropolis Hastings rejection/acceptance step
	else {
		double limit=exp(chi_before-chi) * RAND_MAX;
		double rand_num=rand();

		if(rand_num > limit )
			accepted=3;	//rejection
		else
			accepted=2;	//acceptance
	}
    
    //record the point parameter space
    this->record_data();
    
	return 0;
}


//this function is called in every iteration and records 
//the parameter values in the step, and the accceptance rate
//and other diagnostic data
int mcmc::record_data(){
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

	if(accepted==0 || accepted==1 || accepted==2  ){ //step was accepted
		chi_before=chi;
		out_acc_chi_evol.push_back(chi);
		acc_s[param_iter->first].push_back(1);

		//record temp data to parameter chain
		//now i do not record points arter
		//rejected step
		//that might would result in 
		//a bit different histograms

		//push back parameter to evolution vector	
		//now it prints all steps, burn in too!!!!
		//if(burnin_ended)
			for (auto& param : parameter_evol)
				param.second.push_back(parameters[param.first]);
	}
	else { //not accepted
		//step back
		param_iter->second -= steps[param_iter->first];
		
		out_acc_chi_evol.push_back(0);
		acc_s[param_iter->first].push_back(0);
	}
    if (accepted==0){ //the best chi
		for (auto& param : parameters)
			best_parameters[param.first] = param.second;
		best_chi=chi;
	}

	if(accepted==2 || accepted==3) //worse step
		worse.push_back(1);
	else //better step
		worse.push_back(0);
	if(accepted==2) //worse but accepted
		worse_acc.push_back(1);
	if(accepted==3) //worse and not accepted
		worse_acc.push_back(0);
	return 0;
}



int mcmc::write_results(){
    /*
        write parameter points
     */
    write_map(parameter_evol,output_dir+"/mcmc_points.dat");
    
    //write logp evolution
    write_vector(out_chi_evol,output_dir+"/logp_evolution.dat");

    /*
        Write some diagnostic data (some not)
     */
    //write diagnostic data
	//write acc_rates
	for (auto& param : acc_ratio_s)
		write_vector(param.second,output_dir+"/diagnostic_acc_ratios_"+param.first+".dat");

	//write sigmas 
	for (auto& param : sigmas_evol)
		write_vector(param.second,output_dir+"/diagnostic_sigmas_evol_"+param.first+".dat");

    /*
	write_vector(out_best_chi_evol,output_dir+"/best_chi_evol.dat");
	write_vector(out_acc_chi_evol,"../output/acc_chi_evol.dat");
	write_vector(worse_acc_ratio,output_dir+"/worse-acc-rate.dat");
	write_vector(acc_ratio,"../output/acc-rate.dat");
	write_vector(worse_rate,output_dir+"/worse-rate.dat");
     */


    /*
        Write best fit parameters
     */
	//report to stdout	
	std::cout<<"\n\nbest params:\n";
	for (auto& param : best_parameters)
		std::cout<<param.first<<"\t=\t"<<param.second<<std::endl;
	std::cout<<"log(P)\t=\t"<<best_chi<<std::endl;

	//create fitted param file
	std::ofstream outfile(output_dir+"/best_fit_params.dat");
	outfile<<"best fit params:\n \n";
	for (auto& param : best_parameters)
		outfile<<param.first<<"\t=\t"<<param.second<<std::endl;
	outfile<<"log(P)="<<best_chi<<std::endl;
	outfile.close();

	return 0;
}



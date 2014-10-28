//#include <iostream>
//#include <sstream>

#include "sps_mcmc.h"

//initalize
sps_mcmc::sps_mcmc()
{
	//chi values
	chi_before=DBL_MAX;
	best_chi=DBL_MAX;

	//burnin indicator
	burnin_ended=false;

	change_iter=0;
}


//reads config file
//config file must have 2 columns
int sps_mcmc::read_config(std::string input_filename)
{
	std::ifstream infile(input_filename.c_str(), std::ifstream::in );
	std::string str;
	while(getline(infile,str))
	{
		std::stringstream sstr;
		std::vector<std::string> tempvec;
		sstr<<str;	
		while(sstr>>str)
		{
			tempvec.push_back(str);
		}

		if (tempvec.size() !=0 )
		{	
			//initializing parameters
			if( tempvec[0]=="parameter")
			//need more error checking 
			{
				parameters.insert(std::pair<std::string,double> (tempvec[1],atof(tempvec[2].c_str())));
				best_parameters.insert(std::pair<std::string,double> (tempvec[1],atof(tempvec[2].c_str())));
				parameter_evol.insert(std::pair<std::string,std::vector<double> > (tempvec[1],{}));
				steps.insert(std::pair<std::string,double> (tempvec[1],0));

				//stuff for controlling step size
				//this might be in some advanced config file
				//initial step size hardcoded 
				sigmas.insert(std::pair<std::string,double> (tempvec[1],0.2));
				sigmas_evol.insert(std::pair<std::string,std::vector<double> > (tempvec[1],{}));
				acc_s.insert(std::pair<std::string,std::vector<double> > (tempvec[1],{}));
				acc_ratio_s.insert(std::pair<std::string,std::vector<double> > (tempvec[1],{}));
			}	
	
			//parameter boundaries
			else if( tempvec[0]=="bounds")
			//need more error checking 
			{
				param_lower_bound.insert(std::pair<std::string,double> (tempvec[1],atof(tempvec[2].c_str())));
				param_upper_bound.insert(std::pair<std::string,double> (tempvec[1],atof(tempvec[3].c_str())));
			}	
			
			//fixing params
			else if( tempvec[0]=="fix")
			//need more error checking 
			{
				if (tempvec[2]=="yes")
				{
					fix_parameters.insert(std::pair<std::string,double> (tempvec[1],true));
				}
				else if (tempvec[2]=="no")
				{
					fix_parameters.insert(std::pair<std::string,double> (tempvec[1],false));
				}
			}	
		}
	}
	infile.close();

	//initialize parameter iterator
	param_iter=parameters.begin();

	//print info
	std::cout<<"\n\nInitial parameters guesses:"<<std::endl;
	for (auto& param: parameters )
	{
		std::cout<<param.first<<"\t"<<param.second<<std::endl;
	}

	std::cout<<"\n\nFix parameters:"<<std::endl;
	for (auto& param: fix_parameters )
	{
		std::cout<<param.first<<"\t"<<param.second<<std::endl;
	}

	std::cout<<"\n\nbounds :"<<std::endl;
	for (auto& param: param_lower_bound )
	{
		std::cout<<param.first<<"\t"<<param.second<<"\t"<<param_upper_bound[param.first]<<std::endl;
	}

	return 0;
}


//opt acc is the optimal acceptace ratio
int sps_mcmc::control_step_size(double opt_acc)
{

	int control_window=500;
	///////////////////////////////////////////////////
	//counting average values
	///////////////////////////////////////////////////

	//the equality is not neccesary
	int int_window=control_window;

	//count some average values for diagnostics
	if(iter%control_window==1 && iter>int_window)
	{
		double mean=0;
		if (worse_acc.size()>int_window)
		{
			for(int i=0;i<int_window;i++)
				mean+=worse_acc[worse_acc.size()-int_window+i];

			worse_acc_ratio.push_back(mean/int_window);
		}

		mean=0;
		if (worse.size()>int_window)
		{
			for(int i=0;i<int_window;i++)
				mean+=worse[worse.size()-int_window+i];

			worse_rate.push_back(mean/int_window);
		}
	

		for (auto& param : acc_s )
		{
			mean=0;
			if (param.second.size()>int_window)
			{
				for(int i=0;i<int_window;i++)
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

	for (auto& param : acc_ratio_s)
	{
		//if( param.second.size()>1 && iter%200==1 && iter>3 )
		if( param.second.size()>1 && iter%control_window==1 && iter>3 )
		{
			double weigth_prop=0.6;
			//weigth int is 0, means no integral control!!
			double weigth_int=0;
			double weigth_all=1;

			//proportional error value
			double prop_err=param.second[param.second.size()-1]-opt_acc;

			//calculate integral error value
			double int_err=0;
			for(int i=100;i<param.second.size();i++)
				int_err+=(param.second[i]-opt_acc);


			//modify control variable
			//now there is the scale of the control var
			sigmas[param.first]+=weigth_all*sigmas[param.first]*(weigth_prop*prop_err+weigth_int*int_err);

			//this should not happen	
			if(sigmas[param.first]>1)
			{
				std::cout<<"WARNING sigma > 1";
			}

			//this should not happen	
			if(sigmas[param.first]<0)
			{
				std::cout<<"ERROR sigma < 0";
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
int sps_mcmc::change_params()
{
	//error variable
	int status=0;

	///////////////////////////////////////////////////
	//create jump	
	///////////////////////////////////////////////////

	//choose parameter to change

	int noparams=parameters.size();
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
int sps_mcmc::evaluate_chi(double input_chi)
{
//	std::cout<<"chi arrived"<<std::endl;
	chi=input_chi;

	//evaluating chi
	if (best_chi > chi)
		accepted=0;
	else if (chi_before>chi)
		accepted=1;
	//if chi is no better than the one before
	//Metropolis Hastings rejection/acceptance step
	else 
	{
		double limit=exp(chi_before-chi) * RAND_MAX;
		double rand_num=rand();

		if(rand_num > limit )
			accepted=3;	//rejection
		else
			accepted=2;	//acceptance
	}
	return 0;
}


//this function is called in every iteration and records 
//the parameter values in the step, and the accceptance rate
//and other diagnostic data
int sps_mcmc::record_data()
{
	int status=0;
	
	out_chi_evol.push_back(chi);
	out_best_chi_evol.push_back(best_chi);

	//check if burnin has ended
	if (burnin_ended==false )
	{
		int window=5000;
		if ( iter > window )
		{
			if (chi > out_chi_evol[iter-window])
			{
				burnin_ended=true;
				std::cout<<"\nBurnin section ended at: "<<iter<<"\n"<<std::endl;
			}	
		}
	}
	

	if(accepted==0 || accepted==1 || accepted==2  ) //step was accepted
	{
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
	else //not accepted
	{
		//step back
		param_iter->second -= steps[param_iter->first];
		
		out_acc_chi_evol.push_back(0);
		acc_s[param_iter->first].push_back(0);
	}

	if (accepted==0) //the best chi 
	{
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



int sps_mcmc::write_results()
{
	//write point evolutions
	for (auto& param : parameter_evol)
		write_vector(param.second,"../output/"+param.first+".dat");

	//write acc_rates
	for (auto& param : acc_ratio_s)
		write_vector(param.second,"../output/acc_ratios_"+param.first+".dat");

	//write sigmas 
	for (auto& param : sigmas_evol)
		write_vector(param.second,"../output/sigmas_evol_"+param.first+".dat");

	//write diagnostic data
	write_vector(out_chi_evol,"../output/chi_evol.dat");
	write_vector(out_best_chi_evol,"../output/best_chi_evol.dat");
//	write_vector(out_acc_chi_evol,"../output/acc_chi_evol.dat");
	write_vector(worse_acc_ratio,"../output/worse-acc-rate.dat");
//	write_vector(acc_ratio,"../output/acc-rate.dat");
	write_vector(worse_rate,"../output/worse-rate.dat");


	//report to stdout	
	std::cout<<"\nbest params:\n\n";
	for (auto& param : parameters)
		std::cout<<param.first<<"\t=\t"<<param.second<<std::endl;
	std::cout<<"log(P)\t~\t"<<best_chi<<std::endl;


	//create fitted param file
	std::ofstream outfile("../output/fitted-params.dat");
	//Checking filename
	if(!(outfile))
	{
		std::cout<<"ERROR INVALID OUTPUT FILE: ../output/fitted-params.dat"<<std::endl;
		return 1;
	}
	outfile<<"best params:\n \n";
	for (auto& param : parameters)
		outfile<<param.first<<"\t=\t"<<param.second<<std::endl;
	outfile<<"log(P)="<<best_chi<<std::endl;
	outfile.close();

	//info out
	std::cout<<"writing succesful: "<<std::endl;

	return 0;
}



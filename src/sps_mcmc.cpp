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

	//initial sigma parameter of the normal distribution
	// of steps is the markov chain
	// the step is relative so it is 0.8% now
	sigma=0.008;
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
				steps.insert(std::pair<std::string,double> (tempvec[1],0));
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


//change parameters
//this will be called at every iteration
//in the markov chain
//
//opt acc is the optimal acceptace ratio
int sps_mcmc::change_params(double opt_acc)
{
	//error variable
	int status=0;

	//control the step size
	//better control mechanism should be used (PID)
	if(acc_ratio.size()>1 && iter%200==1 && iter>3 )
	{
		if (acc_ratio[acc_ratio.size()-1]>opt_acc && sigma < 0.5)
			sigma=sigma+sigma*(acc_ratio[acc_ratio.size()-1]-opt_acc);
		
		if (acc_ratio[acc_ratio.size()-1]<opt_acc)
			sigma=sigma-sigma*(opt_acc-acc_ratio[acc_ratio.size()-1]);

		//std::cout<<"\n acc_rate="<<acc_ratio[acc_ratio.size()-1]<<"\n";
	}

	//this should not happen	
	if(sigma<0)
	{
		std::cout<<"ERROR sigma < 0";
		return 1;
	}




	//create jump

	//intialize normal distribution random generator
	std::normal_distribution<double> distribution(0,sigma);
	do{
		status=0;

		//create step
		for (auto& param : parameters)
		{
			if (fix_parameters[param.first]==false)
				steps[param.first] = param.second * distribution(generator);
			else
				steps[param.first] = 0;

			param.second += steps[param.first];
		}

		//check boundaries
		for (auto& param : parameters)
		{
			if (	param.second < param_lower_bound[param.first] ||
				param.second > param_upper_bound[param.first] )
			{
				//std::cout<<"Warning, boundary reached: "<< param.first<< "\t"<<param.second<<std::endl;
				status=1;
			}
		}

		//if boundary reached step back	
		if (status==1)
		{
			for (auto& param : parameters)
			{
				param.second -= steps[param.first];
			}
		}

	//repeat until acceptable step is created
	}while(status==1);


	return status;
}

//evaluate the chi (P) parameters, with Metropolis-Hastings
//should change cryptic acceptance codes
int sps_mcmc::evaluate_chi(double input_chi)
{
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
		acc.push_back(1);

		//record temp data to parameter chain
		//now i do not record points arter
		//rejected step
		//that might would result in 
		//a bit different histograms
		std::vector<double> temp_point;
		temp_point.resize(6);
	
		/*int i=0;
		for (auto& param : parameters)
		{
			temp_point[i] = param.second;
			i++;
		}
		*/
	
		//this is hardcoded beacause of the too simple
		//plotting tool, but this is lame
		//plotter should be changed 
		temp_point[0]=parameters["dust_tau_v"];
		temp_point[1]=parameters["dust_mu"];
		temp_point[2]=parameters["sfr_tau"];
		temp_point[3]=parameters["age"];
		temp_point[4]=parameters["metall"];
		temp_point[5]=parameters["vdisp"];

		//now it prints all steps, burn in too!!!!
		//if(burnin_ended)
			points.push_back(temp_point); 
		
	}
	else //not accepted
	{
		//step back
		for (auto& param : parameters)
			param.second -= steps[param.first];
		
		out_acc_chi_evol.push_back(0);
		acc.push_back(0);
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

	//counting average values

	//count some average values for diagnostics
	if(iter%200==0)
	{
		double mean=0;
		if (worse_acc.size()>100)
		{
			for(int i=0;i<100;i++)
				mean+=worse_acc[worse_acc.size()-100+i];

			worse_acc_ratio.push_back(mean/100);
		}

		mean=0;
		if (worse.size()>100)
		{
			for(int i=0;i<100;i++)
				mean+=worse[worse.size()-100+i];

			worse_rate.push_back(mean/100);
		}
	
		mean=0;

		if (acc.size()>100)
		{
			for(int i=0;i<100;i++)
				mean+=acc[acc.size()-100+i];
				
			acc_ratio.push_back(mean/100);
		}
	}

	return 0;
}



int sps_mcmc::write_results()
{
	//write point evolutions
	write_table_row(points,"../output/points.dat");

	//write diagnostic data
	write_vector(out_chi_evol,"../output/chi_evol.txt");
	write_vector(out_best_chi_evol,"../output/best_chi_evol.txt");
	write_vector(out_acc_chi_evol,"../output/acc_chi_evol.txt");
	write_vector(worse_acc_ratio,"../output/worse-acc-rate.dat");
	write_vector(acc_ratio,"../output/acc-rate.dat");
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



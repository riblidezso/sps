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
//
//ok
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
		if( tempvec.size()>0)
		{
			config_map.insert( std::pair<std::string,std::string> (tempvec[0],tempvec[1]) );
		}
	}
	infile.close();
	return 0;
}


//set initial parameters from config map
//
//ok
int sps_mcmc::set_initial_params()
{
	std::cout<<"\nSetting initial parameter guesses:\n";	
	
	//set dust_tau_v
	if (config_map.count("init_guess_dust_tau_v")==1)
	{
		dust_tau_v=atof(config_map["init_guess_dust_tau_v"].c_str());
		std::cout<<"initial guess for dust_tau_v\t\t"<<dust_tau_v<<std::endl;
	}
	else
	{
		std::cout<<"initial guess for dust_tau_v not found, setting it to 1.0"<<std::endl;
		dust_tau_v=1.0;
	}

	//set dust_mu
	if (config_map.count("init_guess_dust_mu")==1)
	{
		dust_mu=atof(config_map["init_guess_dust_mu"].c_str());
		std::cout<<"initial guess for dust_mu\t\t"<<dust_mu<<std::endl;
	}
	else
	{
		std::cout<<"initial guess for dust_mu not found, setting it to 0.3"<<std::endl;
		dust_mu=0.3;
	}

	//set sfr_tau 
	if (config_map.count("init_guess_sfr_tau")==1)
	{
		sfr_tau=atof(config_map["init_guess_sfr_tau"].c_str());
		std::cout<<"initial guess for sfr_tau\t\t"<<sfr_tau<<std::endl;
	}
	else
	{
		std::cout<<"initial guess for sfr_tau not found, setting it to 3.0e+08"<<std::endl;
		sfr_tau=3.0e+08;
	}

	//set metall 
	if (config_map.count("init_guess_metall")==1)
	{
		metall=atof(config_map["init_guess_metall"].c_str());
		std::cout<<"initial guess for metall\t\t"<<metall<<std::endl;
	}
	else
	{
		std::cout<<"initial guess for metall not found, setting it to 0.01 "<<std::endl;
		metall=0.01;
	}

	//set age 
	if (config_map.count("init_guess_age")==1)
	{
		age=atof(config_map["init_guess_age"].c_str());
		std::cout<<"initial guess for age\t\t\t"<<age<<std::endl;
	}
	else
	{
		std::cout<<"initial guess for age not found, setting it to 2e+09 "<<std::endl;
		age=2e+09;
	}

	//set vdisp 
	if (config_map.count("init_guess_vdisp")==1)
	{
		vdisp=atof(config_map["init_guess_vdisp"].c_str());
		std::cout<<"initial guess for vdisp\t\t"<<vdisp<<std::endl;
	}
	else
	{
		std::cout<<"initial guess for vdisp not found, setting it to 0.0003 "<<std::endl;
		metall=0.0003;
	}
	std::cout<<"\n";	

	return 0;
}

//fix parameters if needed
//
//ok
int sps_mcmc::fix_params()
{
	std::cout<<"\nFixing parameters :\n";	
	
	//set dust_tau_v
	if (config_map.count("fix_dust_tau_v")==1)
	{
		fix_dust_tau_v=true;
		std::cout<<"dust_tau_v\tfixed"<<std::endl;
	}
	else
	{
		fix_dust_tau_v=false;
		std::cout<<"dust_tau_v\tnot fixed"<<std::endl;
	}

	//set dust_mu
	if (config_map.count("fix_dust_mu")==1)
	{
		fix_dust_mu=true;
		std::cout<<"dust_mu\t\tfixed"<<std::endl;
	}
	else
	{
		fix_dust_mu=false;
		std::cout<<"dust_mu\t\tnot fixed"<<std::endl;
	}

	//set sfr_tau 
	if (config_map.count("fix_sfr_tau")==1)
	{
		fix_sfr_tau=true;
		std::cout<<"sfr_tau\t\tfixed"<<std::endl;
	}
	else
	{
		fix_sfr_tau=false;
		std::cout<<"sfr_tau\t\tnot fixed"<<std::endl;
	}

	//set metall 
	if (config_map.count("fix_metall")==1)
	{
		fix_metall=true;
		std::cout<<"metall\t\tfixed"<<std::endl;
	}
	else
	{
		fix_metall=false;
		std::cout<<"metall\t\tnot fixed"<<std::endl;
	}

	//set age 
	if (config_map.count("fix_age")==1)
	{
		fix_age=true;
		std::cout<<"age\t\tfixed"<<std::endl;
	}
	else
	{
		fix_age=false;
		std::cout<<"age\t\tnot fixed"<<std::endl;
	}

	//set vdisp 
	if (config_map.count("fix_vdisp")==1)
	{
		fix_vdisp=true;
		std::cout<<"vdisp\t\tfixed"<<std::endl;
	}
	else
	{
		fix_vdisp=false;
		std::cout<<"vdisp\t\tnot fixed"<<std::endl;
	}
	std::cout<<"\n";	

	return 0;
}


//change parameters
//this will be called at every iteration
//in the markov chain
//
//opt acc is the optimal acceptace ratio
//
//ok
int sps_mcmc::change_params(double opt_acc)
{
	//error variable
	int status=0;

	//control the step size
	//better control mechanism should be used
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

		//creating normal distribution random jump
		if (fix_dust_tau_v==false)
			d_dust_tau_v= dust_tau_v * distribution(generator);
		else
			d_dust_tau_v=0;
		dust_tau_v+=d_dust_tau_v;

		if (fix_dust_mu==false)
			d_dust_mu= dust_mu * distribution(generator) ;
		else
			d_dust_mu=0;
		dust_mu+=d_dust_mu;

		if (fix_sfr_tau==false)
			d_sfr_tau= sfr_tau *  distribution(generator) ;
		else
			d_sfr_tau=0;
		sfr_tau+=d_sfr_tau;

		if (fix_age==false)
			d_age= age *  distribution(generator) ;
		else
			d_age=0;
		age+=d_age;

		if (fix_metall==false)
			d_metall= metall * distribution(generator) ;
		else
			d_metall=0;
		metall+=d_metall;

		if (fix_vdisp==false)
			d_vdisp= vdisp * distribution(generator) ;
		else
			d_vdisp=0;
		vdisp+=d_vdisp;

		//check boundaries
		//should be in config file
		if(	dust_mu <=0 || dust_mu >1 || 
			dust_tau_v <0 || dust_tau_v>1.5 ||
			age<1e+8 || age>2e+10 ||
			sfr_tau>40e+19	|| sfr_tau<=1e+7 || 
			metall<0.0001 || metall>0.05 || 
			vdisp > 0.002)
			
		{
			dust_tau_v-=d_dust_tau_v;
			dust_mu-=d_dust_mu;
			sfr_tau-=d_sfr_tau;
			age-=d_age;
			metall-=d_metall;
			vdisp-=d_vdisp;

			status=1;
		}
	}while(status==1);


	return status;
}

//
//ok
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


//this function is called in everz iteration and records 
//the parameter values in the step, and the accceptance rate
//and other diagnostic data
//
//ok
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

		temp_point[0]=dust_tau_v;
		temp_point[1]=dust_mu;
		temp_point[2]=sfr_tau;
		temp_point[3]=age;
		temp_point[4]=metall;
		temp_point[5]=vdisp;
		if(burnin_ended)
			points.push_back(temp_point); 
		
	}
	else //not accepted
	{
		dust_tau_v-=d_dust_tau_v;
		dust_mu-=d_dust_mu;
		sfr_tau-=d_sfr_tau;
		age-=d_age;
		metall-=d_metall;
		vdisp-=d_vdisp;
		out_acc_chi_evol.push_back(0);
		acc.push_back(0);
	}

	if (accepted==0) //the best chi 
	{
		best_chi=chi;
		best_dust_tau_v=dust_tau_v;
		best_dust_mu=dust_mu;
		best_sfr_tau=sfr_tau;
		best_age=age;
		best_metall=metall;
		best_vdisp=vdisp;
					
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

	//count some average values
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

//
//ok
int sps_mcmc::write_results()
{
	write_table_row(points,"../output/points.dat");

	write_vector(out_chi_evol,"../output/chi_evol.txt");
	write_vector(out_best_chi_evol,"../output/best_chi_evol.txt");
	write_vector(out_acc_chi_evol,"../output/acc_chi_evol.txt");
	write_vector(worse_acc_ratio,"../output/worse-acc-rate.dat");
	write_vector(acc_ratio,"../output/acc-rate.dat");
	write_vector(worse_rate,"../output/worse-rate.dat");
	
	std::cout<<"\nbest params:\n\n";
	std::cout<<"dust_tau_v="<<best_dust_tau_v<<std::endl;
	std::cout<<"dust_mu="<<best_dust_mu<<std::endl;
	std::cout<<"sfr_tau="<<best_sfr_tau<<std::endl;
	std::cout<<"age="<<best_age<<std::endl;
	std::cout<<"metall="<<best_metall<<std::endl;
	std::cout<<"veloc_disp="<<best_vdisp<<std::endl;
	std::cout<<"chisquare="<<best_chi<<std::endl;

	std::ofstream outfile("../output/fitted-params.dat");
	//Checking filename
	if(!(outfile))
	{
		std::cout<<"ERROR INVALID OUTPUT FILE: ../output/fitted-params.dat"<<std::endl;
		return 1;
	}

	outfile<<"best params:\n \n";
	outfile<<"dust_tau_v="<<best_dust_tau_v<<std::endl;
	outfile<<"dust_mu="<<best_dust_mu<<std::endl;
	outfile<<"sfr_tau="<<best_sfr_tau<<std::endl;
	outfile<<"age="<<best_age<<std::endl;
	outfile<<"metall="<<best_metall<<std::endl;
	outfile<<"veloc_disp="<<best_vdisp<<std::endl;
	outfile<<"log(P)="<<best_chi<<std::endl;
	
	outfile.close();

	//info out
	std::cout<<"writing succesful: "<<std::endl;
	return 0;
}



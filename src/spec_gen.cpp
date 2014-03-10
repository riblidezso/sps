#include "spec_gen.h"
#include "read.h"


void spec_gen::cpy(read& input)
{
	wavelengths=input.wavelengths;
	nspecsteps=wavelengths.size();

	time=input.time;
	ntimesteps=time.size();
}


//gets dust parameters and modifies data
//
//gets dust_tauv, dust_mu and modifies model data
//calls dust_modif
//
int spec_gen::usr_get_dust()
{
	bool tryagain;	
	std::cout<<"\nDust"<<std::endl;
	do{
		tryagain=false;
		try
		{
			//get option
			std::string temp_line;
			std::cout<<"include attenuation by dust?: y/[n] ";
			getline(std::cin,temp_line);
			std::stringstream temp_sstr;
			temp_sstr<<temp_line;
			char option='0';
			temp_sstr>>option;
			
			//read parameters 
			if(option=='y' || option=='Y')
			{
				std::cout<<"Using parametrization described by Charlot & Fall (see documentation for more info)\n ";

				//get the line of ages
				std::cout<<"\nTau_v parameter: (default is: 1) ";
				std::string temp_line;
				getline(std::cin,temp_line);
				std::stringstream sstr;
				sstr<<temp_line;
				sstr>>dust_tau_v;
				//if no value was given we use default
				if (temp_line.size()==0) dust_tau_v=1;

				//get the line of ages
				std::cout<<"Mu parameter: (default is: 0.3 ) ";
				std::string temp_line1;
				getline(std::cin,temp_line1);
				std::stringstream sstr1;
				sstr1<<temp_line1;
				sstr1>>dust_mu;
				//if no value was given we use default
				if (temp_line1.size()==0) dust_mu=0.3;
				dust_option=0;

			}
			else if(option=='n' || option=='N')
			{
				dust_option=1;			
			}
			else
			{
				std::cout<<"\nERROR: WRONG OPTION\n";
				throw 1;
			}
		}
		catch (int ex)
		{
			if (ex==1){tryagain=true;}
		}
	}while(tryagain);

	return 0;
}

///
//sets dust parameters 
//calls dust_modif
//
void spec_gen::set_dust(double tau_v_value, double mu_value)
{
	dust_tau_v=tau_v_value;
	dust_mu=mu_value;
	dust_modif();
	return;
}


//gets the ages of galaxies
//
//either from a file or from stdin
//
int spec_gen::usr_get_age()
{
	bool tryagain;	
	std::cout<<"\nGetting age to convolve to"<<std::endl;
	do{
		tryagain=false;
		try
		{
			//get the line of ages
			std::cout<<"write the ages to convolve to ( in Gyrs): ";
			std::string temp_line;
			getline(std::cin,temp_line);
			std::stringstream sstr;
			sstr<<temp_line;
			std::string token;
			double temp_d;
			if(sstr>>temp_d)
				age=temp_d*1e+9;
			else 
				throw 1;
		}
		catch (int ex)
		{
			if (ex==1){tryagain=true;}
		}
	}while(tryagain);

	return 0;
}

void spec_gen::set_age(double age_value)
{
	age=age_value*1e+9;
}

//gets sfr parameters
//
int spec_gen::usr_get_sfr()
{
	std::cout<<"\nGetting star formation rate"<<std::endl;

	bool tryagain;
	do
	{
		tryagain=false;
		try
		{
			std::cout<<"Give the Tau of the exponential star formation rate, in Gy: ";
			std::string temp_line;
			getline(std::cin,temp_line);
			std::stringstream temp_sstr;
			temp_sstr<<temp_line;
			double temp_d=0;
			if(temp_sstr>>temp_d)
				tau=temp_d*1e+9;
			else
				throw 1;			

		}catch(int e)
		{
		if (e==1) tryagain=true;
		}

	}while(tryagain);

	return 0;
}


void spec_gen::set_sfr(double tau_value)
{
	tau=tau_value*1e+9;
}

//returns star formation rate
//
double spec_gen::sfr(double time, double tau)
{
 	return exp(-time/tau);
}

int spec_gen::usr_get_imf()
{
	std::cout<<"\nGetting initial mass function"<<std::endl;

	bool tryagain;
	do
	{
		tryagain=false;
		try
		{
			std::cout<<"Choose the IMF to use: [chabrier/sapleter] :";
			std::string temp_line;
			getline(std::cin,temp_line);
			std::stringstream temp_sstr;
			temp_sstr<<temp_line;
			if(!(temp_sstr>>imf))
				throw 1;

			if(imf != "salpeter" && imf!="chabrier")
				throw 1;

		}catch(int e)
		{
			if (e==1) tryagain=true;
		}
		

	}while(tryagain);

	return 0;
}

//gets metallicity parameter
//
int spec_gen::usr_get_metall()
{
	std::cout<<"\nGetting metallicity"<<std::endl;

	bool tryagain;
	do
	{
		tryagain=false;
		try
		{
			std::cout<<"Give the metallicity (Z) of the star population: (range: ( 0.0001< z <0.05 )";
			std::string temp_line;
			getline(std::cin,temp_line);
			std::stringstream temp_sstr;
			temp_sstr<<temp_line;
			double temp_d=0;
			if( temp_sstr>>temp_d )
				metall=temp_d;
			else
				throw 1;
			if(metall<0.0001 || metall>0.05)
				throw 2;

		}catch(int e)
		{
			if (e==1) 
			{
				tryagain=true;
				std::cout<<"running again\n";
			}
			if (e==2) 
			{
				tryagain=true;
				std::cout<<"ERROR metallicity is out of range!"<<std::endl;
			}
		}
		

	}while(tryagain);

	return 0;
}

//problem on the edges!!!!!!!!!!!!!!!
int spec_gen::metall_interpol(std::vector<std::vector<double> >& original_models)
{
	int offset;

	if( imf =="chabrier")
		offset=0;
	else if(imf=="salpeter")
		offset=6;

	if (metall!=0.05)
	{
		double model_metal[6]={0.0001,0.0004,0.004,0.008,0.02,0.05};
	
		int intpol_model;
		for(int i=0;i<6;i++)
		{
			if(model_metal[i] > metall)
			{
				intpol_model=i-1;
				break;
			}
		}

		double wd,wu,delta;

		//interpolating
	
		delta=log(model_metal[intpol_model+1] / model_metal[intpol_model]);
		wd=log(model_metal[intpol_model+1]/metall) / delta;
		wu=log(metall/model_metal[intpol_model]) / delta;

		model.resize(original_models[offset].size());
		//interpolating models around given metallicity

		for(int i=0;i<original_models[offset].size();i++)
		{
			model[i]= wd * original_models[offset+intpol_model][i] +  wu * original_models[offset+intpol_model+1][i] ;
		}
	}
	else 
	{
		//if metall==0.5 there is no upper model to interpolate with
		model.resize(original_models[offset].size());

		for(int i=0;i<original_models[offset].size();i++)
		{
			model[i]= original_models[offset+5][i];
		}
	}
	
	return 0;

}

int spec_gen::chose_model(std::vector<std::vector<double> >& original_models, int no_model)
{

	//copying

	model.resize(original_models[no_model].size());
	//interpolating models around given metallicity

	for(int i=0;i<original_models[no_model].size();i++)
	{
		model[i]= original_models[no_model][i];
	}

	return 0;

}

///
//modifies model, due to dust
//
void spec_gen::dust_modif()
{
	if(dust_option==1) return;

	//Dust factors:
	double dust_tau; 			// tau_v or mu*tau_v
	double exponent; 			// lambda/5500

	//mu is 0.3 by default in C-B
	//tau_v is 1 by default in C-B

	for(int j=0;j<nspecsteps;j++)
	{
		// exponent is const for a wavelength
		exponent=pow(wavelengths[j]/5500,-0.7);
	
		for(int i=0;i<ntimesteps;i++)
		{
			//dust_tau depends on the age of the stellar pop
			if(time[i]<1e7)
			{
				dust_tau= -dust_tau_v;
			}	
			if(time[i]>1e7)
			{
				dust_tau= -dust_mu*dust_tau_v;
			}
			
			//the modification
			model[j+i*(nspecsteps)] *= exp(dust_tau*exponent);
		}
	}
	return;
} 

//writes the result in a table format
//from stdin input filename
//
//separated with whitespaces
//no " " at the end of file
//no /n at theend of file
//
int spec_gen::usr_write_result()
{
	bool tryagain;  // indicator for loop
	int err;

	std::cout<<"\nWriting results "<<std::endl;
	do
	{
		//setting indicator
		tryagain=false;
	
		try
		{
			std::string temp_line;
			//getting output filename
			std::cout<<"\noutput filename: ";
			getline(std::cin,temp_line);
			std::stringstream temp_sstr;
			temp_sstr<<temp_line;
			std::string outfilename;
			temp_sstr>>outfilename;
	
			err= write_result(outfilename);
			if( err != 0 )
				throw 1;
		
		}catch(int error)
		{
			if(error==1)	
			{
				tryagain=true;
				std::cout<<"ERROR\n";
			}
	
		}

	}while(tryagain);
	
	return 0;
}

///
//writes the result in a table format
//
//separated with whitespaces
//no " " at the end of file
//no /n at theend of file
//
int spec_gen::write_result(std::string outfilename)
{
	int err;
	err=write_vector(conv_to_age_res,outfilename);
	if (err==0)
		std::cout<<"spectrum written in: '"<<outfilename<<"'"<<std::endl;

	std::stringstream temp_sstr;
	temp_sstr<<outfilename<<"_w";
	temp_sstr>>outfilename;

	std::vector<std::vector<double> > vecvec;
	vecvec.push_back(wavelengths);
	vecvec.push_back(conv_to_age_res);
	err |= write_table_col(vecvec,outfilename);
	if (err==0)
		std::cout<<"wavelengths and spectrum written in: '"<<outfilename<<"'"<<std::endl;
	
	return err;
}


///
//writes a vector to a file
//
//one column
int spec_gen::write_vector(std::vector<double>& vec_to_write, std::string outfilename)
{
	std::ofstream outfile(outfilename.c_str());
	//Checking filename
	if(!(outfile))
	{
		std::cout<<"ERROR INVALID OUTPUT FILE: "<<outfilename<<std::endl;
		return 1;
	}

	//writing numbers 
	for(int j=0; j<vec_to_write.size(); j++)
    {
		outfile<<vec_to_write[j]<<"\n";
    }
	outfile.close();

	//info out
	std::cout<<"writing succesful: "<<std::endl;

	return 0;
}

///
//writes a table (vector of vectors) to a file
//
//the outer vectors are in rows
//
int spec_gen::write_table_row(std::vector< std::vector<double > >& table_to_write, std::string outfilename)
{
	std::ofstream outfile(outfilename.c_str());
	//Checking filename
	if(!(outfile))
	{
		std::cout<<"ERROR INVALID OUTPUT FILE: "<<outfilename<<std::endl;
		return 1;
	}

	for(int j=0; j<table_to_write.size(); j++)
	{
		for(int k=0;k<table_to_write[j].size();k++)
		{
			outfile<<table_to_write[j][k]<<" ";
		}
		//end of lines
		outfile<<"\n";
	}
	outfile.close();

	//info out
	std::cout<<"writing succesful: "<<std::endl;

	return 0;
}

///
//writes a table (vector of vectors) to a file
//
//the outer vectors are in columns
//each vector should be of same length
//
int spec_gen::write_table_col(std::vector< std::vector<double > >& table_to_write, std::string outfilename)
{
	std::ofstream outfile(outfilename.c_str());
	//Checking filename
	if(!(outfile))
	{
		std::cout<<"ERROR INVALID OUTPUT FILE: "<<outfilename<<std::endl;
		return 1;
	}

	for(int j=0; j<table_to_write[0].size(); j++)
	{
		for(int k=0;k<table_to_write.size();k++)
		{
			outfile<<table_to_write[k][j]<<" ";
		}
		//end of lines
		outfile<<"\n";
	}
	outfile.close();

	//info out
	std::cout<<"writing succesful: "<<std::endl;

	return 0;
}


//The c++ version of the convolution:
//
//to only one age
//

int spec_gen::conv_to_age()
{
	//setting the size of the result vec
	conv_to_age_res.resize(nspecsteps); 

	for(int i=0;i<nspecsteps;i++)
	{
		double temp=0;		
		for(int j=1; (j < ntimesteps) && (time[j]<=age) ;j++) 
		{
			//the actual integrating
			temp+= (time[j]-time[j-1] ) * model[i+(j-1)*nspecsteps] * sfr(age-time[j],tau);
		}
		//tau is a contant so we only divide at the end
		conv_to_age_res[i]=temp/tau;
	}

	return 0;
}


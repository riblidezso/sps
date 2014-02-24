#include "cb_data.h"
#include "table.h"

///
//Quick test
//
//tries to open input/model.txt
//from different places
//
//uses default dust and a simple age vector 
int cb_data::quick_test()
{
	std::cout<<"testing ... "<<std::endl;

	int err=0;
	int branch=0;
	//Reading model from an ASCII file
	//tries top open it from different directories
	err=read_buff_atof("../input/model.txt");
	if (err==1)
	{
		err=read_buff_atof("input/model.txt");
		branch=1;
	}
	if (err==1)
	{
		err=read_buff_atof("model.txt");
		branch=2;
	}
	if (err==1)
	{
		err=read_buff_atof("../../input/model.txt");
		branch=3;
	}
	if(err==1)
	{
		std::cout<<"ERROR could not find model file"<<std::endl;
		return 1;
	}
	//set dust parameters and modify data
	set_dust(1,0.3);
						
	//sets the age of galaxies
	std::vector<double> temp;
	for(int i=1;i<5;i++)
	temp.push_back(i);
	set_age(temp);

	//sets sfr parameters
	set_sfr(3);

	//The c++ version of the convolution:
	conv_to_age_vector();	

	//writes the result in a table format
	if (branch==0)
	{
		err=write_convresult("../output/quicktest.txt");
	}
	if (branch==1)
	{
		err=write_convresult("output/quicktest.txt");
	}
	if (branch==2)
	{
		err=write_convresult("quicktest.txt");
	}
	if (branch==3)
	{
		err=write_convresult("../../output/quicktest.txt");
	}
	
	return 0;
}

///
//Reading model from an ASCII file
//
int cb_data::usr_read_buff_atof()
{
	std::cout<<"\nReading in ssp model data "<<std::endl;
	int error;			//indicator
	bool tryagain;		//indicator to terminate loop

	do
	{
		tryagain=false;

		// getting filename											
		std::cout<<"input filename: ";
		std::string temp_line;
		getline(std::cin,temp_line);
		std::stringstream temp_sstr;
		temp_sstr<<temp_line;
		std::string infilename;
		temp_sstr>>infilename;

		error=read_buff_atof(infilename);

		if (error!=0)
		{
			tryagain=true;
		}

	}while(tryagain);

	return 0;
}

///
//Actual reading function
//
//reads all numbers separated with whitespaces from an ASCII file 
//into a vector of doubles (rawdata)
//
int cb_data::read_buff_atof(std::string infilename)
{
	#define NUM_MAX_LEN 128 	// the size of the buffer of numbers
	#define BUFFERSIZE 1024		// the size of chunks read in binary

	//starting clock
	clock_t t1, t2;
	t1 = clock();
	
	// open file in binary i/o
	std::ifstream infile(infilename.c_str(), std::ios::binary |std::ios::ate);

	// checking filename
	if(!(infile))
	{
		std::cout<<"\nERROR CAN'T OPEN FILE: "<<infilename<<"\n"<<std::endl;
		return 1;
	}

	//info out
	std::cout<<"reading in file: '"<<infilename<<"'\n..."<<std::endl;

	// getting the size of the file
	std::ifstream::pos_type size;							
	size = infile.tellg();	
	// go to the begginging of file								
	infile.seekg (0, std::ios::beg);

	
	char* file_buf=new char[BUFFERSIZE];// to store chunks of file:
			
	int file_buf_size=BUFFERSIZE;		// the size of sensible data in the file_buffer
										// not BUFFERSIZE at the end of the file!
	
	int chunks_read=0;					// number of chunks already read	
	static char num_buffer[NUM_MAX_LEN];// to store numbers temporarily:
	int bufferpos=0;					// pointer inside num_buffer	
	int c;								//stores the current character in file:
	bool end_reading = false;			//indicator to terminate the loop					
	bool potential_number = true;		//indicator to discard numbers(ie.:"Padova1994") 	
	bool inside_number = false;			//indicator		

	do
	{	
		//checking if there are BUFFERSIZE char to read in left
		if(((chunks_read+1)*BUFFERSIZE)<size)			
		{												
			//then reading a chunk and ++ counter
			infile.read((char*)(&file_buf[0]), BUFFERSIZE);
			chunks_read++;
		}
		//reading last chunk
		else											
		{	
			//setting buffer data size,setting end_reading indicator
			infile.read((char*)(&file_buf[0]),((int)size-(chunks_read*BUFFERSIZE)));
			file_buf_size=((int)size-(chunks_read*BUFFERSIZE));
			end_reading=true;
		}
		// getting numbers out of buffer;
		for(int i=0; i<file_buf_size  ;i++) 				
		{
			c = file_buf[i];
			
			//if current character is part of a number: append it to the buffer
			if (potential_number && isdigit(c) )
			{									
				num_buffer[bufferpos] = c;	
				bufferpos++;			 				
				
				// error if the number is too long
				if (bufferpos == NUM_MAX_LEN)	
				{
					std::cerr<<"\nError: Too long numbers\n"<<std::endl;
					infile.close();
					return 2;
				}
			
				//set indicator
				inside_number=true;	
			}
			//if current character is part of a number: append it to the buffer
			else if(inside_number&&(c=='.'||c=='e'||c=='E'||c=='+'||c=='-'))
			{
				num_buffer[bufferpos] = c;
				bufferpos++;
		
				// error if the number is too long
				if (bufferpos == NUM_MAX_LEN)	
				{
					std::cerr<<"\nError: Too long numbers\n"<<std::endl;
					infile.close();
					return 2;
				}
						
				//set indicator
				inside_number=true;	
			}
			else
			{
				// if buffer contains a number, read it
				if (bufferpos > 0)
				{

					// add zero at the end of number buffer
					num_buffer[bufferpos] = 0;
					// parse number
					rawdata.push_back(atof(num_buffer));
					// reset number buffer position to zero
					bufferpos = 0;
				}

				// after whitespace there might be data to read				
				if (c==' ' || c=='\n')
				{
					potential_number=true;			
				}
				//if the last char is not a number nor a whitespace
				//then the next number is 
				//not a data to be read (like "Padova1994")
				else
				{
					potential_number=false;			
				}										
				inside_number=false;					
			}
		}
	}while(!end_reading);// if end reading is true, we should end reading
		
	infile.close();

	//get time
	t2 = clock();
	double diff = (((double)t2 - (double)t1)/CLOCKS_PER_SEC);
	
	//some info out
	std::cout<<rawdata.size()<<" numbers read"<<std::endl;
	std::cout<< "It took "<< diff <<" second(s)."<< std::endl;


	//getting important data from the raw vector
	get_data_from_model();

	return 0;
}


//gets some important data from model
//
//gets: time, wavelengths, ntimesteps, nspecsteps, offset
int cb_data::get_data_from_model()
{
	// there are 19 data in the model we do not use yet
	#define NO_TO_SKIP 19  	

	// reading in the timesteps
	ntimesteps=rawdata[0];
	for(int i=1;i<ntimesteps+1;i++)
	{
		time.push_back(rawdata[i]);
	}

	// reading the wavelengths
	nspecsteps=rawdata[ntimesteps+1+NO_TO_SKIP];
	int begin_specsteps= ntimesteps+1+NO_TO_SKIP+1;
	for(int i=0;i<nspecsteps;i++)
	{
		wavelengths.push_back(rawdata[begin_specsteps+i]);
	}

	// the beginning of model data
	offset=begin_specsteps+nspecsteps+1;

	// there are some data between the spectra, but the number of them is given in the file.
	nfillingdata=rawdata[offset+nspecsteps];
														
	return 0;
} 


//gets dust parameters and modifies data
//
//gets dust_tauv, dust_mu and modifies model data
//calls dust_modif
//
int cb_data::usr_get_dust()
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

				//modifying raw data
				dust_modif();
			}
			else if(option=='n' || option=='N')
			{
				dust_tau_v=1;
				dust_mu=0.3;			
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
void cb_data::set_dust(double tau_v_value, double mu_value)
{
	dust_tau_v=tau_v_value;
	dust_mu=mu_value;
	dust_modif();
	return;
}

///
//modifies rawdata, due to dust
//
void cb_data::dust_modif()
{
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
			rawdata[offset+j+i*(nspecsteps+nfillingdata+2)]*= exp(dust_tau*exponent);
		}
	}
	return;
} 


//gets the ages of galaxies
//
//either from a file or from stdin
//
int cb_data::usr_get_age()
{
	bool tryagain;	
	std::cout<<"\nGetting ages to convolve to"<<std::endl;
	do{
		tryagain=false;
		try
		{
			//get option
			std::string temp_line;
			std::cout<<"input: from file ( type '1') or standard input ( type '2' ) ): ";
			getline(std::cin,temp_line);
			std::stringstream temp_sstr;
			temp_sstr<<temp_line;
			int option=-1;
			temp_sstr>>option;
			
			//read from file
			if (option==1)
			{
				// get filename
				std::cout<<"input filename: ";
				std::string temp_line;
				getline(std::cin,temp_line);
				std::stringstream temp_sstr;
				temp_sstr<<temp_line;
				std::string infilename;
				temp_sstr>>infilename;
				std::ifstream infile(infilename.c_str());
			
				// check filename
				if(!(infile))										
				{
					std::cout<<"\nERROR CAN'T OPEN FILE: "<<infilename<<"\n"<<std::endl;
					throw 1;
				}
				
				// read ages
				std::cout<<"reading file: "<<infilename<<std::endl;
				std::string str;
				double temp;
				while(infile>>str)
				{
					std::stringstream s;
					s<<str;
					if(s>>temp)
					{
						ages.push_back(temp*1e+9);
		        		}
				}
				infile.close();
			}
			//read from stdin
			else if(option==2)
			{
				//get the line of ages
				std::cout<<"write the ages to convolve to (separated with spaces, in Gyrs): ";
				std::string temp_line;
				getline(std::cin,temp_line);
				std::stringstream sstr;
				sstr<<temp_line;
				std::string token;
				double temp_d;
		
				//put the ages to the 'ages' vector
				while(sstr>>token)
				{
					std::stringstream str;
					str<<token;
					if (str>>temp_d)
					{
						ages.push_back(temp_d*1e+9);
					}
				}
			}
			//wrong option typed
			else
			{
				std::cout<<"\nERROR: WRONG OPTION!\n"<<std::endl;
				throw 1;
			}
			//check no of ages
			if(ages.size()==0)
			{
				std::cout<<"\nERROR: 0 AGES READ\n"<<std::endl;
				throw 1;
			}
		}
		catch (int ex)
		{
			if (ex==1){tryagain=true;}
		}
	}while(tryagain);
	
	n_ages=ages.size();

	//some info out
	std::cout<<ages.size()<<" ages read"<<std::endl;

	return 0;
}

void cb_data::set_age(std::vector<double> age_values)
{
	for(int i=0;i<age_values.size();i++)
		ages.push_back(age_values[i] * 1e+9);
	
	n_ages=age_values.size();
	return;
}


//gets sfr parameters
//
//no error handling yet
//
int cb_data::usr_get_sfr()
{
	std::cout<<"\nStar formation rate"<<std::endl;

	bool tryagain;
	do
	{
		try
		{
			tryagain=false;

			std::cout<<"Give the Tau of the exponential star formation rate, in Gy: ";
			std::string temp_line;
			getline(std::cin,temp_line);
			std::stringstream temp_sstr;
			temp_sstr<<temp_line;
			double temp_d=0;
			temp_sstr>>temp_d;

			if(temp_d==0)
			{
				throw 1;
			}
		
			tau=temp_d*1e+9;
		}catch(int e)
		{
		if (e==1) tryagain=true;
		}

	}while(tryagain);

	return 0;
}

void cb_data::set_sfr(double tau_value)
{
	tau=tau_value*1e+9;
}

//returns star formation rate
//
double cb_data::sfr(double time, double tau)
{
 	return exp(-time/tau);
}


//The c++ version of the convolution:
//
int cb_data::conv_to_age_vector()
{
	//setting the size of the table
	//but not resizing its container, because we use
	//push back in this function
	conv_result.rows=nspecsteps;
	conv_result.columns=n_ages; 

	clock_t t1, t2;
	t1 = clock();

	for(int k=0;k<n_ages;k++)
	{
		for(int i=0;i<nspecsteps;i++)
		{
			double temp=0;		
			for(int j=1; (j < ntimesteps) && (time[j]<=ages[k]) ;j++) 
			{
				//the actual integrating
				temp+=(time[j]-time[j-1])*rawdata[offset+i+(j-1)*(nspecsteps+nfillingdata+2)]*sfr(ages[k]-time[j],tau);
			}
			//tau is a contant so we only divide at the end
			conv_result.container.push_back(temp/tau);
		}
	}

	t2 = clock();   
	double diff = (((double)t2 - (double)t1)/CLOCKS_PER_SEC);  

	//time info out 
	std::cout<<"\nconvolution took "<< diff <<" second(s)."<< std::endl;

	return 0;
}

///
//writes the result in a table format
//from stdin input filename
//
//separated with whitespaces
//no " " at the end of file
//no /n at theend of file
//
int cb_data::usr_write_convresult()
{
	bool tryagain;  // indicator for loop
	int error;		// indicating failure or succes

	std::cout<<"\nWriting convolution results "<<std::endl;
	do
	{
		//setting indicator
		tryagain=false;
		error=0;
		
		//getting output filename
		std::cout<<"output filename: ";
		std::string temp_line;
		getline(std::cin,temp_line);
		std::stringstream temp_sstr;
		temp_sstr<<temp_line;
		std::string outfilename;
		temp_sstr>>outfilename;
	
		error=write_convresult(outfilename);
		if(error!=0)	
			tryagain=true;

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
int cb_data::write_convresult(std::string outfilename)
{
	std::ofstream outfile(outfilename.c_str());
	//Checking filename
	if(!(outfile))
	{
		std::cout<<"ERROR INVALID OUTPUT FILE: "<<outfilename<<std::endl;
		return 1;
	}

	//writing numbers 
	for(int j=0; j<(nspecsteps-1); j++)
	{
		outfile<<wavelengths[j]<<" ";
		for(int k=0;k<(n_ages-1);k++)
		{
			 outfile<<conv_result(j,k)<<" ";
		}
		//no " " at the end of lines
		outfile<<conv_result(j,n_ages-1)<<"\n";
	}
	outfile<<wavelengths[nspecsteps-1]<<" ";
	for(int k=0;k<(n_ages-1);k++)
	{
		outfile<<conv_result(nspecsteps-1,k)<<" ";
	}
	
	//no "\n" at the end of the file
	outfile<<conv_result(nspecsteps-1,n_ages-1);
	outfile.close();

	//info out
	std::cout<<"writing succesful: "<<n_ages<<" spectra written";

	return 0;

}
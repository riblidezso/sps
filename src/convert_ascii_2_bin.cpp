#include "convert_ascii_2_bin.h"

///
//Reading model from an ASCII file
//
int convert_ascii_2_bin::usr_read_buff_atof()
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
int convert_ascii_2_bin::read_buff_atof(std::string infilename)
{
	#define NUM_MAX_LEN 128 	// the size of the buffer of numbers
	#define BUFFERSIZE 1024		// the size of chunks read in binary

	//if its been used
	rawdata.resize(0);

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
	std::cout<<"\nreading in file: '"<<infilename<<"' ..."<<std::endl;

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
	//std::cout<<rawdata.size()<<" numbers read"<<"   ,";
	std::cout<< "it took "<< diff <<" second(s)"<< std::endl;

	return 0;
}


///
//gets some important data from model
//
//gets: time, wavelengths, ntimesteps, nspecsteps, offset
int convert_ascii_2_bin::get_data_from_raw(std::string imf)
{
	//there are some data in the model we do not use yet
	//and s no. is different in different models
	#define NO_TO_SKIP_SALP 19 
	#define NO_TO_SKIP_CHAB 13 
	
	int no_toskip;
	if(imf=="salp")
		no_toskip=NO_TO_SKIP_SALP;
	else if(imf=="chab")
		no_toskip=NO_TO_SKIP_CHAB;


	int ntimesteps;		//the no. of timesteps
	int nspecsteps;		//the no. of wavelengths
	int begin_specsteps;//the beginning of specsteps
	int offset;			//the beginning of model data 
	int nfillingdata;	//no. of data not used in model


	// reading in the timesteps
	ntimesteps=rawdata[0];
	time.resize(ntimesteps);
	for(int i=0;i<ntimesteps;i++)
	{
		time[i]=rawdata[i+1];
	}

	// reading the wavelengths
	nspecsteps=rawdata[ntimesteps+1+no_toskip];
	begin_specsteps= ntimesteps+1+no_toskip+1;
	wavelengths.resize(nspecsteps);
	for(int i=0;i<nspecsteps;i++)
	{
		wavelengths[i]=rawdata[begin_specsteps+i];
	}

	// the beginning of model data
	offset=begin_specsteps+nspecsteps+1;

	// there are some data between the spectra, but the number of them is given in the file.
	nfillingdata=rawdata[offset+nspecsteps];

	//creating the model only table
	model.resize(time.size());
	for(int i=0;i<ntimesteps;i++)
	{
		model[i].resize(wavelengths.size());
		for(int j=0;j< wavelengths.size();j++)
		{
			model[i][j]=rawdata[offset+j+i*(nspecsteps+nfillingdata+2)];
		}
	}
													
	return 0;
} 

///
//writes out vectors
//
int convert_ascii_2_bin::write_vector_bin(std::vector<double>& outvec, std::string outfilename)
{
	std::ofstream outfile(outfilename.c_str(), std::ios::binary);
	//	CHECKING FILENAME	
	if(!(outfile))
	{
		std::cout<<"ERROR CAN'T OPEN FILE: "<<outfilename<<std::endl;
		return 1;
	}

	std::cout<<"writing in file: "<<outfilename<<" ... ";
	outfile.write( reinterpret_cast<char*>(&outvec[0]),sizeof(double)*outvec.size());
	outfile.close();

	std::cout<<"writing succesful "<<std::endl;
	return 0;
}

///
//writes out vectors of vectors
//
int convert_ascii_2_bin::write_vec_vec_bin(std::vector<std::vector<double> >& outvecvec , std::string outfilename)
{
	std::ofstream outfile(outfilename.c_str(), std::ios::binary);
	//	CHECKING FILENAME	
	if(!(outfile))
	{
		std::cout<<"ERROR CAN'T OPEN FILE: "<<outfilename<<std::endl;
		return 1;
	}

	std::cout<<"writing in file: "<<outfilename<<" ... ";
	for(int i=0;i<outvecvec.size();i++ )
	{
		outfile.write( reinterpret_cast<char*>(outvecvec[0].data()),sizeof(double) * outvecvec[0].size() );
	}
	outfile.close();

	std::cout<<"writing succesful "<<std::endl;
	return 0;

}
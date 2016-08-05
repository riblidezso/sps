#ifndef CONVERT_ASCII_2_BIN_H
#define CONVERT_ASCII_2_BIN_H


#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <ctime>
#include <stdio.h>
#include <stdlib.h>

class convert_ascii_2_bin{
public: //functions

/*interactive functions*/
	//Reading model from an ASCII file
	int usr_read_buff_atof();

/*functions*/
	//Reading model from an ASCII file 
	int read_buff_atof(std::string infilename);
	//gets some important data from model
	int get_data_from_raw(std::string imf);
	//write vectors
	int write_vector_bin(std::vector<double>& outvec, std::string outfilename);
	//writes vec of vec
	int write_vec_vec_bin(std::vector<std::vector<double> >& outvecvec, std::string outfilename);

public: //data

//data read in
	//stores all numbers of the data ascii file
	std::vector<double> rawdata;

//data reconstructed
	//stores the ages where model data is available
	std::vector<double> time;
	//stores the wavelengths of the model
	std::vector<double> wavelengths;
	//stores the actual model data
	std::vector< std::vector< double> > model;
};

#endif

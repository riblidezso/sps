#include "sps_write.h"
#include <fstream>
#include <iostream>
#include <string>

int write_table_row(std::vector< std::vector<double > >& table_to_write, std::string outfilename)
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
	std::cout<<"writing succesful: "<<outfilename<<std::endl;

	return 0;
}
int write_table_col(std::vector< std::vector<double > >& table_to_write, std::string outfilename)
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
	std::cout<<"writing succesful: "<<outfilename<<std::endl;

	return 0;
}

///
//writes a vector to a file
//
//one column
int write_vector(std::vector<double>& vec_to_write, std::string outfilename)
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
	std::cout<<"writing succesful: "<<outfilename<<std::endl;

	return 0;
}

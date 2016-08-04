#include <vector>
#include <string>
#include <map>
#include <fstream>
#include <iostream>

#ifndef SPS_WRITE_H
#define SPS_WRITE_H


int write_vector(std::vector<double>& vec_to_write, std::string outfilename);
int write_map(std::map<std::string,std::vector<double> >& map_to_write, std::string outfilename);

int write_table_row(std::vector< std::vector<double > >& table_to_write, std::string outfilename);
int write_table_col(std::vector< std::vector<double > >& table_to_write, std::string outfilename);

#endif

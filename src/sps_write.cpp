#include "sps_write.h"

/*
 write a vector to a file
 */
int write_vector(std::vector<double>& vec_to_write, std::string outfilename){
    //open file
    std::ofstream outfile(outfilename.c_str());
    if(!outfile.is_open()){
        std::cerr<<"ERROR COULDN'T OPEN FILE: "<<outfilename<<std::endl;
        exit(1);
    }
    
    //write numbers
    for(size_t j=0; j<vec_to_write.size(); j++){
        outfile<<vec_to_write[j]<<"\n";
    }
    
    outfile.close();
    return 0;
}


/*
 write a map to a csv file
 */
int write_map(std::map<std::string,std::vector<double> >& map_to_write, std::string outfilename){
    //open file
    std::ofstream outfile(outfilename.c_str());
    if(!outfile.is_open()){
        std::cerr<<"ERROR COULDN'T OPEN FILE: "<<outfilename<<std::endl;
        exit(1);
    }
    
    //write header
    outfile<<"#";
    for(auto element : map_to_write){
        outfile<<element.first<<"\t";
    }
    
    //write numbers
    for(auto element : map_to_write){
        outfile<<"\n";
        for (double data : element.second){
            outfile<<data<<"\t";
        }
    }
    
    outfile.close();
    return 0;
}

/*
 write vector of vector to file row wise
 */
int write_table_row(std::vector< std::vector<double > >& table_to_write, std::string outfilename){
    //open file
    std::ofstream outfile(outfilename.c_str());
    if(!outfile.is_open()){
        std::cerr<<"ERROR COULDN'T OPEN FILE: "<<outfilename<<std::endl;
        exit(1);
    }

    //write data
    size_t k,j;
    for(j=0; j<table_to_write.size()-1; j++){
        for(k=0;k<table_to_write[j].size()-1;k++){
            outfile<<table_to_write[j][k]<<"\t";
        }
        //end of lines
        outfile<<table_to_write[j][k]<<"\n";
    }
    
    //last line
    for(k=0;k<table_to_write[j].size()-1;k++){
        outfile<<table_to_write[j][k]<<"\t";
    }
    outfile<<table_to_write[j][k];
    
	outfile.close();

	return 0;
}

/*
 write vector of vector to file columnn wise
 */
int write_table_col(std::vector< std::vector<double > >& table_to_write, std::string outfilename){
    //open file
    std::ofstream outfile(outfilename.c_str());
    if(!outfile.is_open()){
        std::cerr<<"ERROR COULDN'T OPEN FILE: "<<outfilename<<std::endl;
        exit(1);
    }

    //write data
    size_t k,j;
    for(j=0; j<table_to_write[0].size()-1; j++){
        for(k=0;k<table_to_write.size()-1;k++){
            outfile<<table_to_write[k][j]<<"\t";
        }
        //end of lines
        outfile<<table_to_write[k][j]<<"\n";
    }
    
    //last line
    for(k=0;k<table_to_write.size()-1;k++){
        outfile<<table_to_write[k][j]<<"\t";
    }
    outfile<<table_to_write[k][j];
    
    outfile.close();
    return 0;
}

#include "sps_write.h"

/*
 writes a vector to a file
 */
int write_vector(std::vector<double>& vec_to_write, std::string outfilename){
    std::ofstream outfile(outfilename.c_str());
    //Checking filename
    if(!(outfile)){
        std::cout<<"ERROR INVALID OUTPUT FILE: "<<outfilename<<std::endl;
        return 1;
    }
    
    //writing numbers
    for(size_t j=0; j<vec_to_write.size(); j++){
        outfile<<vec_to_write[j]<<"\n";
    }
    outfile.close();
    
    return 0;
}



/*
 writes a map to a csv file
 */
int write_map(std::map<std::string,std::vector<double> >& map_to_write, std::string outfilename){
    std::ofstream outfile(outfilename.c_str());
    //Checking filename
    if(!(outfile)){
        std::cerr<<"ERROR INVALID OUTPUT FILE: "<<outfilename<<std::endl;
        exit(1);
    }
    
    //writing header
    outfile<<"#";
    for(auto element : map_to_write){
        outfile<<element.first<<"\t";
    }
    
    //writing numbers
    for(auto element : map_to_write){
        outfile<<"\n";
        for (double data : element.second){
            outfile<<data<<"\t";
        }
    }
    outfile.close();
    
    return 0;
}


int write_table_row(std::vector< std::vector<double > >& table_to_write, std::string outfilename){
	std::ofstream outfile(outfilename.c_str());
	//Checking filename
	if(!(outfile)){
		std::cout<<"ERROR INVALID OUTPUT FILE: "<<outfilename<<std::endl;
		return 1;
	}

	for(size_t j=0; j<table_to_write.size(); j++){
		for(size_t k=0;k<table_to_write[j].size();k++){
			outfile<<table_to_write[j][k]<<" ";
		}
		//end of lines
		outfile<<"\n";
	}
	outfile.close();

	return 0;
}


int write_table_col(std::vector< std::vector<double > >& table_to_write, std::string outfilename){
	std::ofstream outfile(outfilename.c_str());
	//Checking filename
	if(!(outfile)){
		std::cout<<"ERROR INVALID OUTPUT FILE: "<<outfilename<<std::endl;
		return 1;
	}

	for(size_t j=0; j<table_to_write[0].size(); j++){
		for(size_t k=0;k<table_to_write.size();k++){
			outfile<<table_to_write[k][j]<<" ";
		}
		//end of lines
		outfile<<"\n";
	}
	outfile.close();
    
    return 0;
}

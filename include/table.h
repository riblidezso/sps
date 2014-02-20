#ifndef TABLE_H
#define TABLE_H

#include <iostream>
#include <vector>

// Its a simple table with rows and columns
// This will contain the results of a convolution
// its columns will be different spectra with different parameters


struct table{
public:
	size_t columns, rows;
	std::vector<double> container;

	double& operator()(size_t i, size_t j);
	double operator()(size_t i, size_t j) const;
};

#endif

#include<vector>
#include<algorithm>
#include<iostream>
#include<cmath>

#pragma once
class DataSet
{
	int number_of_features, number_of_samples;
	std::vector < std::vector<double>> dataset;
	std::vector<double> minimums, maximums, means, variance;
public:
	DataSet();
	void load_from_memory();
	void load_from_file();

	void generate_meta_data();
	void debug_samples();
	void normalize_samples();

	void display_samples();

	size_t get_number_of_features();
	size_t get_number_of_samples();
};


#include "DataSet.h"

DataSet::DataSet()
{
	load_from_memory();
}

void DataSet::load_from_memory()
{
	dataset = {
	{7.5, 0.263, 0.84, 403.768, 46.33, 793.52, 1.03, 3.82, 26.95, 19.19, 10.35 },
	{7.5, 0.286, 0.81, 389.407, 23.46, 778.4, 1.35, 2.75, 14.72, 16.77, 8.28},
	{7.2, 0.268, 0.75, 360.685, 9.9, 554.064, 0.6, 3.11, 15.32, 13.27, 16.56},
	{7.5, 0.138, 0.33, 159.631, 6.73, 214.928, 0.28, 1.76, 12.7, 10.85, 13.11},
	{7.3, 0.17, 0.495, 238.617, 16.23, 135.073, 0.6, 1.4, 10.64, 10.95, 15.18 },
	{7.6, 0.268, 0.42, 202.714, 2.37, 341.6, 0.67, 3.18, 22.13, 18.46, 8.28},
	{7.6, 0.152, 0.48, 231.436, 9.5, 407.344, 0.32, 1.41, 17.91, 8.98, 20.01},
	{7.4, 0.199, 0.855, 410.949, 5.54, 165.536, 0.6, 5, 24.49, 26.39, 6.21},
	{7.1, 0.226, 0.66, 317.602, 14.65, 334.096, 0.71, 2.57, 16.62, 19.18, 17.25},
	{7, 0.09, 0.45, 217.075, 7.12, 562.24, 0.57, 2.59, 18.38, 12.44, 5.52},
	{6.9, 0.22, 0.75, 360.685, 26.93, 323.008, 0.14, 1.66, 10.31, 9.41, 14.49},
	{8.1, 0.21, 0.42, 202.714, 28.91, 413.28, 0.22, 0.9, 4.94, 4.4, 21.39},
	{7.1, 0.178, 0.38, 183.566, 30.9, 291.2, 0.18, 1.58, 14.49, 14.05, 16.56},
	{7.3, 0.166, 0.4, 193.14, 24.4, 240.128, 0.3, 2.19, 3.88, 4.74, 12.42},
	{7.8, 0.171, 0.27, 130.909, 38.9, 288.736, 0.13, 0.78, 5.62, 3.24, 8.28},
	{7.6, 0.174, 0.36, 173.992, 49.5, 302.96, 0.36, 1.59, 10.58, 8.13, 11.73},
	{8, 0.175, 0.435, 209.894, 4.75, 138.544, 0.28, 1.25, 4.4, 4.12, 8.97},
	{8, 0.25, 0.33, 159.631, 24.94, 296.24, 0.44, 1.47, 7.96, 5.83, 5.52},
	{8, 0.152, 0.345, 166.811, 5.54, 178.416, 0.35, 1.52, 6.63, 6.75, 2.76},
	{7.9, 0.158, 0.39, 188.353, 11.1, 332.304, 0.09, 2.71, 7.51, 9.43, 11.73}
	};

	number_of_features = dataset[0].size();
	number_of_samples = dataset.size();
}

void DataSet::load_from_file()
{
	load_from_memory();
}

void DataSet::generate_meta_data()
{
	minimums.resize(number_of_features, 1e9);
	maximums.resize(number_of_features, -1e9);
	means.resize(number_of_features, 0);
	variance.resize(number_of_features, 0);

	for (auto data : dataset) {
		int feature_index{};
		for (auto feature : data) {
			minimums[feature_index] = std::min(minimums[feature_index], feature);
			maximums[feature_index] = std::max(maximums[feature_index], feature);
			means[feature_index] += feature;
			feature_index++;
		}
	}
	for (auto& it : means)	it /= number_of_samples;

	for (auto data : dataset) {
		int feature_index{};
		for (auto feature : data) {
			variance[feature_index] += std::pow((feature - means[feature_index]), 2);
			feature_index++;
		}
	}
}

void DataSet::debug_samples()
{
	for (int i = 0; i < number_of_features; ++i) {
		std::cerr << minimums[i] << '\t' << maximums[i] << '\t' << means[i] << std::endl;
	}
}

void DataSet::normalize_samples()
{
	for (auto& data : dataset) {
		int feature_index{ };
		for (auto& feature : data) {
			feature = (feature - means[feature_index]) / (std::sqrt(variance[feature_index]));
			feature_index++;
		}
	}
}

void DataSet::display_samples()
{
	for (auto data : dataset) {
		for (auto feature : data)	std::cerr << feature << ' ';
		std::cerr << std::endl;
	}
}

size_t DataSet::get_number_of_features()
{
	return number_of_features;
}

size_t DataSet::get_number_of_samples()
{
	return number_of_samples;
}

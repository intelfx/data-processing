#include "util.h"

static const size_t MAX_SIGMA = 2;

int main()
{
	std::cin.exceptions (std::istream::badbit | std::istream::failbit);

	auto data = read_into_vector<double> (std::cin);
	double average = avg (data);
	double squared_difference_sum = std::accumulate (data.begin(), data.end(), (double) 0,
	                                                 [average] (double acc, double value) { return acc + (value - average) * (value - average); });

	double stddev = sqrt (squared_difference_sum / data.size());
	double stderror = sqrt (squared_difference_sum) / data.size();

	std::cout << "average: " << average << std::endl;
	std::cout << "standard deviation of the sample: " << stddev << std::endl;
	std::cout << "standard error of the average: " << stderror << std::endl;

	size_t measurements_in_range[MAX_SIGMA] = { };

	for (size_t sigma = 1; sigma <= MAX_SIGMA; ++sigma) {
		measurements_in_range[sigma - 1] = std::count_if (data.begin(), data.end(), [average, stddev, sigma] (double value) { return fabs (value - average) <= stddev * sigma; });
	}

	for (size_t sigma = 1; sigma <= MAX_SIGMA; ++sigma) {
		size_t count = measurements_in_range[sigma - 1];
		std::cout << "error = " << sigma << " sigma (" << stddev * sigma << "): " << count << " measurements (" << (double) 100 * count / data.size() << "%)" << std::endl;
	}
}
#include"DataSet.h"
#include"thread_safe_queue.h"
#include<atomic>
#include<algorithm>
#include<mutex>
#include<random>
#include<array>
#include<numeric>
#include<utility>
#include<unordered_set>

const int MAX_POPULATION = 1e4;
const int MAX_GENERATION = 1e2;
const int MAX_CLUSTER_HEADS = 3;
const int MAX_FEATURES = 11;
const int MAX_THREADS = std::thread::hardware_concurrency();

std::mutex cout_mu;

auto double_rng() {
	static std::uniform_real_distribution<double> dis(-1.0, 1.0);
	static std::random_device device;
	static std::mt19937 engine{ device() };
	return dis(engine);
}

auto int_rng() {
	static std::uniform_int_distribution<int> dis(0, MAX_POPULATION-1);
	static std::random_device device;
	static std::mt19937 engine{ device() };
	return dis(engine);
}

auto calculate_fitness_value(const std::array<std::vector<double>, MAX_CLUSTER_HEADS>& cluster_heads) {

	//Calculating the mean of cluster heads
	std::vector<double> cluster_mean(MAX_FEATURES, 0);
	for (const auto& cluster_head : cluster_heads) std::transform(std::begin(cluster_head), std::end(cluster_head), std::begin(cluster_mean), std::begin(cluster_mean), std::plus<>{});
	for (auto& it : cluster_mean) it /= MAX_CLUSTER_HEADS;

	//Calculating the variance of cluster heads from the mean
	auto fitness_value = (double)(0);
	for (const auto& cluster_head : cluster_heads) {
		std::vector<double> cluster_distance_squared_from_mean_cluster_head(MAX_FEATURES,0);
		std::transform(std::begin(cluster_head), std::end(cluster_head), std::begin(cluster_mean), std::begin(cluster_distance_squared_from_mean_cluster_head), [](const auto& a, const auto& b)->double {
			return (a - b) * (a - b);
		});
		fitness_value += std::accumulate(std::begin(cluster_distance_squared_from_mean_cluster_head), std::end(cluster_distance_squared_from_mean_cluster_head), 0);
	}
	return fitness_value;
}

int main() {
	DataSet dataset;	dataset.generate_meta_data();
	//dataset.debug_samples();
	//dataset.display_samples();
	dataset.normalize_samples();
	//dataset.display_samples();

	//Create a Population
	std::vector<std::array<std::vector<double>, MAX_CLUSTER_HEADS>>
		current_population(MAX_POPULATION, std::array < std::vector<double>, MAX_CLUSTER_HEADS >{std::vector<double>(dataset.get_number_of_features())}),
		next_population(MAX_POPULATION, std::array < std::vector<double>, MAX_CLUSTER_HEADS >{std::vector<double>(dataset.get_number_of_features())});

	//Filling the Population
	//for (auto& soil : current_population)	std::ranges::generate(soil.begin(), soil.end(), rng);
	for (auto& soil : current_population) for (auto& cluster_head : soil) std::ranges:: generate(std::begin(cluster_head), std::end(cluster_head), double_rng);

	//Algorithm Starts here
	auto crossover_probability = double_rng() * 0.5 + 0.5;		//Range of Crossover Probability = [0,1]
	auto differential_weight = double_rng() + 1.0;				//Range of Differential Wight = [0,2]

	auto program_start = std::chrono::high_resolution_clock::now();

	std::atomic<int> population_indexer = 0, generation = MAX_GENERATION;
	std::thread task_master{ [&]() {
		while (true) {
			if (population_indexer.load(std::memory_order_acquire) >= MAX_POPULATION) {
				if (generation.fetch_sub(1, std::memory_order_acq_rel) == 1)
					break;
				//Modify previous from current using std::move
				current_population.swap(next_population);
				//initialize constants CR and F for this generation
				crossover_probability = double_rng() * 0.5 + 0.5;
				differential_weight = double_rng() + 1.0;
				population_indexer.store(0, std::memory_order_release);
			}
			//Considerable Performance Weight of yield();
			//std::this_thread::yield();
		}
		cout_mu.lock();
		std::cerr << "Thread: " << std::this_thread::get_id() << " retiring from Task Master\n";
		cout_mu.unlock(); 
	}};
	task_master.detach();

	auto slave_function = [&]() {
		auto start_time = std::chrono::high_resolution_clock::now();
		while (true) {
			if (generation.load(std::memory_order_acquire)) {
				while (true) {
					int index;
					if ((index = population_indexer.fetch_add(1, std::memory_order_acq_rel)) < MAX_POPULATION) {

						std::unordered_set<int> bc;
						while(bc.size()<2){
							auto x = int_rng();
							if (x == index)	continue;
							bc.insert(x);
						}
						auto initial_fitness_value_of_cluster_head_index = calculate_fitness_value(current_population[index]);
						auto r = double_rng() * 0.5 + 0.5;
						auto b = *bc.begin(); bc.erase(bc.begin());
						auto c = *bc.begin(); bc.erase(bc.begin());

						if (r <= crossover_probability) {
							std::array<std::vector<double>, MAX_CLUSTER_HEADS> next_gen{ std::vector<double>(MAX_FEATURES,0) };

							for (auto cluster_index = 0; cluster_index < MAX_CLUSTER_HEADS; ++cluster_index) {
								std::transform(std::begin(current_population[b][cluster_index]),
									std::end(current_population[b][cluster_index]),
									std::begin(current_population[c][cluster_index]),
									std::begin(next_gen[cluster_index]),
									[&differential_weight](const auto& val1, const auto& val2)->double {
										return differential_weight * (val1 - val2);
									});

								std::transform(std::begin(current_population[index][cluster_index]),
									std::end(current_population[index][cluster_index]),
									std::begin(next_gen[cluster_index]),
									std::begin(next_gen[cluster_index]),
									std::plus<>{});
							}

							if ((calculate_fitness_value(next_gen) - initial_fitness_value_of_cluster_head_index) >= 1e-6) next_population[index] = next_gen;
						}
						else {
							next_population[index] = current_population[index];
						}
					
					}
					else {
						break;
					}
				}
			}
			else {
				break;
			}
		}

		cout_mu.lock();
		std::cerr << "Thread: " << std::this_thread::get_id() << " retiring from Slavery, it was up for: " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start_time) << std::endl;
		cout_mu.unlock();
	};

	std::vector<std::thread> slaves;
	for (int i = 0; i < MAX_THREADS; ++i)	slaves.push_back(std::thread{slave_function});
	for (auto& it : slaves)	it.join();

	auto best_candidate = std::max_element(std::begin(next_population), std::end(next_population), [](const auto& pop1, const auto& pop2)->auto{
		return calculate_fitness_value(pop1) < calculate_fitness_value(pop2); });

	auto worst_candidate = std::min_element(std::begin(next_population), std::end(next_population), [](const auto& pop1, const auto& pop2)->auto{
		return calculate_fitness_value(pop1) < calculate_fitness_value(pop2); });

	std::cout << "BEST CANDIDATE found at index: " << best_candidate - std::begin(next_population) << '\n';
	std::cout << "WORST CANDIDATE found at index: " << worst_candidate - std::begin(next_population) << '\n';


	auto program_end = std::chrono::high_resolution_clock::now();
	std::cerr <<"Total ms Program ran for: "<< std::chrono::duration_cast<std::chrono::milliseconds>(program_end - program_start);
	return 0;
}
#include "Map.hpp"

#include <mutex>
#include <filesystem>
#include <limits.h>

/** Default constructor.*/
Map::Map() {}

/** Constructor with files.
*
* @param map_file_name file containing map.
* @param agents_file_name file containing agents.
*/
Map::Map(std::string map_file_name, std::string agents_file_name) :
	map_file_name(map_file_name),
	agents_file_name(agents_file_name)
{}

/** Setting file name containig map.
*
* @param new_map_file_name file containing map.
*/
void Map::set_map_file(std::string new_map_file_name) {
	map_file_name = new_map_file_name;
}

/** Setting file name containig agents.
*
* @param new_agents_file_name file containing agents.
*/
void Map::set_agents_file(std::string new_agents_file_name) {
	agents_file_name = new_agents_file_name;
}

/** Loading map from file.
*
* @param custom_map_file_name custom file containing map, optional.
* @return error message, "OK" if everything ended right.
*/
std::string Map::load_map(std::string custom_map_file_name) {

	std::ifstream map_file;
	if (custom_map_file_name != "") {
		map_file = std::ifstream(custom_map_file_name);
	}
	else {
		map_file = std::ifstream(map_file_name);
	}

	size_t vertex_number = 0;

	if (map_file.is_open()) {
		std::string map_line;

		std::getline(map_file, map_line); //first line is unnecessary

		std::getline(map_file, map_line); //second line
		height = atoi(&map_line[height_number_index]);

		std::getline(map_file, map_line); //third line
		width = atoi(&map_line[width_number_index]);

		std::getline(map_file, map_line); //fourth line is unnecessary

		map = std::vector<std::vector<size_t>>(height + 2);
		map[0].resize(width + 2, 0); //top bound
		map[height + 1].resize(width + 2, 0); //bot bound	

		vertex_number++;

		for (size_t i = 1; i <= height + 1; i++) { //reading input
			if (i != height + 1) {
				std::getline(map_file, map_line);
				map[i].resize(width + 2, 0);
			}
			for (size_t j = 0; j < width; j++) {
				if (map_line[j] == '.' && i != height + 1) {
					map[i][j + 1] = vertex_number;
					vertex_number++;
				}
			}
		}
	}
	else {
		if (custom_map_file_name != "") {
			return "ERROR: cannot open file for reading: " + custom_map_file_name + "\n";
		}
		else {
			return "ERROR: cannot open file for reading: " + map_file_name + "\n";
		}
	}

	original_number_of_vertices = vertex_number - 1;

	map_file.close();

	return "OK";
}

/** Loading agents from file.
*
* @param number_of_agents max number of agents to be loaded, optional.
* @param custom_agents_file_name custom file containing agents, optional.
* @return error message, "OK" if everything ended right.
*/
std::string Map::load_agents(int number_of_agents, std::string custom_agents_file_name) {

	agents.clear();

	std::ifstream agents_file;
	if (custom_agents_file_name != "") {
		agents_file = std::ifstream(custom_agents_file_name);
	}
	else {
		agents_file = std::ifstream(agents_file_name);
	}

	if (number_of_agents == -1) {
		number_of_agents = INT_MAX;
	}

	if (agents_file.is_open()) {
		std::string agents_line;
		std::getline(agents_file, agents_line); //first line unimportant

		int agents_loaded = 0;

		while (agents_loaded < number_of_agents && std::getline(agents_file, agents_line)) {

			std::stringstream tokenizer(agents_line);
			std::string column;
			size_t asx, asy, aex, aey;
			asx = asy = aex = aey = 0;
			tokenizer >> column;
			for (size_t i = 0; i < 9; i++, tokenizer >> column) {
				switch (i) {
				case 4:
					asx = std::stoi(column);
					break;
				case 5:
					asy = std::stoi(column);
					break;
				case 6:
					aex = std::stoi(column);
					break;
				case 7:
					aey = std::stoi(column);
					break;
				default:
					break;
				}
			}

			agents.push_back(std::make_pair(std::make_pair(asy + 1, asx + 1), std::make_pair(aey + 1, aex + 1)));
			agents_loaded++;
		}
	}
	else {
		if (custom_agents_file_name != "") {
			return "ERROR: cannot open file for reading: " + custom_agents_file_name + "\n";
		}
		else {
			return "ERROR: cannot open file for reading: " + agents_file_name + "\n";
		}
	}

	agents_file.close();

	agents_shortest_paths.clear();
	time_expanded_graph.first.clear();
	time_expanded_graph.second.clear();

	agents_shortest_paths.resize(agents.size());
	time_expanded_graph.first.resize(agents.size());
	time_expanded_graph.second.resize(agents.size());

	return "OK";
}

/** Loading map and agents from files.
*
* @param number_of_agents max number of agents to be loaded, optional.
* @return error message, "OK" if everything ended right.
*/
std::string Map::reload(int number_of_agents) {

	auto ret_load_map = load_map();
	auto ret_load_agents = load_agents(number_of_agents);

	std::string ret = "";
	if (ret_load_map != "OK") {
		ret += ret_load_map;
	}
	if (ret_load_agents != "OK") {
		ret += ret_load_agents;
	}

	if (ret == "") {
		return "OK";
	}
	else {
		return ret;
	}
}

std::string Map::kissat(std::string log_file, std::string alg, size_t lower_bound, size_t bonus_makespan, size_t time_limit_ms) {

	number_of_vertices = 0;
	std::vector<std::vector<int>> map = std::vector<std::vector<int>>(expanded_map.size(), std::vector<int>(expanded_map[0].size(), 0));
	for (size_t i = 0; i < expanded_map.size(); i++) {
		for (size_t j = 0; j < expanded_map[0].size(); j++) {
			if (expanded_map[i][j] == 0) {
				map[i][j] = -1;
			}
			else {
				map[i][j] = expanded_map[i][j];
				number_of_vertices++;
			}
		}
	}

	std::vector<std::pair<int, int>> starts;
	std::vector<std::pair<int, int>> goals;

	for (size_t a = 0; a < agents.size(); a++) {
		starts.push_back(std::make_pair<int, int>((int)agents[a].first.first, (int)agents[a].first.second));
		goals.push_back(std::make_pair<int, int>((int)agents[a].second.first, (int)agents[a].second.second));
	}

	Instance* inst = new Instance(map, starts, goals, "scenario", "map_name");
	Logger* log = new Logger(inst, "log.log", "encoding_name");
	Pass_parallel_mks_all* solver = new Pass_parallel_mks_all("encoding_name", 1);

	solver->SetData(inst, log, time_limit_ms/1000, true, false);
	inst->SetAgents(agents.size());
	log->NewInstance(agents.size());

	int res = solver->Solve(agents.size(), (int)bonus_makespan, false);
	
	delete solver;
	delete log;
	delete inst;

	std::string base_map_name = map_file_name.substr(map_file_name.find_last_of("/") + 1);

	std::string base_agents_name = agents_file_name.substr(agents_file_name.find_last_of("/") + 1);

	std::string out = base_map_name + "\t" + base_agents_name + "\t" + std::to_string(number_of_vertices) + "\t" + std::to_string(agents.size()) + "\t" + std::to_string(lower_bound + bonus_makespan) + "\t";
	std::ofstream ofile;
	ofile.open(log_file, std::ios::app);
	if (ofile.is_open()) {
		ofile << out;
	}

	std::mutex m;

	switch(res) {
		case -1:
			if (ofile.is_open()) {
				ofile << "NO solution" << std::endl;
			}
			m.lock();
			std::cout << alg + "\t" << out << "NO solution" << std::endl;
			std::cout.flush();
			m.unlock();
			ofile.close();
			return "NO solution";
		case 0:
			if (ofile.is_open()) {
				ofile << "OK" << std::endl;
			}
			m.lock();
			std::cout << alg + "\t" << out << "OK" << std::endl;
			std::cout.flush();
			m.unlock();
			ofile.close();
			return "OK";
		case 1:
			if (ofile.is_open()) {
				ofile << "Timed out" << std::endl;
			}
			m.lock();
			std::cout << alg + "\t" << out << "Timed out" << std::endl;
			std::cout.flush();
			m.unlock();
			ofile.close();
			return "Timed out";
		default:
			ofile.close();
			return "Timed out";
	}
}

/** Computes minimal required time for agents to complete its paths.
*
* Computes from Map::agents_shortest_paths.
*
* @return lenght of the logest from the shortest paths.
*/
size_t Map::get_min_time() {
	size_t max = 0;
	for (size_t a = 0; a < agents_shortest_paths.size(); a++) {
		if (agents_shortest_paths[a].size() > max) {
			max = agents_shortest_paths[a].size();
		}
	}
	return max;
}

/** Zeroes computed map and mirrors sizes of loaded map.*/
void Map::reset_computed_map() {

	expanded_map = std::vector<std::vector<size_t>>(map.size(), std::vector<size_t>(map[0].size(), 0));
}
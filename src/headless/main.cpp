#include <openvic-simulation/country/CountryInstance.hpp>
#include <openvic-simulation/dataloader/Dataloader.hpp>
#include <openvic-simulation/economy/GoodDefinition.hpp>
#include <openvic-simulation/economy/production/ProductionType.hpp>
#include <openvic-simulation/economy/production/ResourceGatheringOperation.hpp>
#include <openvic-simulation/GameManager.hpp>
#include <openvic-simulation/pop/Pop.hpp>
#include <openvic-simulation/testing/Testing.hpp>
#include <openvic-simulation/utility/Logger.hpp>

using namespace OpenVic;

static void print_help(std::ostream& stream, char const* program_name) {
	stream
		<< "Usage: " << program_name << " [-h] [-t] [-b <path>] [path]+\n"
		<< "    -h : Print this help message and exit the program.\n"
		<< "    -t : Run tests after loading defines.\n"
		<< "    -b : Use the following path as the base directory (instead of searching for one).\n"
		<< "    -s : Use the following path as a hint to search for a base directory.\n"
		<< "Any following paths are read as mod directories, with priority starting at one above the base directory.\n"
		<< "(Paths with spaces need to be enclosed in \"quotes\").\n";
}

static void print_rgo(ProvinceInstance const& province) {
	ResourceGatheringOperation const& rgo = province.get_rgo();
	ProductionType const* const production_type_nullable = rgo.get_production_type_nullable();
	if (production_type_nullable == nullptr) {
		Logger::error(
			"\n    ", province.get_identifier(),
			" - production_type: nullptr"
		);
	} else {
		ProductionType const& production_type = *production_type_nullable;
		GoodDefinition const& output_good = production_type.get_output_good();
		std::string text = StringUtils::append_string_views(
			"\n\t", province.get_identifier(),
			" - good: ", output_good.get_identifier(),
			", production_type: ", production_type.get_identifier(),
			", size_multiplier: ", rgo.get_size_multiplier().to_string(3),
			", output_quantity_yesterday: ", rgo.get_output_quantity_yesterday().to_string(3),
			", revenue_yesterday: ", rgo.get_revenue_yesterday().to_string(3),
			", total owner income: ", rgo.get_total_owner_income_cache().to_string(3),
			", total employee income: ", rgo.get_total_employee_income_cache().to_string(3),
			"\n\temployees:"
		);

		for (auto [pop_type, employees_of_type] : rgo.get_employee_count_per_type_cache()) {
			if (employees_of_type > 0) {
				text += StringUtils::append_string_views(
					"\n\t\t", std::to_string(employees_of_type), " ", pop_type.get_identifier()
				);
			}
		}
		Logger::info("", text);
	}
}

static bool run_headless(Dataloader::path_vector_t const& roots, bool run_tests) {
	bool ret = true;

	GameManager game_manager { []() {
		Logger::info("State updated");
	}, nullptr };

	Logger::info("===== Loading definitions... =====");
	ret &= game_manager.set_roots(roots);
	ret &= game_manager.load_definitions(
		[](std::string_view key, Dataloader::locale_t locale, std::string_view localisation) -> bool {
			return true;
		}
	);

	if (run_tests) {
		Testing testing { game_manager.get_definition_manager() };
		std::cout << std::endl << "Testing Loaded" << std::endl << std::endl;
		testing.execute_all_scripts();
		testing.report_results();
		std::cout << "Testing Executed" << std::endl << std::endl;
	}

	Logger::info("===== Setting up instance... =====");
	ret &= game_manager.setup_instance(
		game_manager.get_definition_manager().get_history_manager().get_bookmark_manager().get_bookmark_by_index(0)
	);

	Logger::info("===== Starting game session... =====");
	ret &= game_manager.start_game_session();

	// This triggers a gamestate update
	ret &= game_manager.update_clock();

	// TODO - REMOVE TEST CODE
	Logger::info("===== Ranking system test... =====");
	if (game_manager.get_instance_manager()) {
		const auto print_ranking_list = [](std::string_view title, std::vector<CountryInstance*> const& countries) -> void {
			std::string text;
			for (CountryInstance const* country : countries) {
				text += StringUtils::append_string_views(
					"\n    ", country->get_identifier(),
					" - Total #", std::to_string(country->get_total_rank()), " (", country->get_total_score().to_string(1),
					"), Prestige #", std::to_string(country->get_prestige_rank()), " (", country->get_prestige().to_string(1),
					"), Industry #", std::to_string(country->get_industrial_rank()), " (", country->get_industrial_power().to_string(1),
					"), Military #", std::to_string(country->get_military_rank()), " (", country->get_military_power().to_string(1), ")"
				);
			}
			Logger::info(title, ":", text);
		};

		CountryInstanceManager const& country_instance_manager =
			game_manager.get_instance_manager()->get_country_instance_manager();

		std::vector<CountryInstance*> const& great_powers = country_instance_manager.get_great_powers();
		print_ranking_list("Great Powers", great_powers);
		print_ranking_list("Secondary Powers", country_instance_manager.get_secondary_powers());
		print_ranking_list("All countries", country_instance_manager.get_total_ranking());

		Logger::info("===== RGO test... =====");
		for (size_t i = 0; i < std::min<size_t>(3, great_powers.size()); ++i) {
			CountryInstance const& great_power = *great_powers[i];
			ProvinceInstance const* const capital_province = great_power.get_capital();
			if (capital_province == nullptr) {
				Logger::warning(great_power.get_identifier(), " has no capital ProvinceInstance set.");
			} else {
				print_rgo(*capital_province);
			}
		}
	} else {
		Logger::error("Instance manager not available!");
		ret = false;
	}

	return ret;
}

template<typename Value>
static void test_indexed_map() {
	using key_type = int32_t;
	using value_type = Value;

	const std::vector<key_type> test_vector { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };

	IndexedMap<key_type, value_type> map { &test_vector };

	for (auto [key, value] : map) {
		value = 2 * key;
		Logger::info("Map entry: ", key, " -> ", value);
	}

	using iterator_t = decltype(map)::iterator;
	using const_iterator_t = decltype(map)::const_iterator;

	iterator_t it = map.begin();
	Logger::info("*it = ", (*it).first, " -> ", (*it).second);

	const_iterator_t it2 = ++it;
	Logger::info("it2 = ++it: it = ", it.key(), " -> ", it.value(), ", it2 = ", it2.key(), " -> ", it2.value());

	it2 = it++;
	Logger::info("it2 = it++: it = ", it.key(), " -> ", it.value(), ", it2 = ", it2.key(), " -> ", it2.value());

	it += 4;
	Logger::info("it += 4 = ", it.key(), " -> ", it.value());

	Logger::info("it[2] = ", it[2].first, " -> ", it[2].second);

	it -= 2;
	Logger::info("it -= 2 = ", it.key(), " -> ", it.value());

	Logger::info("it + 3 = ", (it + 3).key(), " -> ", (it + 3).value());
	Logger::info("2 + it = ", (2 + it).key(), " -> ", (2 + it).value());
	Logger::info("it - 3 = ", (it - 3).key(), " -> ", (it - 3).value());
	Logger::info("it - it2 = ", it - it2);
	Logger::info("it2 - it = ", it2 - it);
	Logger::info("it == it2 = ", it == it2);
	Logger::info("it != it2 = ", it != it2);
	Logger::info("it < it2 = ", it < it2);
	Logger::info("it > it2 = ", it > it2);
	Logger::info("it <= it2 = ", it <= it2);
	Logger::info("it >= it2 = ", it >= it2);

	it2 = --it;
	Logger::info("it2 = --it: it = ", it.key(), " -> ", it.value(), ", it2 = ", it2.key(), " -> ", it2.value());

	it2 = it--;
	Logger::info("it2 = it--: it = ", it.key(), " -> ", it.value(), ", it2 = ", it2.key(), " -> ", it2.value());

	Logger::info("map.begin() = ", map.begin().key(), " -> ", map.begin().value());
	Logger::info("map.end() = ", map.end().key(), " -> ", map.end().value());
	Logger::info("map.end() - 1 = ", (map.end() - 1).key(), " -> ", (map.end() - 1).value());
	Logger::info("map.cbegin() = ", map.cbegin().key(), " -> ", map.cbegin().value());
	Logger::info("map.cend() = ", map.cend().key(), " -> ", map.cend().value());
	Logger::info("map.cend() - 1 = ", (map.cend() - 1).key(), " -> ", (map.cend() - 1).value());

	Logger::info("map.cend().get_key/value_it()[-1] = ", map.cend().get_key_it()[-1], " -> ", map.cend().get_value_it()[-1]);

	Logger::info("map.front() = ", map.front().first, " -> ", map.front().second);
	Logger::info("map.back() = ", map.back().first, " -> ", map.back().second);

	for (key_type const& key : *map.get_keys()) {
		Logger::info(
			"Key: ", key, ", map[key] = ", map[key], ", get_index_from_item(key) = ", map.get_index_from_item(key),
			", *get_item_by_key(key) = ", *map.get_item_by_key(key)
		);
	}

	for (size_t i = 0; i < map.size(); ++i) {
		Logger::info(
			"map[", i, "] = ", map[i], ", map(i) = ", map(i), ", *map.get_key_by_index(i) = ", *map.get_key_by_index(i),
			", map.data()[", i, "] = ", map.data()[i]
		);
	}

	map.fill(6);
	Logger::info("map.fill(6):");
	for (value_type const& value : map.get_values()) {
		Logger::info("Map entry: ", value);
	}

	Logger::info("map.clear():");
	map.clear();
	for (value_type const& value : map.get_values()) {
		Logger::info("Map entry: ", value);
	}
}

/*
	$ program [-h] [-t] [-b] [path]+
*/

int main(int argc, char const* argv[]) {
	Logger::set_logger_funcs();

	Logger::info("========== TESTING int32_t ==========");
	test_indexed_map<int32_t>();

	Logger::info("========== TESTING fixed_point_t ==========");
	test_indexed_map<fixed_point_t>();

	return 0;

	char const* program_name = StringUtils::get_filename(argc > 0 ? argv[0] : nullptr, "<program>");
	fs::path root;
	bool run_tests = false;
	int argn = 0;

	/* Reads the next argument and converts it to a path via path_transform. If reading or converting fails, an error
	 * message and the help text are displayed, along with returning false to signify the program should exit.
	 */
	const auto _read = [&root, &argn, argc, argv, program_name](
		std::string_view command, std::string_view path_use, auto path_transform) -> bool {
		if (root.empty()) {
			if (++argn < argc) {
				char const* path = argv[argn];
				root = path_transform(path);
				if (!root.empty()) {
					return true;
				} else {
					std::cerr << "Empty path after giving \"" << path << "\" to " << path_use
						<< " command line argument \"" << command << "\"." << std::endl;
				}
			} else {
				std::cerr << "Missing path after " << path_use << " command line argument \"" << command << "\"." << std::endl;
			}
		} else {
			std::cerr << "Duplicate " << path_use << " command line argument \"-b\"." << std::endl;
		}
		print_help(std::cerr, program_name);
		return false;
	};

	while (++argn < argc) {
		char const* arg = argv[argn];
		if (strcmp(arg, "-h") == 0) {
			print_help(std::cout, program_name);
			return 0;
		} else if (strcmp(arg, "-t") == 0) {
			run_tests = true;
		} else if (strcmp(arg, "-b") == 0) {
			if (!_read("-b", "base directory", std::identity {})) {
				return -1;
			}
		} else if (strcmp(arg, "-s") == 0) {
			if (!_read("-s", "search hint", Dataloader::search_for_game_path)) {
				return -1;
			}
		} else {
			break;
		}
	}
	if (root.empty()) {
		root = Dataloader::search_for_game_path();
		if (root.empty()) {
			std::cerr << "Search for base directory path failed!" << std::endl;
			print_help(std::cerr, program_name);
			return -1;
		}
	}
	Dataloader::path_vector_t roots { root };
	while (argn < argc) {
		static const fs::path mod_directory = "mod";
		roots.emplace_back(root / mod_directory / argv[argn++]);
	}

	std::cout << "!!! HEADLESS SIMULATION START !!!" << std::endl;

	const bool ret = run_headless(roots, run_tests);

	std::cout << "!!! HEADLESS SIMULATION END !!!" << std::endl;

	std::cout << "\nLoad returned: " << (ret ? "SUCCESS" : "FAILURE") << std::endl;

	std::cout << "\nLogger Summary: Info = " << Logger::get_info_count() << ", Warning = " << Logger::get_warning_count()
		<< ", Error = " << Logger::get_error_count() << std::endl;

	return ret ? 0 : -1;
}

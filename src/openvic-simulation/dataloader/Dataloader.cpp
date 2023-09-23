#include "Dataloader.hpp"
#include <filesystem>

#include <openvic-dataloader/csv/Parser.hpp>
#include <openvic-dataloader/detail/CallbackOStream.hpp>
#include <openvic-dataloader/v2script/Parser.hpp>

#include "openvic-simulation/GameManager.hpp"
#include "openvic-simulation/utility/Logger.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;
using namespace ovdl;

bool Dataloader::set_roots(path_vector_t new_roots) {
	if (!roots.empty()) {
		Logger::error("Overriding existing dataloader roots!");
		roots.clear();
	}
	bool ret = true;
	for (std::reverse_iterator<path_vector_t::const_iterator> it = new_roots.crbegin(); it != new_roots.crend(); ++it) {
		if (std::find(roots.begin(), roots.end(), *it) == roots.end()) {
			if (fs::is_directory(*it)) {
				Logger::info("Adding dataloader root: ", *it);
				roots.push_back(*it);
			} else {
				Logger::error("Invalid dataloader root (must be an existing directory): ", *it);
				ret = false;
			}
		} else {
			Logger::error("Duplicate dataloader root: ", *it);
			ret = false;
		}
	}
	if (roots.empty()) {
		Logger::error("Dataloader has no roots after attempting to add ", new_roots.size());
		ret = false;
	}
	return ret;
}

fs::path Dataloader::lookup_file(fs::path const& path) const {
	for (fs::path const& root : roots) {
		const fs::path composed = root / path;
		if (fs::is_regular_file(composed)) {
			return composed;
		}
	}
	Logger::error("Lookup for ", path, " failed!");
	return {};
}

static bool contains_file_with_name(Dataloader::path_vector_t const& paths, fs::path const& name) {
	for (fs::path const& path : paths) {
		if (path.filename() == name) return true;
	}
	return false;
}

Dataloader::path_vector_t Dataloader::lookup_files_in_dir(fs::path const& path, fs::path const& extension) const {
	path_vector_t ret;
	for (fs::path const& root : roots) {
		const fs::path composed = root / path;
		std::error_code ec;
		for (fs::directory_entry const& entry : fs::directory_iterator { composed, ec }) {
			if (entry.is_regular_file()) {
				const fs::path file = entry;
				if (extension.empty() || file.extension() == extension) {
					if (!contains_file_with_name(ret, file.filename())) {
						ret.push_back(file);
					}
				}
			}
		}
	}
	return ret;
}

bool Dataloader::apply_to_files_in_dir(fs::path const& path, fs::path const& extension, callback_t<fs::path const&> callback) const {
	bool ret = true;
	for (fs::path const& file : lookup_files_in_dir(path, extension)) {
		ret &= callback(file);
	}
	return ret;
}

template<std::derived_from<detail::BasicParser> Parser, bool (*parse_func)(Parser&)>
static Parser _run_ovdl_parser(fs::path const& path) {
	Parser parser;
	std::string buffer;
	auto error_log_stream = detail::CallbackStream {
		[](void const* s, std::streamsize n, void* user_data) -> std::streamsize {
			if (s != nullptr && n > 0 && user_data != nullptr) {
				static_cast<std::string*>(user_data)->append(static_cast<char const*>(s), n);
				return n;
			} else {
				Logger::error("Invalid input to parser error log callback: ", s, " / ", n, " / ", user_data);
				return 0;
			}
		},
		&buffer
	};
	parser.set_error_log_to(error_log_stream);
	parser.load_from_file(path);
	if (!buffer.empty()) {
		Logger::error("Parser load errors for ", path, ":\n\n", buffer, "\n");
		buffer.clear();
	}
	if (parser.has_fatal_error() || parser.has_error()) {
		Logger::error("Parser errors while loading ", path);
		return parser;
	}
	if (!parse_func(parser)) {
		Logger::error("Parse function returned false for ", path, "!");
	}
	if (!buffer.empty()) {
		Logger::error("Parser parse errors for ", path, ":\n\n", buffer, "\n");
		buffer.clear();
	}
	if (parser.has_fatal_error() || parser.has_error()) {
		Logger::error("Parser errors while parsing ", path);
	}
	return parser;
}

static bool _v2script_parse(v2script::Parser& parser) {
	return parser.simple_parse();
}

static v2script::Parser _parse_defines(fs::path const& path) {
	return _run_ovdl_parser<v2script::Parser, &_v2script_parse>(path);
}

static bool _csv_parse(csv::Windows1252Parser& parser) {
	return parser.parse_csv();
}

static csv::Windows1252Parser _parse_csv(fs::path const& path) {
	return _run_ovdl_parser<csv::Windows1252Parser, &_csv_parse>(path);
}

static callback_t<fs::path const&> _parse_defines_callback(node_callback_t callback) {
	return [callback](fs::path const& path) -> bool {
		return callback(_parse_defines(path).get_file_node());
	};
}

bool Dataloader::_load_pop_types(PopManager& pop_manager, fs::path const& pop_type_directory) const {
	const bool ret = apply_to_files_in_dir(pop_type_directory, ".txt",
		[&pop_manager](fs::path const& file) -> bool {
			return pop_manager.load_pop_type_file(file.stem().string(), _parse_defines(file).get_file_node());
		}
	);
	pop_manager.lock_pop_types();
	return ret;
}

bool Dataloader::_load_units(GameManager& game_manager, fs::path const& units_directory) const {
	const bool ret = apply_to_files_in_dir(units_directory, ".txt",
		[&game_manager](fs::path const& file) -> bool {
			return game_manager.get_unit_manager().load_unit_file(game_manager.get_good_manager(), _parse_defines(file).get_file_node());
		}
	);
	game_manager.get_unit_manager().lock_units();
	return ret;
}

bool Dataloader::_load_map_dir(GameManager& game_manager, fs::path const& map_directory) const {
	Map& map = game_manager.get_map();

	static const fs::path defaults_filename = "default.map";
	static const std::string default_definitions = "definition.csv";
	static const std::string default_provinces = "provinces.bmp";
	static const std::string default_positions = "positions.txt";
	static const std::string default_terrain = "terrain.bmp";
	static const std::string default_rivers = "rivers.bmp";
	static const std::string default_terrain_definition = "terrain.txt";
	static const std::string default_tree_definition = "trees.txt";
	static const std::string default_continent = "continent.txt";
	static const std::string default_adjacencies = "adjacencies.csv";
	static const std::string default_region = "region.txt";
	static const std::string default_region_sea = "region_sea.txt";
	static const std::string default_province_flag_sprite = "province_flag_sprites";

	const v2script::Parser parser = _parse_defines(lookup_file(map_directory / defaults_filename));

	std::vector<std::string_view> water_province_identifiers;

#define APPLY_TO_MAP_PATHS(F) \
	F(definitions) F(provinces) F(positions) F(terrain) F(rivers) \
	F(terrain_definition) F(tree_definition) F(continent) F(adjacencies) \
	F(region) F(region_sea) F(province_flag_sprite)

#define MAP_PATH_VAR(X) std::string_view X = default_##X;
	APPLY_TO_MAP_PATHS(MAP_PATH_VAR)
#undef MAP_PATH_VAR

	bool ret = expect_dictionary_keys(
		"max_provinces", ONE_EXACTLY,
		expect_uint(
			[&map](uint64_t val) -> bool {
				if (Province::NULL_INDEX < val && val <= Province::MAX_INDEX) {
					return map.set_max_provinces(val);
				}
				Logger::error("Invalid max province count ", val, " (out of valid range ",
					Province::NULL_INDEX, " < max_provinces <= ", Province::MAX_INDEX, ")");
				return false;
			}
		),
		"sea_starts", ONE_EXACTLY,
		expect_list_reserve_length(
			water_province_identifiers,
			expect_identifier(
				[&water_province_identifiers](std::string_view identifier) -> bool {
					water_province_identifiers.push_back(identifier);
					return true;
				}
			)
		),

#define MAP_PATH_DICT_ENTRY(X) \
		#X, ONE_EXACTLY, expect_string(assign_variable_callback(X)),
		APPLY_TO_MAP_PATHS(MAP_PATH_DICT_ENTRY)
#undef MAP_PATH_DICT_ENTRY

#undef APPLY_TO_MAP_PATHS

		"border_heights", ZERO_OR_ONE, success_callback,
		"terrain_sheet_heights", ZERO_OR_ONE, success_callback,
		"tree", ZERO_OR_ONE, success_callback,
		"border_cutoff", ZERO_OR_ONE, success_callback
	)(parser.get_file_node());

	if (!ret) {
		Logger::error("Failed to load map default file!");
	}

	if (!map.load_province_definitions(_parse_csv(lookup_file(map_directory / definitions)).get_lines())) {
		Logger::error("Failed to load province definitions file!");
		ret = false;
	}

	if (!map.load_province_positions(game_manager.get_building_manager(), _parse_defines(lookup_file(map_directory / positions)).get_file_node())) {
		Logger::error("Failed to load province positions file!");
		ret = false;
	}

	if (!map.load_region_file(_parse_defines(lookup_file(map_directory / region)).get_file_node())) {
		Logger::error("Failed to load region file!");
		ret = false;
	}

	if (!map.set_water_province_list(water_province_identifiers)) {
		Logger::error("Failed to set water provinces!");
		ret = false;
	}
	map.lock_water_provinces();

	return ret;
}

bool Dataloader::load_defines(GameManager& game_manager) const {
	static const fs::path goods_file = "common/goods.txt";
	static const fs::path pop_type_directory = "poptypes";
	static const fs::path graphical_culture_type_file = "common/graphicalculturetype.txt";
	static const fs::path culture_file = "common/cultures.txt";
	static const fs::path religion_file = "common/religion.txt";
	static const fs::path ideology_file = "common/ideologies.txt";
	static const fs::path issues_file = "common/issues.txt";
	static const fs::path map_directory = "map";
	static const fs::path units_directory = "units";

	bool ret = true;

	if (!game_manager.get_good_manager().load_goods_file(_parse_defines(lookup_file(goods_file)).get_file_node())) {
		Logger::error("Failed to load goods!");
		ret = false;
	}
	if (!_load_pop_types(game_manager.get_pop_manager(), pop_type_directory)) {
		Logger::error("Failed to load pop types!");
		ret = false;
	}
	if (!game_manager.get_pop_manager().get_culture_manager().load_graphical_culture_type_file(_parse_defines(lookup_file(graphical_culture_type_file)).get_file_node())) {
		Logger::error("Failed to load graphical culture types!");
		ret = false;
	}
	if (!game_manager.get_pop_manager().get_culture_manager().load_culture_file(_parse_defines(lookup_file(culture_file)).get_file_node())) {
		Logger::error("Failed to load cultures!");
		ret = false;
	}
	if (!game_manager.get_pop_manager().get_religion_manager().load_religion_file(_parse_defines(lookup_file(religion_file)).get_file_node())) {
		Logger::error("Failed to load religions!");
		ret = false;
	}
	if (!game_manager.get_ideology_manager().load_ideology_file(_parse_defines(lookup_file(ideology_file)).get_file_node())) {
		Logger::error("Failed to load ideologies!");
		ret = false;
	}
	if (!game_manager.get_issue_manager().load_issues_file(_parse_defines(lookup_file(issues_file)).get_file_node())) {
		Logger::error("Failed to load issues!");
		ret = false;
	}
	if (!_load_units(game_manager, units_directory)) {
		Logger::error("Failed to load units!");
		ret = false;
	}
	if (!_load_map_dir(game_manager, map_directory)) {
		Logger::error("Failed to load map!");
		ret = false;
	}

	return ret;
}

bool Dataloader::load_pop_history(GameManager& game_manager, fs::path const& path) const {
	return apply_to_files_in_dir(path, ".txt",
		[&game_manager](fs::path const& file) -> bool {
			return _parse_defines_callback(game_manager.get_map().expect_province_dictionary(
				[&game_manager](Province& province, ast::NodeCPtr value) -> bool {
					return province.load_pop_list(game_manager.get_pop_manager(), value);
				}
			))(file);
		}
	);
}

static bool _load_localisation_file(Dataloader::localisation_callback_t callback, std::vector<csv::LineObject> const& lines) {
	bool ret = true;
	for (csv::LineObject const& line : lines) {
		const std::string_view key = line.get_value_for(0);
		if (!key.empty()) {
			const size_t max_entry = std::min<size_t>(line.value_count() - 1, Dataloader::_LocaleCount);
			for (size_t i = 0; i < max_entry; ++i) {
				const std::string_view entry = line.get_value_for(i + 1);
				if (!entry.empty()) {
					ret &= callback(key, static_cast<Dataloader::locale_t>(i), entry);
				}
			}
		}
	}
	return ret;
}

bool Dataloader::load_localisation_files(localisation_callback_t callback, fs::path const& localisation_dir) {
	return apply_to_files_in_dir(localisation_dir, ".csv",
		[callback](fs::path path) -> bool {
			return _load_localisation_file(callback, _parse_csv(path).get_lines());
		}
	);
}

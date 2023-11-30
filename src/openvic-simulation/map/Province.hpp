#pragma once

#include <cassert>

#include "openvic-simulation/economy/BuildingInstance.hpp"
#include "openvic-simulation/politics/Ideology.hpp"
#include "openvic-simulation/pop/Pop.hpp"
#include "openvic-simulation/country/Country.hpp"

namespace OpenVic {
	struct Map;
	struct Region;
	struct Good;
	struct TerrainType;
	struct TerrainTypeMapping;
	struct ProvinceHistoryEntry;

	/* REQUIREMENTS:
	 * MAP-5, MAP-7, MAP-8, MAP-43, MAP-47
	 * POP-22
	 */
	struct Province : HasIdentifierAndColour {
		friend struct Map;

		using index_t = uint16_t;
		using life_rating_t = int8_t;
		using distance_t = uint16_t;
		using flags_t = uint16_t;

		enum struct colony_status_t : int8_t { STATE, PROTECTORATE, COLONY };

		struct adjacency_t {
			friend struct Province;
			enum type_t {standard, sea, impassable, canal};

		private:
			type_t PROPERTY(type);
			Province const* const province;
			const distance_t PROPERTY(distance);
			Province const* const through_province;
			//For now using Flags as the "data" section of adjacencies.csv
			flags_t PROPERTY(flags);

			adjacency_t(Province const* province, distance_t distance, flags_t flags);
			adjacency_t(Province const* province, distance_t distance, flags_t flags, type_t type, Province const* through_province);
		};

		struct province_positions_t {
			fvec2_t text;
			fixed_point_t text_rotation;
			fixed_point_t text_scale;
			fvec2_t unit;
			fvec2_t city;
			fvec2_t factory;
			fvec2_t building_construction;
			fvec2_t military_construction;
			fvec2_t fort;
			fixed_point_t fort_rotation;
			fvec2_t railroad;
			fixed_point_t railroad_rotation;
			fvec2_t navalbase;
			fixed_point_t navalbase_rotation;
		};

		static constexpr index_t NULL_INDEX = 0, MAX_INDEX = std::numeric_limits<index_t>::max();

	private:
		const index_t PROPERTY(index);
		Region* PROPERTY(region);
		bool PROPERTY(on_map);
		bool PROPERTY(has_region);
		bool PROPERTY(water);
		/* Terrain type calculated from terrain image */
		TerrainType const* PROPERTY(default_terrain_type);

		std::vector<adjacency_t> PROPERTY(adjacencies);
		province_positions_t positions;

		TerrainType const* PROPERTY(terrain_type);
		life_rating_t PROPERTY(life_rating);
		colony_status_t PROPERTY(colony_status);
		Country const* PROPERTY(owner);
		Country const* PROPERTY(controller);
		std::vector<Country const*> PROPERTY(cores);
		bool PROPERTY(slave);
		// TODO - change this into a factory-like structure
		Good const* PROPERTY(rgo);
		IdentifierRegistry<BuildingInstance> buildings;

		std::vector<Pop> PROPERTY(pops);
		Pop::pop_size_t PROPERTY(total_population);
		fixed_point_map_t<PopType const*> PROPERTY(pop_type_distribution);
		fixed_point_map_t<Ideology const*> PROPERTY(ideology_distribution);
		fixed_point_map_t<Culture const*> PROPERTY(culture_distribution);
		fixed_point_map_t<Religion const*> PROPERTY(religion_distribution);

		Province(std::string_view new_identifier, colour_t new_colour, index_t new_index);

	public:
		Province(Province&&) = default;

		std::string to_string() const;

		bool load_positions(BuildingManager const& building_manager, ast::NodeCPtr root);

		IDENTIFIER_REGISTRY_ACCESSORS(building)
		bool expand_building(std::string_view building_type_identifier);

		bool load_pop_list(PopManager const& pop_manager, ast::NodeCPtr root);
		bool add_pop(Pop&& pop);
		size_t get_pop_count() const;
		void update_pops();

		void update_state(Date today);
		void tick(Date today);

		bool is_adjacent_to(Province const* province);
		bool add_adjacency(Province const* province, distance_t distance, flags_t flags);
		bool add_special_adjacency(Province const* province, distance_t distance, flags_t flags, adjacency_t::type_t type, Province const* through);
		adjacency_t::type_t get_adjacency_type_from_string(std::String type);

		bool reset(BuildingManager const& building_manager);
		bool apply_history_to_province(ProvinceHistoryEntry const* entry);
	};
}

#pragma once

#include "openvic-simulation/map/Building.hpp"
#include "openvic-simulation/pop/Pop.hpp"

namespace OpenVic {
	struct Map;
	struct Region;
	struct Good;

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

		struct adjacency_t {
			friend struct Province;

		private:
			Province const* const province;
			const distance_t distance;
			flags_t flags;

			adjacency_t(Province const* province, distance_t distance, flags_t flags);

		public:
			distance_t get_distance() const;
			flags_t get_flags();
		};

		static constexpr index_t NULL_INDEX = 0, MAX_INDEX = (1 << (8 * sizeof(index_t))) - 1;

	private:
		const index_t index;
		Region* region = nullptr;
		bool has_region = false, water = false;
		life_rating_t life_rating = 0;
		IdentifierRegistry<Building> buildings;
		// TODO - change this into a factory-like structure
		Good const* rgo = nullptr;

		std::vector<Pop> pops;
		Pop::pop_size_t total_population;
		distribution_t pop_types, cultures, religions;

		std::vector<adjacency_t> adjacencies;

		Province(const std::string_view new_identifier, colour_t new_colour, index_t new_index);

	public:
		Province(Province&&) = default;

		index_t get_index() const;
		Region* get_region() const;
		bool get_has_region() const;
		bool get_water() const;
		life_rating_t get_life_rating() const;
		bool load_positions(BuildingManager const& building_manager, ast::NodeCPtr root);

		bool add_building(Building&& building);
		IDENTIFIER_REGISTRY_ACCESSORS(Building, building)
		void reset_buildings();
		bool expand_building(const std::string_view building_type_identifier);
		Good const* get_rgo() const;
		std::string to_string() const;

		bool load_pop_list(PopManager const& pop_manager, ast::NodeCPtr root);
		bool add_pop(Pop&& pop);
		void clear_pops();
		size_t get_pop_count() const;
		std::vector<Pop> const& get_pops() const;
		Pop::pop_size_t get_total_population() const;
		distribution_t const& get_pop_type_distribution() const;
		distribution_t const& get_culture_distribution() const;
		distribution_t const& get_religion_distribution() const;
		void update_pops();

		void update_state(Date const& today);
		void tick(Date const& today);

		bool is_adjacent_to(Province const* province);
		bool add_adjacency(Province const* province, distance_t distance, flags_t flags);
		std::vector<adjacency_t> const& get_adjacencies() const;
	};
}

#pragma once

#include "openvic-simulation/GameAdvancementHook.hpp"
#include "openvic-simulation/economy/Good.hpp"
#include "openvic-simulation/map/Map.hpp"
#include "openvic-simulation/politics/Ideology.hpp"
#include "openvic-simulation/politics/Issue.hpp"
#include "openvic-simulation/GameAdvancementHook.hpp"
#include "openvic-simulation/economy/Good.hpp"
#include "openvic-simulation/map/Map.hpp"
#include "openvic-simulation/units/Unit.hpp"

namespace OpenVic {
	struct GameManager {
		using state_updated_func_t = std::function<void()>;

	private:
		Map map;
		BuildingManager building_manager;
		GoodManager good_manager;
		PopManager pop_manager;
		IdeologyManager ideology_manager;
		IssueManager issue_manager;
		UnitManager unit_manager;
		GameAdvancementHook clock;

		time_t session_start; /* SS-54, as well as allowing time-tracking */
		Date today;
		state_updated_func_t state_updated;
		bool needs_update;

		void set_needs_update();
		void update_state();
		void tick();

	public:
		GameManager(state_updated_func_t state_updated_callback);
		Map* get_map();
		BuildingManager* get_building_manager();
		GoodManager* get_good_manager();
		PopManager* get_pop_manager();
		GameAdvancementHook* get_game_advancement_hook();

		Map& get_map();
		Map const& get_map() const;
		BuildingManager& get_building_manager();
		BuildingManager const& get_building_manager() const;
		GoodManager& get_good_manager();
		GoodManager const& get_good_manager() const;
		PopManager& get_pop_manager();
		PopManager const& get_pop_manager() const;
		IdeologyManager& get_ideology_manager();
		IdeologyManager const& get_ideology_manager() const;
		IssueManager& get_issue_manager();
		IssueManager const& get_issue_manager() const;
		UnitManager& get_unit_manager();
		UnitManager const& get_unit_manager() const;
		GameAdvancementHook& get_clock();
		GameAdvancementHook const& get_clock() const;

		bool setup();

		Date const& get_today() const;
		bool expand_building(Province::index_t province_index, const std::string_view building_type_identifier);

		/* Hardcoded data for defining things for which parsing from files has
		 * not been implemented, currently mapmodes and building types.
		 */
		bool load_hardcoded_defines();
	};
}

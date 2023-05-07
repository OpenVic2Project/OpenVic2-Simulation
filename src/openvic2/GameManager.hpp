#pragma once

#include "GameAdvancementHook.hpp"
#include "map/Map.hpp"

namespace OpenVic2 {
	struct GameManager {
		using state_updated_func_t = std::function<void()>;

		Map map;
		BuildingManager building_manager;
		GameAdvancementHook clock;
		time_t session_start; /* SS-54, as well as allowing time-tracking */
	private:
		Date today;
		state_updated_func_t state_updated;
		bool needs_update;

		void set_needs_update();
		void update_state();
		void tick();
	public:
		GameManager(state_updated_func_t state_updated_callback);

		return_t setup();

		Date const& get_today() const;
		return_t expand_building(index_t province_index, std::string const& building_type_identifier);
	};
}

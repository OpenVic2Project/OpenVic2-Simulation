#include "InstanceManager.hpp"

#include "openvic-simulation/DefinitionManager.hpp"
#include "openvic-simulation/console/ConsoleInstance.hpp"
#include "openvic-simulation/utility/Logger.hpp"

using namespace OpenVic;

InstanceManager::InstanceManager(
	GameRulesManager const& new_game_rules_manager,
	DefinitionManager const& new_definition_manager,
	gamestate_updated_func_t gamestate_updated_callback,
	SimulationClock::state_changed_function_t clock_state_changed_callback
) : definition_manager { new_definition_manager },
	game_rules_manager { new_game_rules_manager },
	good_instance_manager { new_definition_manager.get_economy_manager().get_good_definition_manager() },
	market_instance { new_definition_manager.get_define_manager().get_country_defines(), good_instance_manager },
	artisanal_producer_factory_pattern {
		new_definition_manager.get_modifier_manager().get_modifier_effect_cache(),
		new_definition_manager.get_economy_manager().get_production_type_manager()
	},
	global_flags { "global" },
	country_instance_manager { new_definition_manager.get_country_definition_manager() },
	politics_instance_manager { *this },
	map_instance { new_definition_manager.get_map_definition() },
	simulation_clock {
		std::bind(&InstanceManager::tick, this), std::bind(&InstanceManager::update_gamestate, this),
		clock_state_changed_callback ? std::move(clock_state_changed_callback) : []() {}
	},
	console_instance { *this },
	gamestate_updated { gamestate_updated_callback ? std::move(gamestate_updated_callback) : []() {} } {}

void InstanceManager::set_gamestate_needs_update() {
	if (!currently_updating_gamestate) {
		gamestate_needs_update = true;
	} else {
		Logger::error("Attempted to queue a gamestate update already updating the gamestate!");
	}
}

void InstanceManager::update_gamestate() {
	if (!gamestate_needs_update) {
		return;
	}
	currently_updating_gamestate = true;

	Logger::info("Update: ", today);

	update_modifier_sums();

	// Update gamestate...
	map_instance.update_gamestate(today, definition_manager.get_define_manager());
	country_instance_manager.update_gamestate(*this);
	unit_instance_manager.update_gamestate();

	gamestate_updated();
	gamestate_needs_update = false;

	currently_updating_gamestate = false;
}

/* REQUIREMENTS:
 * SS-98, SS-101
 */
void InstanceManager::tick() {
	today++;

	Logger::info("Tick: ", today);

	// Tick...
	map_instance.map_tick(today);
	country_instance_manager.tick(*this);
	unit_instance_manager.tick();
	market_instance.execute_orders();

	if (today.is_month_start()) {
		market_instance.record_price_history();
	}

	set_gamestate_needs_update();
}

bool InstanceManager::setup() {
	if (is_game_instance_setup()) {
		Logger::error("Cannot setup game instance - already set up!");
		return false;
	}

	bool ret = good_instance_manager.setup_goods(
		game_rules_manager
	);
	ret &= map_instance.setup(
		definition_manager.get_economy_manager().get_building_type_manager(),
		market_instance,
		definition_manager.get_modifier_manager().get_modifier_effect_cache(),
		definition_manager.get_define_manager().get_pops_defines(),
		definition_manager.get_pop_manager().get_stratas(),
		definition_manager.get_pop_manager().get_pop_types(),
		definition_manager.get_politics_manager().get_ideology_manager().get_ideologies()
	);
	ret &= country_instance_manager.generate_country_instances(
		definition_manager.get_economy_manager().get_building_type_manager().get_building_types(),
		definition_manager.get_research_manager().get_technology_manager().get_technologies(),
		definition_manager.get_research_manager().get_invention_manager().get_inventions(),
		definition_manager.get_politics_manager().get_ideology_manager().get_ideologies(),
		definition_manager.get_politics_manager().get_issue_manager().get_reform_groups(),
		definition_manager.get_politics_manager().get_government_type_manager().get_government_types(),
		definition_manager.get_crime_manager().get_crime_modifiers(),
		definition_manager.get_pop_manager().get_pop_types(),
		good_instance_manager.get_good_instances(),
		definition_manager.get_military_manager().get_unit_type_manager().get_regiment_types(),
		definition_manager.get_military_manager().get_unit_type_manager().get_ship_types(),
		definition_manager.get_pop_manager().get_stratas(),
		good_instance_manager
	);

	game_instance_setup = true;

	return ret;
}

bool InstanceManager::load_bookmark(Bookmark const* new_bookmark) {
	if (is_bookmark_loaded()) {
		Logger::error("Cannot load bookmark - already loaded!");
		return false;
	}

	if (!is_game_instance_setup()) {
		Logger::error("Cannot load bookmark - game instance not set up!");
		return false;
	}

	if (new_bookmark == nullptr) {
		Logger::error("Cannot load bookmark - null!");
		return false;
	}

	bookmark = new_bookmark;

	Logger::info("Loading bookmark ", bookmark->get_name(), " with start date ", bookmark->get_date());

	if (!definition_manager.get_define_manager().in_game_period(bookmark->get_date())) {
		Logger::warning("Bookmark date ", bookmark->get_date(), " is not in the game's time period!");
	}

	today = bookmark->get_date();

	politics_instance_manager.setup_starting_ideologies();
	bool ret = map_instance.apply_history_to_provinces(
		definition_manager.get_history_manager().get_province_manager(), today,
		country_instance_manager,
		// TODO - the following argument is for generating test pop attributes
		definition_manager.get_politics_manager().get_issue_manager(),
		market_instance,
		artisanal_producer_factory_pattern
	);

	// It is important that province history is applied before country history as province history includes
	// generating pops which then have stats like literacy and consciousness set by country history.

	ret &= country_instance_manager.apply_history_to_countries(
		definition_manager.get_history_manager().get_country_manager(), *this
	);

	ret &= map_instance.get_state_manager().generate_states(
		map_instance,
		definition_manager.get_pop_manager().get_stratas(),
		definition_manager.get_pop_manager().get_pop_types(),
		definition_manager.get_politics_manager().get_ideology_manager().get_ideologies()
	);

	if (ret) {
		update_modifier_sums();
		map_instance.initialise_for_new_game(
			today,
			definition_manager.get_define_manager()
		);
		market_instance.execute_orders();
	}

	return ret;
}

bool InstanceManager::start_game_session() {
	if (is_game_session_started()) {
		Logger::error("Cannot start game session - already started!");
		return false;
	}

	session_start = time(nullptr);
	simulation_clock.reset();
	set_gamestate_needs_update();

	game_session_started = true;

	return true;
}

bool InstanceManager::update_clock() {
	if (!is_game_session_started()) {
		Logger::error("Cannot update clock - game session not started!");
		return false;
	}

	simulation_clock.conditionally_advance_game();
	return true;
}

bool InstanceManager::set_today_and_update(Date new_today) {
	if (!is_game_session_started()) {
		Logger::error("Cannot update clock - game session not started!");
		return false;
	}

	today = new_today;
	gamestate_needs_update = true;
	update_gamestate();
	return true;
}

bool InstanceManager::expand_selected_province_building(size_t building_index) {
	set_gamestate_needs_update();
	ProvinceInstance* province = map_instance.get_selected_province();
	if (province == nullptr) {
		Logger::error("Cannot expand building index ", building_index, " - no province selected!");
		return false;
	}
	if (building_index < 0) {
		Logger::error("Invalid building index ", building_index, " while trying to expand in province ", province);
		return false;
	}
	return province->expand_building(building_index);
}

void InstanceManager::update_modifier_sums() {
	// Calculate national country modifier sums first, then local province modifier sums, adding province contributions
	// to controller countries' modifier sums if each province has a controller. This results in every country having a
	// full copy of all the modifiers affecting them in their modifier sum, but provinces only having their directly/locally
	// applied modifiers in their modifier sum, hence requiring owner country modifier effect values to be looked up when
	// determining the value of a global effect on the province.
	country_instance_manager.update_modifier_sums(
		today, definition_manager.get_modifier_manager().get_static_modifier_cache()
	);
	map_instance.update_modifier_sums(
		today, definition_manager.get_modifier_manager().get_static_modifier_cache()
	);
}

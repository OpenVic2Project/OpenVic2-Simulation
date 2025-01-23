#include "GameManager.hpp"

using namespace OpenVic;

GameManager::GameManager(
	InstanceManager::gamestate_updated_func_t new_gamestate_updated_callback,
	SimulationClock::state_changed_function_t new_clock_state_changed_callback
) : gamestate_updated_callback {
		new_gamestate_updated_callback ? std::move(new_gamestate_updated_callback) : []() {}
	}, clock_state_changed_callback {
		new_clock_state_changed_callback ? std::move(new_clock_state_changed_callback) : []() {}
	}, definitions_loaded { false } {}

bool GameManager::set_roots(Dataloader::path_vector_t const& roots) {
	if (!dataloader.set_roots(roots)) {
		Logger::error("Failed to set dataloader roots!");
		return false;
	}
	return true;
}

bool GameManager::load_definitions(Dataloader::localisation_callback_t localisation_callback) {
	if (definitions_loaded) {
		Logger::error("Cannot load definitions - already loaded!");
		return false;
	}

	bool ret = true;

	if (!dataloader.load_defines(game_rules_manager, definition_manager)) {
		Logger::error("Failed to load defines!");
		ret = false;
	}

	if (!dataloader.load_localisation_files(localisation_callback)) {
		Logger::error("Failed to load localisation!");
		ret = false;
	}

	definitions_loaded = true;

	return ret;
}

bool GameManager::setup_instance(
	Bookmark const* bookmark, update_modifier_sum_rule_t first, update_modifier_sum_rule_t second
) {
	if (instance_manager) {
		Logger::info("Resetting existing game instance.");
	} else {
		Logger::info("Setting up first game instance.");
	}

	bool ret = true;

#if OV_MODIFIER_CALCULATION_TEST
	instance_manager.emplace(
		// update_modifier_sum_rule_t { PROVINCES_THEN_COUNTRIES | ADD_OWNER_CONTRIBUTIONS | ADD_OWNER_VIA_PROVINCES },
		first,
		game_rules_manager, definition_manager, gamestate_updated_callback, clock_state_changed_callback
	);
	ret &= instance_manager->setup();
	ret &= instance_manager->load_bookmark(bookmark);

	instance_manager_no_add.emplace(
		// update_modifier_sum_rule_t { COUNTRIES_THEN_PROVINCES | DONT_ADD_OWNER_CONTRIBUTIONS | ADD_OWNER_VIA_COUNTRIES },
		second,
		game_rules_manager, definition_manager, gamestate_updated_callback, clock_state_changed_callback
	);
	ret &= instance_manager_no_add->setup();
	ret &= instance_manager_no_add->load_bookmark(bookmark);
#else
	instance_manager.emplace(
		game_rules_manager, definition_manager, gamestate_updated_callback, clock_state_changed_callback
	);
	ret &= instance_manager->setup();
	ret &= instance_manager->load_bookmark(bookmark);
#endif

	return ret;
}

bool GameManager::start_game_session() {
	if (!instance_manager || !instance_manager->is_game_instance_setup()) {
		Logger::error("Cannot start game session - instance manager not set up!");
		return false;
	}

	if (instance_manager->is_game_session_started()) {
		Logger::error("Cannot start game session - session already started!");
		return false;
	}

	if (!instance_manager->is_bookmark_loaded()) {
		Logger::warning("Starting game session with no bookmark loaded!");
	}

#if OV_MODIFIER_CALCULATION_TEST
	return instance_manager->start_game_session() & instance_manager_no_add->start_game_session();
#else
	return instance_manager->start_game_session();
#endif
}

bool GameManager::update_clock() {
	if (!instance_manager) {
		Logger::error("Cannot update clock - instance manager uninitialised!");
		return false;
	}

#if OV_MODIFIER_CALCULATION_TEST
	return instance_manager->update_clock() & instance_manager_no_add->update_clock();
#else
	return instance_manager->update_clock();
#endif
}

#pragma once

#include <optional>

#include "openvic-simulation/dataloader/Dataloader.hpp"
#include "openvic-simulation/DefinitionManager.hpp"
#include "openvic-simulation/InstanceManager.hpp"
#include "openvic-simulation/dataloader/ModManager.hpp"
#include "openvic-simulation/misc/GameRulesManager.hpp"

namespace OpenVic {
	struct GameManager {
	private:
		GameRulesManager PROPERTY(game_rules_manager);
		Dataloader PROPERTY(dataloader);
		DefinitionManager PROPERTY(definition_manager);
		ModManager PROPERTY(mod_manager);
		std::optional<InstanceManager> instance_manager;

		InstanceManager::gamestate_updated_func_t gamestate_updated_callback;
		SimulationClock::state_changed_function_t clock_state_changed_callback;
		bool PROPERTY_CUSTOM_PREFIX(definitions_loaded, are);
		bool PROPERTY_CUSTOM_PREFIX(mod_descriptors_loaded, are);

	public:
		GameManager(
			InstanceManager::gamestate_updated_func_t new_gamestate_updated_callback,
			SimulationClock::state_changed_function_t new_clock_state_changed_callback
		);

		inline constexpr InstanceManager* get_instance_manager() {
			return instance_manager ? &*instance_manager : nullptr;
		}

		inline constexpr InstanceManager const* get_instance_manager() const {
			return instance_manager ? &*instance_manager : nullptr;
		}

		bool set_roots(Dataloader::path_vector_t const& roots, Dataloader::path_vector_t const& replace_paths);

		bool load_mod_descriptors(std::vector<std::string> descriptors);

		bool load_definitions(Dataloader::localisation_callback_t localisation_callback);

		bool setup_instance(Bookmark const* bookmark);

		bool start_game_session();

		bool update_clock();
	};
}

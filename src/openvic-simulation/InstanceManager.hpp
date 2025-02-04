#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <ostream>
#include <variant>

#include "openvic-simulation/console/ConsoleInstance.hpp"
#include "openvic-simulation/country/CountryInstance.hpp"
#include "openvic-simulation/diplomacy/CountryRelation.hpp"
#include "openvic-simulation/economy/GoodInstance.hpp"
#include "openvic-simulation/economy/trading/MarketInstance.hpp"
#include "openvic-simulation/map/MapInstance.hpp"
#include "openvic-simulation/map/Mapmode.hpp"
#include "openvic-simulation/military/UnitInstanceGroup.hpp"
#include "openvic-simulation/misc/SimulationClock.hpp"
#include "openvic-simulation/politics/PoliticsInstanceManager.hpp"
#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/types/FlagStrings.hpp"

namespace OpenVic {
	enum struct game_action_type_t : uint64_t {
		GAME_ACTION_NONE,

		// Core
		GAME_ACTION_TICK,
		GAME_ACTION_SET_PAUSE,
		GAME_ACTION_SET_SPEED,
		GAME_ACTION_SET_AI,

		// Production
		GAME_ACTION_EXPAND_PROVINCE_BUILDING,

		// Budget
		GAME_ACTION_SET_STRATA_TAX,
		GAME_ACTION_SET_LAND_SPENDING,
		GAME_ACTION_SET_NAVAL_SPENDING,
		GAME_ACTION_SET_CONSTRUCTION_SPENDING,
		GAME_ACTION_SET_EDUCATION_SPENDING,
		GAME_ACTION_SET_ADMINISTRATION_SPENDING,
		GAME_ACTION_SET_SOCIAL_SPENDING,
		GAME_ACTION_SET_MILITARY_SPENDING,
		GAME_ACTION_SET_TARIFF_RATE,

		// Technology
		GAME_ACTION_START_RESEARCH,

		// Politics

		// Population

		// Trade

		// Diplomacy

		// Military
		GAME_ACTION_CREATE_GENERAL,
		GAME_ACTION_CREATE_ADMIRAL,
		GAME_ACTION_SET_USE_LEADER,
		GAME_ACTION_SET_AUTO_CREATE_LEADERS,
		GAME_ACTION_SET_AUTO_ASSIGN_LEADERS,
		GAME_ACTION_SET_MOBILISE,
		// rally point settings?

		MAX_GAME_ACTION
	};

	using game_action_argument_t = std::variant<
		std::monostate, Date, bool, int64_t, std::pair<uint64_t, bool>, std::pair<uint64_t, uint64_t>,
		std::pair<uint64_t, int64_t>, std::tuple<uint64_t, uint64_t, int64_t>
	>;

	// TODO - prevent this from catching anything that can be used to construct an argument
	// (currently causes this warning: 'argument': conversion from 'size_t' to 'OpenVic::Date::year_t', possible loss of data)

	std::ostream& operator<<(std::ostream& stream, game_action_argument_t const& argument);

	using game_action_t = std::pair<game_action_type_t, game_action_argument_t>;

	struct DefinitionManager;
	struct Bookmark;

	struct InstanceManager {
		using gamestate_updated_func_t = std::function<void()>;

	private:
		DefinitionManager const& PROPERTY(definition_manager);

		GameRulesManager const& game_rules_manager;
		GoodInstanceManager PROPERTY_REF(good_instance_manager);
		MarketInstance PROPERTY_REF(market_instance);
		ArtisanalProducerFactoryPattern artisanal_producer_factory_pattern;

		FlagStrings PROPERTY_REF(global_flags);

		CountryInstanceManager PROPERTY_REF(country_instance_manager);
		CountryRelationManager PROPERTY_REF(country_relation_manager);
		UnitInstanceManager PROPERTY_REF(unit_instance_manager);
		PoliticsInstanceManager PROPERTY_REF(politics_instance_manager);
		/* Near the end so it is freed after other managers that may depend on it,
		 * e.g. if we want to remove military units from the province they're in when they're destructed. */
		MapInstance PROPERTY_REF(map_instance);
		SimulationClock PROPERTY_REF(simulation_clock);
		ConsoleInstance PROPERTY_REF(console_instance);

		bool PROPERTY_CUSTOM_PREFIX(game_instance_setup, is, false);
		bool PROPERTY_CUSTOM_PREFIX(game_session_started, is, false);

		time_t session_start = 0; /* SS-54, as well as allowing time-tracking */
		Bookmark const* PROPERTY(bookmark, nullptr);
		Date PROPERTY(today);
		gamestate_updated_func_t gamestate_updated;
		std::vector<game_action_t> game_action_queue;
		bool gamestate_needs_update = false, currently_updating_gamestate = false, currently_executing_game_actions = false;

		void update_modifier_sums();
		void set_gamestate_needs_update();
		void update_gamestate();
		void tick(Date new_today);

		void execute_game_actions();

	public:
		InstanceManager(
			GameRulesManager const& new_game_rules_manager,
			DefinitionManager const& new_definition_manager,
			gamestate_updated_func_t gamestate_updated_callback
		);

		inline constexpr bool is_bookmark_loaded() const {
			return bookmark != nullptr;
		}

		bool setup();
		bool load_bookmark(Bookmark const* new_bookmark);
		bool start_game_session();
		bool update_clock();

		// TODO - maybe do this as a game action?
		bool set_today_and_update(Date new_today);

		bool queue_game_action(game_action_type_t type, game_action_argument_t&& argument);

	private:
		// Return value indicates whether a gamestate update is needed or not
		using game_action_callback_t = bool(InstanceManager::*)(game_action_argument_t const&);

		static std::array<
			game_action_callback_t, static_cast<size_t>(game_action_type_t::MAX_GAME_ACTION)
		> GAME_ACTION_CALLBACKS;

		bool execute_game_action(game_action_t const& game_action);

		bool game_action_callback_none(game_action_argument_t const& argument);

		// Core
		bool game_action_callback_tick(game_action_argument_t const& argument);
		bool game_action_callback_set_pause(game_action_argument_t const& argument);
		bool game_action_callback_set_speed(game_action_argument_t const& argument);
		bool game_action_callback_set_ai(game_action_argument_t const& argument);

		// Production
		bool game_action_callback_expand_province_building(game_action_argument_t const& argument);

		// Budget
		bool game_action_callback_set_strata_tax(game_action_argument_t const& argument);
		bool game_action_callback_set_land_spending(game_action_argument_t const& argument);
		bool game_action_callback_set_naval_spending(game_action_argument_t const& argument);
		bool game_action_callback_set_construction_spending(game_action_argument_t const& argument);
		bool game_action_callback_set_education_spending(game_action_argument_t const& argument);
		bool game_action_callback_set_administration_spending(game_action_argument_t const& argument);
		bool game_action_callback_set_social_spending(game_action_argument_t const& argument);
		bool game_action_callback_set_military_spending(game_action_argument_t const& argument);
		bool game_action_callback_set_tariff_rate(game_action_argument_t const& argument);

		// Technology
		bool game_action_callback_start_research(game_action_argument_t const& argument);

		// Politics

		// Population

		// Trade

		// Diplomacy

		// Military
		bool game_action_callback_create_general(game_action_argument_t const& argument);
		bool game_action_callback_create_admiral(game_action_argument_t const& argument);
		bool game_action_callback_set_use_leader(game_action_argument_t const& argument);
		bool game_action_callback_set_auto_create_leaders(game_action_argument_t const& argument);
		bool game_action_callback_set_auto_assign_leaders(game_action_argument_t const& argument);
		bool game_action_callback_set_mobilise(game_action_argument_t const& argument);
	};
}

#include "InstanceManager.hpp"

#include "openvic-simulation/DefinitionManager.hpp"
#include "openvic-simulation/console/ConsoleInstance.hpp"
#include "openvic-simulation/utility/Logger.hpp"

using namespace OpenVic;

struct game_action_argument_print_visitor_t {
	std::ostream& stream;

	void operator()(std::monostate) {
		stream << "monostate";
	}

	void operator()(bool b) {
		stream << (b ? "true" : "false");
	}

	template<typename T1, typename T2>
	void operator()(std::pair<T1, T2> const& x) {
		stream << "(";
		(*this)(x.first);
		stream << ", ";
		(*this)(x.second);
		stream << ")";
	}

	template<typename T1, typename T2, typename T3>
	void operator()(std::tuple<T1, T2, T3> const& x) {
		stream << "(";
		(*this)(std::get<0>(x));
		stream << ", ";
		(*this)(std::get<1>(x));
		stream << ", ";
		(*this)(std::get<2>(x));
		stream << ")";
	}

	void operator()(auto x) {
		stream << x;
	}
};

std::ostream& OpenVic::operator<<(std::ostream& stream, game_action_argument_t const& argument) {
	std::visit(game_action_argument_print_visitor_t { stream }, argument);

	return stream;
}

InstanceManager::InstanceManager(
	GameRulesManager const& new_game_rules_manager,
	DefinitionManager const& new_definition_manager,
	gamestate_updated_func_t gamestate_updated_callback
) : definition_manager { new_definition_manager },
	game_rules_manager { new_game_rules_manager },
	good_instance_manager { new_definition_manager.get_economy_manager().get_good_definition_manager() },
	market_instance { new_definition_manager.get_define_manager().get_country_defines(), good_instance_manager },
	artisanal_producer_factory_pattern {
		market_instance,
		new_definition_manager.get_modifier_manager().get_modifier_effect_cache(),
		new_definition_manager.get_economy_manager().get_production_type_manager()
	},
	global_flags { "global" },
	country_instance_manager { new_definition_manager.get_country_definition_manager() },
	unit_instance_manager {
		new_definition_manager.get_pop_manager().get_culture_manager(),
		new_definition_manager.get_military_manager().get_leader_trait_manager(),
		new_definition_manager.get_define_manager().get_military_defines()
	},
	politics_instance_manager { *this },
	map_instance { new_definition_manager.get_map_definition() },
	simulation_clock {
		[this]() -> void {
			queue_game_action(game_action_type_t::GAME_ACTION_TICK, today + 1);
		},
		[this]() -> void {
			execute_game_actions();
			update_gamestate();
		}
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
	if (currently_updating_gamestate) {
		Logger::error("Attempted to update gamestate while already updating gamestate!");
		return;
	}

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
void InstanceManager::tick(Date new_today) {
	today = new_today;

	Logger::info("Tick: ", today);

	// Tick...
	map_instance.map_tick(today);
	country_instance_manager.tick(*this);
	unit_instance_manager.tick();
	market_instance.execute_orders();

	if (today.is_month_start()) {
		market_instance.record_price_history();
	}
}

void InstanceManager::execute_game_actions() {
	if (currently_executing_game_actions) {
		Logger::error("Attempted to execute game actions while already executing game actions!");
		return;
	}

	if (game_action_queue.empty()) {
		return;
	}

	currently_executing_game_actions = true;

	// Temporary gamestate/UI update trigger, to be replaced with a more sophisticated targetted system
	bool needs_gamestate_update = false;

	for (game_action_t const& game_action : game_action_queue) {
		needs_gamestate_update |= execute_game_action(game_action);
	}

	game_action_queue.clear();

	if (needs_gamestate_update) {
		set_gamestate_needs_update();
	}

	currently_executing_game_actions = false;
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

bool InstanceManager::queue_game_action(game_action_type_t type, game_action_argument_t&& argument) {
	if (currently_executing_game_actions) {
		Logger::error("Attempted to queue a game action while already executing game actions!");
		return false;
	}

	if (type >= game_action_type_t::MAX_GAME_ACTION) {
		Logger::error("Invalid game action type ", static_cast<uint64_t>(type));
		return false;
	}

	game_action_queue.emplace_back(type, std::move(argument));
	return true;
}

std::array<
	InstanceManager::game_action_callback_t, static_cast<size_t>(game_action_type_t::MAX_GAME_ACTION)
> InstanceManager::GAME_ACTION_CALLBACKS {
	// GAME_ACTION_NONE
	&InstanceManager::game_action_callback_none,

	// Core
	// GAME_ACTION_TICK
	&InstanceManager::game_action_callback_tick,
	// GAME_ACTION_SET_PAUSE
	&InstanceManager::game_action_callback_set_pause,
	// GAME_ACTION_SET_SPEED
	&InstanceManager::game_action_callback_set_speed,
	// GAME_ACTION_SET_AI
	&InstanceManager::game_action_callback_set_ai,

	// Production
	// GAME_ACTION_EXPAND_PROVINCE_BUILDING
	&InstanceManager::game_action_callback_expand_province_building,

	// Budget
	// GAME_ACTION_SET_STRATA_TAX
	&InstanceManager::game_action_callback_set_strata_tax,
	// GAME_ACTION_SET_LAND_SPENDING
	&InstanceManager::game_action_callback_set_land_spending,
	// GAME_ACTION_SET_NAVAL_SPENDING
	&InstanceManager::game_action_callback_set_naval_spending,
	// GAME_ACTION_SET_CONSTRUCTION_SPENDING
	&InstanceManager::game_action_callback_set_construction_spending,
	// GAME_ACTION_SET_EDUCATION_SPENDING
	&InstanceManager::game_action_callback_set_education_spending,
	// GAME_ACTION_SET_ADMINISTRATION_SPENDING
	&InstanceManager::game_action_callback_set_administration_spending,
	// GAME_ACTION_SET_SOCIAL_SPENDING
	&InstanceManager::game_action_callback_set_social_spending,
	// GAME_ACTION_SET_MILITARY_SPENDING
	&InstanceManager::game_action_callback_set_military_spending,
	// GAME_ACTION_SET_TARIFF_RATE
	&InstanceManager::game_action_callback_set_tariff_rate,

	// Technology
	// GAME_ACTION_START_RESEARCH
	&InstanceManager::game_action_callback_start_research,

	// Politics

	// Population

	// Trade
	// GAME_ACTION_SET_GOOD_AUTOMATED
	&InstanceManager::game_action_callback_set_good_automated,

	// Diplomacy

	// Military
	// GAME_ACTION_CREATE_LEADER
	&InstanceManager::game_action_callback_create_leader,
	// GAME_ACTION_SET_USE_LEADER
	&InstanceManager::game_action_callback_set_use_leader,
	// GAME_ACTION_SET_AUTO_CREATE_LEADERS
	&InstanceManager::game_action_callback_set_auto_create_leaders,
	// GAME_ACTION_SET_AUTO_ASSIGN_LEADERS
	&InstanceManager::game_action_callback_set_auto_assign_leaders,
	// GAME_ACTION_SET_MOBILISE
	&InstanceManager::game_action_callback_set_mobilise
};

bool InstanceManager::execute_game_action(game_action_t const& game_action) {
	return (this->*GAME_ACTION_CALLBACKS[static_cast<size_t>(game_action.first)])(game_action.second);
}

bool InstanceManager::game_action_callback_none(game_action_argument_t const& argument) {
	if (OV_unlikely(!std::holds_alternative<std::monostate>(argument))) {
		Logger::error("GAME_ACTION_NONE called with invalid argument: ", argument);
	}

	return false;
}

// Core
bool InstanceManager::game_action_callback_tick(game_action_argument_t const& argument) {
	Date const* date = std::get_if<Date>(&argument);
	if (OV_unlikely(date == nullptr)) {
		Logger::error("GAME_ACTION_TICK called with invalid argument: ", argument);
		return false;
	}

	tick(*date);
	return true;
}

bool InstanceManager::game_action_callback_set_pause(game_action_argument_t const& argument) {
	bool const* pause = std::get_if<bool>(&argument);
	if (OV_unlikely(pause == nullptr)) {
		Logger::error("GAME_ACTION_SET_PAUSE called with invalid argument: ", argument);
		return false;
	}

	const bool old_pause = simulation_clock.is_paused();

	simulation_clock.set_paused(*pause);

	return old_pause != simulation_clock.is_paused();
}

bool InstanceManager::game_action_callback_set_speed(game_action_argument_t const& argument) {
	int64_t const* speed = std::get_if<int64_t>(&argument);
	if (OV_unlikely(speed == nullptr)) {
		Logger::error("GAME_ACTION_SET_SPEED called with invalid argument: ", argument);
		return false;
	}

	const SimulationClock::speed_t old_speed = simulation_clock.get_simulation_speed();

	simulation_clock.set_simulation_speed(*speed);

	return old_speed != simulation_clock.get_simulation_speed();
}

bool InstanceManager::game_action_callback_set_ai(game_action_argument_t const& argument) {
	std::pair<uint64_t, bool> const* country_ai = std::get_if<std::pair<uint64_t, bool>>(&argument);
	if (OV_unlikely(country_ai == nullptr)) {
		Logger::error("GAME_ACTION_SET_AI called with invalid argument: ", argument);
		return false;
	}

	CountryInstance* country = country_instance_manager.get_country_instance_by_index(country_ai->first);

	if (OV_unlikely(country == nullptr)) {
		Logger::error("GAME_ACTION_SET_AI called with invalid country index: ", country_ai->first);
		return false;
	}

	const bool old_ai = country->is_ai();

	country->set_ai(country_ai->second);

	return old_ai != country->is_ai();
}

// Production
bool InstanceManager::game_action_callback_expand_province_building(game_action_argument_t const& argument) {
	std::pair<uint64_t, uint64_t> const* province_building = std::get_if<std::pair<uint64_t, uint64_t>>(&argument);
	if (OV_unlikely(province_building == nullptr)) {
		Logger::error("GAME_ACTION_EXPAND_PROVINCE_BUILDING called with invalid argument: ", argument);
		return false;
	}

	ProvinceInstance* province = map_instance.get_province_instance_by_index(province_building->first);

	if (OV_unlikely(province == nullptr)) {
		Logger::error("GAME_ACTION_EXPAND_PROVINCE_BUILDING called with invalid province index: ", province_building->first);
		return false;
	}

	return province->expand_building(province_building->second);
}

// Budget
bool InstanceManager::game_action_callback_set_strata_tax(game_action_argument_t const& argument) {
	std::tuple<uint64_t, uint64_t, int64_t> const* country_strata_value =
		std::get_if<std::tuple<uint64_t, uint64_t, int64_t>>(&argument);
	if (OV_unlikely(country_strata_value == nullptr)) {
		Logger::error("GAME_ACTION_SET_STRATA_TAX called with invalid argument: ", argument);
		return false;
	}

	CountryInstance* country = country_instance_manager.get_country_instance_by_index(std::get<0>(*country_strata_value));

	if (OV_unlikely(country == nullptr)) {
		Logger::error("GAME_ACTION_SET_STRATA_TAX called with invalid country index: ", std::get<0>(*country_strata_value));
		return false;
	}

	Strata const* strata = definition_manager.get_pop_manager().get_strata_by_index(std::get<1>(*country_strata_value));

	if (OV_unlikely(strata == nullptr)) {
		Logger::error("GAME_ACTION_SET_STRATA_TAX called with invalid strata index: ", std::get<1>(*country_strata_value));
		return false;
	}

	CountryInstance::StandardSliderValue const& strata_tax_rate = country->get_tax_rate_by_strata()[*strata];

	const CountryInstance::StandardSliderValue::int_type old_tax_rate = strata_tax_rate.get_value();

	country->set_strata_tax_rate(*strata, std::get<2>(*country_strata_value));

	return old_tax_rate != strata_tax_rate.get_value();
}

bool InstanceManager::game_action_callback_set_land_spending(game_action_argument_t const& argument) {
	std::pair<uint64_t, int64_t> const* country_value = std::get_if<std::pair<uint64_t, int64_t>>(&argument);
	if (OV_unlikely(country_value == nullptr)) {
		Logger::error("GAME_ACTION_SET_LAND_SPENDING called with invalid argument: ", argument);
		return false;
	}

	CountryInstance* country = country_instance_manager.get_country_instance_by_index(country_value->first);

	if (OV_unlikely(country == nullptr)) {
		Logger::error("GAME_ACTION_SET_LAND_SPENDING called with invalid country index: ", country_value->first);
		return false;
	}

	const CountryInstance::StandardSliderValue::int_type old_land_spending = country->get_land_spending().get_value();

	country->set_land_spending(country_value->second);

	return old_land_spending != country->get_land_spending().get_value();
}

bool InstanceManager::game_action_callback_set_naval_spending(game_action_argument_t const& argument) {
	std::pair<uint64_t, int64_t> const* country_value = std::get_if<std::pair<uint64_t, int64_t>>(&argument);
	if (OV_unlikely(country_value == nullptr)) {
		Logger::error("GAME_ACTION_SET_NAVAL_SPENDING called with invalid argument: ", argument);
		return false;
	}

	CountryInstance* country = country_instance_manager.get_country_instance_by_index(country_value->first);

	if (OV_unlikely(country == nullptr)) {
		Logger::error("GAME_ACTION_SET_NAVAL_SPENDING called with invalid country index: ", country_value->first);
		return false;
	}

	const CountryInstance::StandardSliderValue::int_type old_naval_spending = country->get_naval_spending().get_value();

	country->set_naval_spending(country_value->second);

	return old_naval_spending != country->get_naval_spending().get_value();
}

bool InstanceManager::game_action_callback_set_construction_spending(game_action_argument_t const& argument) {
	std::pair<uint64_t, int64_t> const* country_value = std::get_if<std::pair<uint64_t, int64_t>>(&argument);
	if (OV_unlikely(country_value == nullptr)) {
		Logger::error("GAME_ACTION_SET_CONSTRUCTION_SPENDING called with invalid argument: ", argument);
		return false;
	}

	CountryInstance* country = country_instance_manager.get_country_instance_by_index(country_value->first);

	if (OV_unlikely(country == nullptr)) {
		Logger::error("GAME_ACTION_SET_CONSTRUCTION_SPENDING called with invalid country index: ", country_value->first);
		return false;
	}

	const CountryInstance::StandardSliderValue::int_type old_construction_spending =
		country->get_construction_spending().get_value();

	country->set_construction_spending(country_value->second);

	return old_construction_spending != country->get_construction_spending().get_value();
}

bool InstanceManager::game_action_callback_set_education_spending(game_action_argument_t const& argument) {
	std::pair<uint64_t, int64_t> const* country_value = std::get_if<std::pair<uint64_t, int64_t>>(&argument);
	if (OV_unlikely(country_value == nullptr)) {
		Logger::error("GAME_ACTION_SET_EDUCATION_SPENDING called with invalid argument: ", argument);
		return false;
	}

	CountryInstance* country = country_instance_manager.get_country_instance_by_index(country_value->first);

	if (OV_unlikely(country == nullptr)) {
		Logger::error("GAME_ACTION_SET_EDUCATION_SPENDING called with invalid country index: ", country_value->first);
		return false;
	}

	const CountryInstance::StandardSliderValue::int_type old_education_spending =
		country->get_education_spending().get_value();

	country->set_education_spending(country_value->second);

	return old_education_spending != country->get_education_spending().get_value();
}

bool InstanceManager::game_action_callback_set_administration_spending(game_action_argument_t const& argument) {
	std::pair<uint64_t, int64_t> const* country_value = std::get_if<std::pair<uint64_t, int64_t>>(&argument);
	if (OV_unlikely(country_value == nullptr)) {
		Logger::error("GAME_ACTION_SET_ADMINISTRATION_SPENDING called with invalid argument: ", argument);
		return false;
	}

	CountryInstance* country = country_instance_manager.get_country_instance_by_index(country_value->first);

	if (OV_unlikely(country == nullptr)) {
		Logger::error("GAME_ACTION_SET_ADMINISTRATION_SPENDING called with invalid country index: ", country_value->first);
		return false;
	}

	const CountryInstance::StandardSliderValue::int_type old_administration_spending =
		country->get_administration_spending().get_value();

	country->set_administration_spending(country_value->second);

	return old_administration_spending != country->get_administration_spending().get_value();
}

bool InstanceManager::game_action_callback_set_social_spending(game_action_argument_t const& argument) {
	std::pair<uint64_t, int64_t> const* country_value = std::get_if<std::pair<uint64_t, int64_t>>(&argument);
	if (OV_unlikely(country_value == nullptr)) {
		Logger::error("GAME_ACTION_SET_SOCIAL_SPENDING called with invalid argument: ", argument);
		return false;
	}

	CountryInstance* country = country_instance_manager.get_country_instance_by_index(country_value->first);

	if (OV_unlikely(country == nullptr)) {
		Logger::error("GAME_ACTION_SET_SOCIAL_SPENDING called with invalid country index: ", country_value->first);
		return false;
	}

	const CountryInstance::StandardSliderValue::int_type old_social_spending = country->get_social_spending().get_value();

	country->set_social_spending(country_value->second);

	return old_social_spending != country->get_social_spending().get_value();
}

bool InstanceManager::game_action_callback_set_military_spending(game_action_argument_t const& argument) {
	std::pair<uint64_t, int64_t> const* country_value = std::get_if<std::pair<uint64_t, int64_t>>(&argument);
	if (OV_unlikely(country_value == nullptr)) {
		Logger::error("GAME_ACTION_SET_MILITARY_SPENDING called with invalid argument: ", argument);
		return false;
	}

	CountryInstance* country = country_instance_manager.get_country_instance_by_index(country_value->first);

	if (OV_unlikely(country == nullptr)) {
		Logger::error("GAME_ACTION_SET_MILITARY_SPENDING called with invalid country index: ", country_value->first);
		return false;
	}

	const CountryInstance::StandardSliderValue::int_type old_military_spending = country->get_military_spending().get_value();

	country->set_military_spending(country_value->second);

	return old_military_spending != country->get_military_spending().get_value();
}

bool InstanceManager::game_action_callback_set_tariff_rate(game_action_argument_t const& argument) {
	std::pair<uint64_t, int64_t> const* country_value = std::get_if<std::pair<uint64_t, int64_t>>(&argument);
	if (OV_unlikely(country_value == nullptr)) {
		Logger::error("GAME_ACTION_SET_TARIFF_RATE called with invalid argument: ", argument);
		return false;
	}

	CountryInstance* country = country_instance_manager.get_country_instance_by_index(country_value->first);

	if (OV_unlikely(country == nullptr)) {
		Logger::error("GAME_ACTION_SET_TARIFF_RATE called with invalid country index: ", country_value->first);
		return false;
	}

	const CountryInstance::TariffSliderValue::int_type old_tariff_rate = country->get_tariff_rate().get_value();

	country->set_tariff_rate(country_value->second);

	return old_tariff_rate != country->get_tariff_rate().get_value();
}

// Technology
bool InstanceManager::game_action_callback_start_research(game_action_argument_t const& argument) {
	std::pair<uint64_t, uint64_t> const* country_tech = std::get_if<std::pair<uint64_t, uint64_t>>(&argument);
	if (OV_unlikely(country_tech == nullptr)) {
		Logger::error("GAME_ACTION_START_RESEARCH called with invalid argument: ", argument);
		return false;
	}

	CountryInstance* country = country_instance_manager.get_country_instance_by_index(country_tech->first);

	if (OV_unlikely(country == nullptr)) {
		Logger::error("GAME_ACTION_START_RESEARCH called with invalid country index: ", country_tech->first);
		return false;
	}

	Technology const* technology =
		definition_manager.get_research_manager().get_technology_manager().get_technology_by_index(country_tech->second);

	if (OV_unlikely(technology == nullptr)) {
		Logger::error("GAME_ACTION_START_RESEARCH called with invalid technology index: ", country_tech->second);
		return false;
	}

	Technology const* old_research = country->get_current_research();

	country->start_research(*technology, *this);

	return old_research != country->get_current_research();
}

// Politics

// Population

// Trade
bool InstanceManager::game_action_callback_set_good_automated(game_action_argument_t const& argument) {
	std::tuple<uint64_t, uint64_t, bool> const* country_good_automated =
		std::get_if<std::tuple<uint64_t, uint64_t, bool>>(&argument);
	if (OV_unlikely(country_good_automated == nullptr)) {
		Logger::error("GAME_ACTION_SET_GOOD_AUTOMATED called with invalid argument: ", argument);
		return false;
	}

	CountryInstance* country = country_instance_manager.get_country_instance_by_index(std::get<0>(*country_good_automated));

	if (OV_unlikely(country == nullptr)) {
		Logger::error(
			"GAME_ACTION_SET_GOOD_AUTOMATED called with invalid country index: ", std::get<0>(*country_good_automated)
		);
		return false;
	}

	GoodInstance const* good = good_instance_manager.get_good_instance_by_index(std::get<1>(*country_good_automated));

	if (OV_unlikely(good == nullptr)) {
		Logger::error("GAME_ACTION_SET_GOOD_AUTOMATED called with invalid good index: ", std::get<1>(*country_good_automated));
		return false;
	}

	CountryInstance::good_data_t& good_data = country->get_good_data(*good);

	const bool old_automated = good_data.is_automated;

	good_data.is_automated = std::get<2>(*country_good_automated);

	return old_automated != good_data.is_automated;
}

// Diplomacy

// Military
bool InstanceManager::game_action_callback_create_leader(game_action_argument_t const& argument) {
	std::pair<uint64_t, bool> const* country_branch = std::get_if<std::pair<uint64_t, bool>>(&argument);
	if (OV_unlikely(country_branch == nullptr)) {
		Logger::error("GAME_ACTION_CREATE_LEADER called with invalid argument: ", argument);
		return false;
	}

	CountryInstance* country = country_instance_manager.get_country_instance_by_index(country_branch->first);

	if (OV_unlikely(country == nullptr)) {
		Logger::error("GAME_ACTION_CREATE_LEADER called with invalid country index: ", country_branch->first);
		return false;
	}

	if (country->get_create_leader_count() < 1) {
		Logger::error(
			"GAME_ACTION_CREATE_LEADER called for country \"", country->get_identifier(),
			"\" without enough leadership points (", country->get_leadership_point_stockpile().to_string(2),
			") to create any leaders!"
		);
		return false;
	}

	return unit_instance_manager.create_leader(
		*country,
		country_branch->second ? UnitType::branch_t::LAND : UnitType::branch_t::NAVAL,
		today
	);
}

bool InstanceManager::game_action_callback_set_use_leader(game_action_argument_t const& argument) {
	std::pair<uint64_t, bool> const* leader_use = std::get_if<std::pair<uint64_t, bool>>(&argument);
	if (OV_unlikely(leader_use == nullptr)) {
		Logger::error("GAME_ACTION_SET_USE_LEADER called with invalid argument: ", argument);
		return false;
	}

	LeaderInstance* leader = unit_instance_manager.get_leader_instance_by_unique_id(leader_use->first);

	if (OV_unlikely(leader == nullptr)) {
		Logger::error("GAME_ACTION_SET_USE_LEADER called with invalid leader index: ", leader_use->first);
		return false;
	}

	const bool old_use = leader->get_can_be_used();

	leader->set_can_be_used(leader_use->second);

	return old_use != leader->get_can_be_used();
}

bool InstanceManager::game_action_callback_set_auto_create_leaders(game_action_argument_t const& argument) {
	std::pair<uint64_t, bool> const* country_value = std::get_if<std::pair<uint64_t, bool>>(&argument);
	if (OV_unlikely(country_value == nullptr)) {
		Logger::error("GAME_ACTION_SET_AUTO_CREATE_LEADERS called with invalid argument: ", argument);
		return false;
	}

	CountryInstance* country = country_instance_manager.get_country_instance_by_index(country_value->first);

	if (OV_unlikely(country == nullptr)) {
		Logger::error("GAME_ACTION_SET_AUTO_CREATE_LEADERS called with invalid country index: ", country_value->first);
		return false;
	}

	const bool old_auto_create = country->get_auto_create_leaders();

	country->set_auto_create_leaders(country_value->second);

	return old_auto_create != country->get_auto_create_leaders();
}

bool InstanceManager::game_action_callback_set_auto_assign_leaders(game_action_argument_t const& argument) {
	std::pair<uint64_t, bool> const* country_value = std::get_if<std::pair<uint64_t, bool>>(&argument);
	if (OV_unlikely(country_value == nullptr)) {
		Logger::error("GAME_ACTION_SET_AUTO_ASSIGN_LEADERS called with invalid argument: ", argument);
		return false;
	}

	CountryInstance* country = country_instance_manager.get_country_instance_by_index(country_value->first);

	if (OV_unlikely(country == nullptr)) {
		Logger::error("GAME_ACTION_SET_AUTO_ASSIGN_LEADERS called with invalid country index: ", country_value->first);
		return false;
	}

	const bool old_auto_assign = country->get_auto_assign_leaders();

	country->set_auto_assign_leaders(country_value->second);

	return old_auto_assign != country->get_auto_assign_leaders();
}

bool InstanceManager::game_action_callback_set_mobilise(game_action_argument_t const& argument) {
	std::pair<uint64_t, bool> const* country_mobilise = std::get_if<std::pair<uint64_t, bool>>(&argument);
	if (OV_unlikely(country_mobilise == nullptr)) {
		Logger::error("GAME_ACTION_SET_MOBILISE called with invalid argument: ", argument);
		return false;
	}

	CountryInstance* country = country_instance_manager.get_country_instance_by_index(country_mobilise->first);

	if (OV_unlikely(country == nullptr)) {
		Logger::error("GAME_ACTION_SET_MOBILISE called with invalid country index: ", country_mobilise->first);
		return false;
	}

	const bool old_mobilise = country->is_mobilised();

	country->set_mobilised(country_mobilise->second);

	return old_mobilise != country->is_mobilised();
}

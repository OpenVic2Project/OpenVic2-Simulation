#include "Building.hpp"

#include <cassert>

#include "openvic-simulation/map/Province.hpp"
#include "openvic-simulation/utility/Logger.hpp"

using namespace OpenVic;

Building::Building(BuildingType const& new_type)
	: HasIdentifier { new_type.get_identifier() },
	  type { new_type } {}

bool Building::_can_expand() const {
	return level < type.get_max_level();
}

BuildingType const& Building::get_type() const {
	return type;
}

Building::level_t Building::get_level() const {
	return level;
}

Building::ExpansionState Building::get_expansion_state() const {
	return expansion_state;
}

Date const& Building::get_start_date() const {
	return start;
}

Date const& Building::get_end_date() const {
	return end;
}

float Building::get_expansion_progress() const {
	return expansion_progress;
}

bool Building::expand() {
	if (expansion_state == ExpansionState::CanExpand) {
		expansion_state = ExpansionState::Preparing;
		expansion_progress = 0.0f;
		return true;
	}
	return false;
}

/* REQUIREMENTS:
 * MAP-71, MAP-74, MAP-77
 */
void Building::update_state(Date const& today) {
	switch (expansion_state) {
		case ExpansionState::Preparing:
			start = today;
			end = start + type.get_build_time();
			break;
		case ExpansionState::Expanding:
			expansion_progress = static_cast<double>(today - start) / static_cast<double>(end - start);
			break;
		default: expansion_state = _can_expand() ? ExpansionState::CanExpand : ExpansionState::CannotExpand;
	}
}

void Building::tick(Date const& today) {
	if (expansion_state == ExpansionState::Preparing) {
		expansion_state = ExpansionState::Expanding;
	}
	if (expansion_state == ExpansionState::Expanding) {
		if (end <= today) {
			level++;
			expansion_state = ExpansionState::CannotExpand;
		}
	}
}

BuildingType::BuildingType(const std::string_view new_identifier, Building::level_t new_max_level, Timespan new_build_time)
	: HasIdentifier { new_identifier },
	  max_level { new_max_level },
	  build_time { new_build_time } {
	assert(max_level >= 0);
	assert(build_time >= 0);
}

Building::level_t BuildingType::get_max_level() const {
	return max_level;
}

Timespan BuildingType::get_build_time() const {
	return build_time;
}

BuildingManager::BuildingManager() : building_types { "building types" } {}

bool BuildingManager::add_building_type(const std::string_view identifier, Building::level_t max_level, Timespan build_time) {
	if (identifier.empty()) {
		Logger::error("Invalid building type identifier - empty!");
		return false;
	}
	if (max_level < 0) {
		Logger::error("Invalid building type max level for ", identifier, ": ", max_level);
		return false;
	}
	if (build_time < 0) {
		Logger::error("Invalid building type build time for ", identifier, ": ", build_time);
		return false;
	}
	return building_types.add_item({ identifier, max_level, build_time });
}

bool BuildingManager::generate_province_buildings(Province& province) const {
	province.reset_buildings();
	if (!building_types.is_locked()) {
		Logger::error("Cannot generate buildings until building types are locked!");
		return false;
	}
	bool ret = true;
	if (!province.get_water()) {
		for (BuildingType const& type : building_types.get_items()) {
			ret &= province.add_building({ type });
		}
	}
	province.lock_buildings();
	return ret;
}

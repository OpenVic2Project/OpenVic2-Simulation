#include "Province.hpp"

#include <cassert>
#include <iomanip>
#include <sstream>

using namespace OpenVic;

Province::Province(const std::string_view new_identifier, colour_t new_colour, index_t new_index)
	: HasIdentifierAndColour { new_identifier, new_colour, false },
	  index { new_index },
	  buildings { "buildings" } {
	assert(index != NULL_INDEX);
}

Province::index_t Province::get_index() const {
	return index;
}

Region* Province::get_region() const {
	return region;
}

bool Province::is_water() const {
	return water;
}

Province::life_rating_t Province::get_life_rating() const {
	return life_rating;
}

return_t Province::add_building(Building&& building) {
	return buildings.add_item(std::move(building));
}

void Province::lock_buildings() {
	buildings.lock(false);
}

void Province::reset_buildings() {
	buildings.reset();
}

Building const* Province::get_building_by_identifier(const std::string_view identifier) const {
	return buildings.get_item_by_identifier(identifier);
}

std::vector<Building> const& Province::get_buildings() const {
	return buildings.get_items();
}

return_t Province::expand_building(const std::string_view building_type_identifier) {
	Building* building = buildings.get_item_by_identifier(building_type_identifier);
	if (building == nullptr) return FAILURE;
	return building->expand();
}

Good const* Province::get_rgo() const {
	return rgo;
}

std::string Province::to_string() const {
	std::stringstream stream;
	stream << "(#" << std::to_string(index) << ", " << get_identifier() << ", 0x" << colour_to_hex_string() << ")";
	return stream.str();
}

return_t Province::add_pop(Pop&& pop) {
	if (!is_water()) {
		pops.push_back(std::move(pop));
		return SUCCESS;
	} else {
		Logger::error("Trying to add pop to water province ", get_identifier());
		return FAILURE;
	}
}

void Province::clear_pops() {
	pops.clear();
}

Pop::pop_size_t Province::get_total_population() const {
	return total_population;
}

distribution_t const& Province::get_pop_type_distribution() const {
	return pop_types;
}

distribution_t const& Province::get_culture_distribution() const {
	return cultures;
}

distribution_t const& Province::get_religion_distribution() const {
	return religions;
}

/* REQUIREMENTS:
 * MAP-65
 */
void Province::update_pops() {
	total_population = 0;
	pop_types.clear();
	cultures.clear();
	religions.clear();
	for (Pop const& pop : pops) {
		total_population += pop.get_size();
		pop_types[&pop.get_type()] += pop.get_size();
		cultures[&pop.get_culture()] += pop.get_size();
		religions[&pop.get_religion()] += pop.get_size();
	}
}

void Province::update_state(Date const& today) {
	for (Building& building : buildings.get_items())
		building.update_state(today);
	update_pops();
}

void Province::tick(Date const& today) {
	for (Building& building : buildings.get_items())
		building.tick(today);
}

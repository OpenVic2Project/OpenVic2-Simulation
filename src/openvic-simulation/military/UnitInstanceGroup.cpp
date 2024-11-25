#include "UnitInstanceGroup.hpp"

#include <vector>

#include "openvic-simulation/country/CountryInstance.hpp"
#include "openvic-simulation/map/MapInstance.hpp"
#include "openvic-simulation/map/ProvinceInstance.hpp"
#include "openvic-simulation/military/Deployment.hpp"

using namespace OpenVic;

using enum UnitType::branch_t;

template<UnitType::branch_t Branch>
UnitInstanceGroup<Branch>::UnitInstanceGroup(
	std::string_view new_name, std::vector<_UnitInstance*>&& new_units
) : name { new_name },
	units { std::move(new_units) },
	leader { nullptr },
	position { nullptr },
	country { nullptr } {}

template<UnitType::branch_t Branch>
size_t UnitInstanceGroup<Branch>::get_unit_count() const {
	return units.size();
}

template<UnitType::branch_t Branch>
bool UnitInstanceGroup<Branch>::empty() const {
	return units.empty();
}

template<UnitType::branch_t Branch>
size_t UnitInstanceGroup<Branch>::get_unit_category_count(UnitType::unit_category_t unit_category) const {
	return std::count_if(units.begin(), units.end(), [unit_category](_UnitInstance const* unit) {
		return unit->get_unit_type().get_unit_category() == unit_category;
	});
}

template<UnitType::branch_t Branch>
UnitType const* UnitInstanceGroup<Branch>::get_display_unit_type() const {
	if (units.empty()) {
		return nullptr;
	}

	fixed_point_map_t<UnitType const*> weighted_unit_types;

	for (_UnitInstance const* unit : units) {
		UnitType const& unit_type = unit->get_unit_type();
		weighted_unit_types[&unit_type] += unit_type.get_weighted_value();
	}

	return get_largest_item_tie_break(
		weighted_unit_types,
		[](UnitType const* lhs, UnitType const* rhs) -> bool {
			return lhs->get_weighted_value() < rhs->get_weighted_value();
		}
	)->first;
}

template<UnitType::branch_t Branch>
void UnitInstanceGroup<Branch>::set_name(std::string_view new_name) {
	name = new_name;
}

template<UnitType::branch_t Branch>
bool UnitInstanceGroup<Branch>::set_position(ProvinceInstance* new_position) {
	bool ret = true;

	if (position != new_position) {
		if (position != nullptr) {
			ret &= position->remove_unit_instance_group(*this);
		}

		position = new_position;

		if (position != nullptr) {
			ret &= position->add_unit_instance_group(*this);
		}
	}

	return ret;
}

template<UnitType::branch_t Branch>
bool UnitInstanceGroup<Branch>::set_country(CountryInstance* new_country) {
	bool ret = true;

	if (country != new_country) {
		if (country != nullptr) {
			ret &= country->remove_unit_instance_group(*this);
		}

		country = new_country;

		if (country != nullptr) {
			ret &= country->add_unit_instance_group(*this);
		}
	}

	return ret;
}

template<UnitType::branch_t Branch>
bool UnitInstanceGroup<Branch>::set_leader(_Leader* new_leader) {
	bool ret = true;

	if (leader != new_leader) {
		if (leader != nullptr) {
			if (leader->unit_instance_group == this) {
				leader->unit_instance_group = nullptr;
			} else {
				Logger::error(
					"Mismatch between leader and unit instance group: group ", name, " has leader ",
					leader->get_name(), " but the leader has group ", leader->get_unit_instance_group() != nullptr
						? leader->get_unit_instance_group()->get_name() : "NULL"
				);
				ret = false;
			}
		}

		leader = new_leader;

		if (leader != nullptr) {
			if (leader->unit_instance_group != nullptr) {
				if (leader->unit_instance_group != this) {
					ret &= leader->unit_instance_group->set_leader(nullptr);
				} else {
					Logger::error("Leader ", leader->get_name(), " already leads group ", name, "!");
					ret = false;
				}
			}

			leader->unit_instance_group = static_cast<UnitInstanceGroupBranched<Branch>*>(this);
		}
	}

	return ret;
}

template<UnitType::branch_t Branch>
bool UnitInstanceGroup<Branch>::path_to(
	ProvinceInstance const& target_province, bool continue_movement, MapInstance const& map
) {
	using adjacency_t = ProvinceDefinition::adjacency_t;
	using distance_t = ProvinceDefinition::distance_t;

	ProvinceInstance const* start_province = continue_movement && !path.empty() ? path.back() : position;

	if (start_province == nullptr) {
		Logger::error("Unit group ", name, " has no position to start movement from!");
		return false;
	}

	// Examples of how to use adjacency_t
	std::vector<adjacency_t> const& adjacencies = target_province.get_province_definition().get_adjacencies();

	bool path_found = false;
	adjacency_t const* shorted_adjacency = nullptr;
	distance_t shortest_distance = fixed_point_t::_0();

	for (adjacency_t const& adjacency : adjacencies) {
		// Skip non-land/non-water adjacencies for LAND/NAVAL unit groups
		if constexpr (Branch == LAND) {
			if (adjacency.get_type() != adjacency_t::type_t::LAND) {
				continue;
			}
		} else if constexpr (Branch == NAVAL) {
			if (adjacency.get_type() != adjacency_t::type_t::WATER) {
				continue;
			}
		}

		if (adjacency.get_distance() < shortest_distance || shorted_adjacency == nullptr) {
			shorted_adjacency = &adjacency;
			shortest_distance = adjacency.get_distance();
		}

		// You can go from ProvinceInstance to ProvinceDefinition using get_province_definition()
		// To go the other way you must use map.get_province_instance_from_definition(province_definition)
		if (adjacency.get_to() == &target_province.get_province_definition()) {
			path_found = true;
			break;
		}
	}

	if (path_found) {
		if (!continue_movement) {
			path.clear();
			movement_progress = fixed_point_t::_0();
		}

		// TODO - add new provinces to path (everything after start_province up to and including &target_province)

		return true;
	} else {
		return false;
	}
}

template struct OpenVic::UnitInstanceGroup<LAND>;
template struct OpenVic::UnitInstanceGroup<NAVAL>;

UnitInstanceGroupBranched<LAND>::UnitInstanceGroupBranched(
	std::string_view new_name,
	std::vector<RegimentInstance*>&& new_units
) : UnitInstanceGroup { new_name, std::move(new_units) } {}

UnitInstanceGroupBranched<NAVAL>::UnitInstanceGroupBranched(
	std::string_view new_name,
	std::vector<ShipInstance*>&& new_units
) : UnitInstanceGroup { new_name, std::move(new_units) } {}

fixed_point_t UnitInstanceGroupBranched<NAVAL>::get_total_consumed_supply() const {
	fixed_point_t total_consumed_supply = 0;

	for (ShipInstance const* ship : get_units()) {
		total_consumed_supply += ship->get_unit_type().get_supply_consumption_score();
	}

	return total_consumed_supply;
}

template<UnitType::branch_t Branch>
bool UnitInstanceManager::generate_unit_instance(
	UnitDeployment<Branch> const& unit_deployment, UnitInstanceBranched<Branch>*& unit_instance
) {
	unit_instance = &*get_unit_instances<Branch>().insert(
		[&unit_deployment]() -> UnitInstanceBranched<Branch> {
			if constexpr (Branch == LAND) {
				return {
					unit_deployment.get_name(), unit_deployment.get_type(),
					nullptr, // TODO - get pop from Province unit_deployment.get_home()
					false // Not mobilised
				};
			} else if constexpr (Branch == NAVAL) {
				return { unit_deployment.get_name(), unit_deployment.get_type() };
			}
		}()
	);

	return true;
}

template<UnitType::branch_t Branch>
bool UnitInstanceManager::generate_unit_instance_group(
	MapInstance& map_instance, CountryInstance& country, UnitDeploymentGroup<Branch> const& unit_deployment_group
) {
	if (unit_deployment_group.get_units().empty()) {
		Logger::error(
			"Trying to generate unit group \"", unit_deployment_group.get_name(), "\" with no units for country \"",
			country.get_identifier(), "\""
		);
		return false;
	}

	if (unit_deployment_group.get_location() == nullptr) {
		Logger::error(
			"Trying to generate unit group \"", unit_deployment_group.get_name(), "\" with no location for country \"",
			country.get_identifier(), "\""
		);
		return false;
	}

	bool ret = true;

	std::vector<UnitInstanceBranched<Branch>*> unit_instances;

	for (UnitDeployment<Branch> const& unit_deployment : unit_deployment_group.get_units()) {
		UnitInstanceBranched<Branch>* unit_instance = nullptr;

		ret &= generate_unit_instance(unit_deployment, unit_instance);

		if (unit_instance != nullptr) {
			unit_instances.push_back(unit_instance);
		}
	}

	if (unit_instances.empty()) {
		Logger::error(
			"Failed to generate any units for unit group \"", unit_deployment_group.get_name(), "\" for country \"",
			country.get_identifier(), "\""
		);
		return false;
	}

	UnitInstanceGroupBranched<Branch>& unit_instance_group = *get_unit_instance_groups<Branch>().insert({
		unit_deployment_group.get_name(), std::move(unit_instances)
	});

	ret &= unit_instance_group.set_position(
		&map_instance.get_province_instance_from_definition(*unit_deployment_group.get_location())
	);
	ret &= unit_instance_group.set_country(&country);

	return ret;
}

bool UnitInstanceManager::generate_deployment(
	MapInstance& map_instance, CountryInstance& country, Deployment const* deployment
) {
	if (deployment == nullptr) {
		Logger::error("Trying to generate null deployment for ", country.get_identifier());
		return false;
	}

	bool ret = true;

	const auto generate_group = [&]<UnitType::branch_t Branch>() -> void {
		for (UnitDeploymentGroup<Branch> const& unit_deployment_group : deployment->get_unit_deployment_groups<Branch>()) {
			ret &= generate_unit_instance_group(map_instance, country, unit_deployment_group);
		}
	};

	generate_group.template operator()<LAND>();
	generate_group.template operator()<NAVAL>();

	for (LeaderBase const& leader : deployment->get_leaders()) {
		switch (leader.get_branch()) {
		case LAND:
			country.add_leader<LAND>({ leader });
			break;
		case NAVAL:
			country.add_leader<NAVAL>({ leader });
			break;
		default:
			Logger::error(
				"Invalid branch ", static_cast<uint64_t>(leader.get_branch()), " for leader \"", leader.get_name(),
				"\", cannot add to country ", country.get_identifier()
			);
			ret = false;
		}
	}

	return ret;
}

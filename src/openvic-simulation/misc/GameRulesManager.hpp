#pragma once

#include "openvic-simulation/utility/Getters.hpp"
namespace OpenVic {
	enum struct demand_category : uint8_t {
		None,
		PopNeeds,
		FactoryNeeds
	};

	struct GameRulesManager {
	private:
		bool PROPERTY_RW(use_simple_farm_mine_logic, false);
		// if changed during a session, call on_use_exponential_price_changes_changed for each GoodInstance.
		bool PROPERTY_RW(use_exponential_price_changes, false);
		demand_category PROPERTY_RW(artisanal_input_demand_category, demand_category::None);

	public:
		constexpr bool get_use_optimal_pricing() const {
			return use_exponential_price_changes;
		}
	};
}
#pragma once

#include "openvic-simulation/country/CountryDefinition.hpp"
#include "openvic-simulation/pop/PopType.hpp"

namespace OpenVic {
	struct DefineManager;
	struct CountryInstance;
	struct ProvinceInstance;

	struct PopBase {
		friend PopManager;

	protected:
		PopType const* PROPERTY_ACCESS(type, protected);
		Culture const& PROPERTY_ACCESS(culture, protected);
		Religion const& PROPERTY_ACCESS(religion, protected);
		pop_size_t PROPERTY_ACCESS(size, protected);
		fixed_point_t PROPERTY_RW_ACCESS(militancy, protected);
		fixed_point_t PROPERTY_RW_ACCESS(consciousness, protected);
		RebelType const* PROPERTY_ACCESS(rebel_type, protected);

		PopBase(
			PopType const& new_type, Culture const& new_culture, Religion const& new_religion, pop_size_t new_size,
			fixed_point_t new_militancy, fixed_point_t new_consciousness, RebelType const* new_rebel_type
		);
	};

	#define DO_FOR_ALL_TYPES_OF_POP_INCOME(F)\
		F(rgo_owner_income)\
		F(rgo_worker_income)\
		F(artisanal_income)\
		F(factory_worker_income)\
		F(factory_owner_income)\
		F(unemployment_subsidies)\
		F(pensions)\
		F(government_salary_administration)\
		F(government_salary_education)\
		F(government_salary_military)\
		F(event_and_decision_income)\
		F(loan_interest_payments)

	#define DECLARE_POP_INCOME_STORES(income_type)\
		fixed_point_t PROPERTY(income_type);

	#define DECLARE_POP_INCOME_STORE_FUNCTIONS(name)\
		void add_##name(const fixed_point_t pop_income);

	/* REQUIREMENTS:
	 * POP-18, POP-19, POP-20, POP-21, POP-34, POP-35, POP-36, POP-37
	 */
	struct Pop : PopBase {
		friend struct ProvinceInstance;

		static constexpr pop_size_t MAX_SIZE = std::numeric_limits<pop_size_t>::max();

	private:
		ProvinceInstance const* PROPERTY(location);

		/* Last day's size change by source. */
		pop_size_t PROPERTY(total_change);
		pop_size_t PROPERTY(num_grown);
		pop_size_t PROPERTY(num_promoted); // TODO - detailed promotion/demotion info (what to)
		pop_size_t PROPERTY(num_demoted);
		pop_size_t PROPERTY(num_migrated_internal); // TODO - detailed migration info (where to)
		pop_size_t PROPERTY(num_migrated_external);
		pop_size_t PROPERTY(num_migrated_colonial);

		fixed_point_t PROPERTY_RW(literacy);

		// All of these should have a total size equal to the pop size, allowing the distributions from different pops to be
		// added together with automatic weighting based on their relative sizes. Similarly, the province, state and country
		// equivalents of these distributions will have a total size equal to their total population size.
		IndexedMap<Ideology, fixed_point_t> PROPERTY(ideology_distribution);
		fixed_point_map_t<Issue const*> PROPERTY(issue_distribution);
		IndexedMap<CountryParty, fixed_point_t> PROPERTY(vote_distribution);

		fixed_point_t PROPERTY(unemployment);
		fixed_point_t PROPERTY(cash);
		fixed_point_t PROPERTY(income);
		fixed_point_t PROPERTY(expenses);
		fixed_point_t PROPERTY(savings);
		fixed_point_t PROPERTY(life_needs_fulfilled);
		fixed_point_t PROPERTY(everyday_needs_fulfilled);
		fixed_point_t PROPERTY(luxury_needs_fulfilled);

		DO_FOR_ALL_TYPES_OF_POP_INCOME(DECLARE_POP_INCOME_STORES);
		#undef DECLARE_POP_INCOME_STORES

		size_t PROPERTY(max_supported_regiments);

		Pop(PopBase const& pop_base, decltype(ideology_distribution)::keys_type const& ideology_keys);

	public:
		Pop(Pop const&) = delete;
		Pop(Pop&&) = default;
		Pop& operator=(Pop const&) = delete;
		Pop& operator=(Pop&&) = delete;

		void setup_pop_test_values(IssueManager const& issue_manager);
		void generate_political_distributions(InstanceManager const& instance_manager);
		bool convert_to_equivalent();

		void set_location(ProvinceInstance const& new_location);
		void update_location_based_attributes();

		// The values returned by these functions are scaled by pop size, so they must be divided by pop size to get
		// the support as a proportion of 1.0
		fixed_point_t get_ideology_support(Ideology const& ideology) const;
		fixed_point_t get_issue_support(Issue const& issue) const;
		fixed_point_t get_party_support(CountryParty const& party) const;

		void update_gamestate(
			InstanceManager const& instance_manager, CountryInstance const* owner,
			const fixed_point_t pop_size_per_regiment_multiplier
		);

		DO_FOR_ALL_TYPES_OF_POP_INCOME(DECLARE_POP_INCOME_STORE_FUNCTIONS)
		#undef DECLARE_POP_INCOME_STORE_FUNCTIONS
		void clear_all_income();

	};
}
#ifndef KEEP_DO_FOR_ALL_TYPES_OF_INCOME
	#undef DO_FOR_ALL_TYPES_OF_POP_INCOME
#endif

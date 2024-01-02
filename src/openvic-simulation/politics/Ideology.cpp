#include "Ideology.hpp"

#include "openvic-simulation/types/Colour.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

IdeologyGroup::IdeologyGroup(std::string_view new_identifier) : HasIdentifier { new_identifier } {}

Ideology::Ideology(
	std::string_view new_identifier, colour_t new_colour, IdeologyGroup const& new_group, bool new_uncivilised,
	bool new_can_reduce_militancy, Date new_spawn_date, ConditionalWeight&& new_add_political_reform,
	ConditionalWeight&& new_remove_political_reform, ConditionalWeight&& new_add_social_reform,
	ConditionalWeight&& new_remove_social_reform, ConditionalWeight&& new_add_military_reform,
	ConditionalWeight&& new_add_economic_reform
) : HasIdentifierAndColour { new_identifier, new_colour, false }, group { new_group }, uncivilised { new_uncivilised },
	can_reduce_militancy { new_can_reduce_militancy }, spawn_date { new_spawn_date },
	add_political_reform { std::move(new_add_political_reform) },
	remove_political_reform { std::move(new_remove_political_reform) },
	add_social_reform { std::move(new_add_social_reform) }, remove_social_reform { std::move(new_remove_social_reform) },
	add_military_reform { std::move(new_add_military_reform) }, add_economic_reform { std::move(new_add_economic_reform) } {}

bool Ideology::parse_scripts(GameManager const& game_manager) {
	bool ret = true;
	ret &= add_political_reform.parse_scripts(game_manager);
	ret &= remove_political_reform.parse_scripts(game_manager);
	ret &= add_social_reform.parse_scripts(game_manager);
	ret &= remove_social_reform.parse_scripts(game_manager);
	ret &= add_military_reform.parse_scripts(game_manager);
	ret &= add_economic_reform.parse_scripts(game_manager);
	return ret;
}

bool IdeologyManager::add_ideology_group(std::string_view identifier) {
	if (identifier.empty()) {
		Logger::error("Invalid ideology group identifier - empty!");
		return false;
	}

	return ideology_groups.add_item({ identifier });
}

bool IdeologyManager::add_ideology(
	std::string_view identifier, colour_t colour, IdeologyGroup const* group, bool uncivilised, bool can_reduce_militancy,
	Date spawn_date, ConditionalWeight&& add_political_reform, ConditionalWeight&& remove_political_reform,
	ConditionalWeight&& add_social_reform, ConditionalWeight&& remove_social_reform, ConditionalWeight&& add_military_reform,
	ConditionalWeight&& add_economic_reform
) {
	if (identifier.empty()) {
		Logger::error("Invalid ideology identifier - empty!");
		return false;
	}

	if (group == nullptr) {
		Logger::error("Null ideology group for ", identifier);
		return false;
	}

	return ideologies.add_item({
		identifier, colour, *group, uncivilised, can_reduce_militancy, spawn_date, std::move(add_political_reform),
		std::move(remove_political_reform), std::move(add_social_reform), std::move(remove_social_reform),
		std::move(add_military_reform), std::move(add_economic_reform)
	});
}

/* REQUIREMENTS:
 * POL-9, POL-10, POL-11, POL-12, POL-13, POL-14, POL-15
 */
bool IdeologyManager::load_ideology_file(ast::NodeCPtr root) {
	size_t expected_ideologies = 0;
	bool ret = expect_dictionary_reserve_length(ideology_groups,
		[this, &expected_ideologies](std::string_view key, ast::NodeCPtr value) -> bool {
			bool ret = expect_length(add_variable_callback(expected_ideologies))(value);
			ret &= add_ideology_group(key);
			return ret;
		}
	)(root);
	lock_ideology_groups();

	ideologies.reserve(ideologies.size() + expected_ideologies);
	ret &= expect_dictionary([this](std::string_view ideology_group_key, ast::NodeCPtr ideology_group_value) -> bool {
		IdeologyGroup const* ideology_group = get_ideology_group_by_identifier(ideology_group_key);

		return expect_dictionary([this, ideology_group](std::string_view key, ast::NodeCPtr value) -> bool {
			colour_t colour = colour_t::null();
			bool uncivilised = true, can_reduce_militancy = false;
			Date spawn_date;
			ConditionalWeight add_political_reform, remove_political_reform, add_social_reform, remove_social_reform,
				add_military_reform, add_economic_reform;

			bool ret = expect_dictionary_keys(
				"uncivilized", ZERO_OR_ONE, expect_bool(assign_variable_callback(uncivilised)),
				"color", ONE_EXACTLY, expect_colour(assign_variable_callback(colour)),
				"date", ZERO_OR_ONE, expect_date(assign_variable_callback(spawn_date)),
				"can_reduce_militancy", ZERO_OR_ONE, expect_bool(assign_variable_callback(can_reduce_militancy)),
				"add_political_reform", ONE_EXACTLY, add_political_reform.expect_conditional_weight(ConditionalWeight::BASE),
				"remove_political_reform", ONE_EXACTLY, remove_political_reform.expect_conditional_weight(ConditionalWeight::BASE),
				"add_social_reform", ONE_EXACTLY, add_social_reform.expect_conditional_weight(ConditionalWeight::BASE),
				"remove_social_reform", ONE_EXACTLY, remove_social_reform.expect_conditional_weight(ConditionalWeight::BASE),
				"add_military_reform", ZERO_OR_ONE, add_military_reform.expect_conditional_weight(ConditionalWeight::BASE),
				"add_economic_reform", ZERO_OR_ONE, add_economic_reform.expect_conditional_weight(ConditionalWeight::BASE)
			)(value);
			ret &= add_ideology(
				key, colour, ideology_group, uncivilised, can_reduce_militancy, spawn_date, std::move(add_political_reform),
				std::move(remove_political_reform), std::move(add_social_reform), std::move(remove_social_reform),
				std::move(add_military_reform), std::move(add_economic_reform)
			);
			return ret;
		})(ideology_group_value);
	})(root);
	lock_ideologies();

	return ret;
}

bool IdeologyManager::parse_scripts(GameManager const& game_manager) {
	bool ret = true;
	for (Ideology& ideology : ideologies.get_items()) {
		ideology.parse_scripts(game_manager);
	}
	return ret;
}

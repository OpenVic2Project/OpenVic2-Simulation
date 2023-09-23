#include "Religion.hpp"

#include <cassert>

using namespace OpenVic;
using namespace OpenVic::NodeTools;

ReligionGroup::ReligionGroup(const std::string_view new_identifier) : HasIdentifier { new_identifier } {}

Religion::Religion(const std::string_view new_identifier, colour_t new_colour,
	ReligionGroup const& new_group, icon_t new_icon, bool new_pagan)
	: HasIdentifierAndColour { new_identifier, new_colour, true, false },
	  group { new_group },
	  icon { new_icon },
	  pagan { new_pagan } {
	assert(icon > 0);
}

ReligionGroup const& Religion::get_group() const {
	return group;
}

Religion::icon_t Religion::get_icon() const {
	return icon;
}

bool Religion::get_pagan() const {
	return pagan;
}

ReligionManager::ReligionManager()
	: religion_groups { "religion groups" },
	  religions { "religions" } {}

bool ReligionManager::add_religion_group(const std::string_view identifier) {
	if (identifier.empty()) {
		Logger::error("Invalid religion group identifier - empty!");
		return false;
	}
	return religion_groups.add_item({ identifier });
}

bool ReligionManager::add_religion(const std::string_view identifier, colour_t colour, ReligionGroup const* group, Religion::icon_t icon, bool pagan) {
	if (!religion_groups.is_locked()) {
		Logger::error("Cannot register religions until religion groups are locked!");
		return false;
	}
	if (identifier.empty()) {
		Logger::error("Invalid religion identifier - empty!");
		return false;
	}
	if (group == nullptr) {
		Logger::error("Null religion group for ", identifier);
		return false;
	}
	if (colour > MAX_COLOUR_RGB) {
		Logger::error("Invalid religion colour for ", identifier, ": ", colour_to_hex_string(colour));
		return false;
	}
	if (icon <= 0) {
		Logger::error("Invalid religion icon for ", identifier, ": ", icon);
		return false;
	}
	return religions.add_item({ identifier, colour, *group, icon, pagan });
}

/* REQUIREMENTS:
 * POP-286, POP-287, POP-288, POP-289, POP-290, POP-291, POP-292,
 * POP-293, POP-294, POP-295, POP-296, POP-297, POP-298, POP-299
 */
bool ReligionManager::load_religion_file(ast::NodeCPtr root) {
	size_t total_expected_religions = 0;
	bool ret = expect_dictionary_reserve_length(
		religion_groups,
		[this, &total_expected_religions](std::string_view key, ast::NodeCPtr value) -> bool {
			bool ret = expect_length(add_variable_callback(total_expected_religions))(value);
			ret &= add_religion_group(key);
			return ret;
		}
	)(root);
	lock_religion_groups();
	religions.reserve(religions.size() + total_expected_religions);
	ret &= expect_dictionary(
		[this](std::string_view religion_group_key, ast::NodeCPtr religion_group_value) -> bool {
			ReligionGroup const* religion_group = get_religion_group_by_identifier(religion_group_key);

			return expect_dictionary(
				[this, religion_group](std::string_view key, ast::NodeCPtr value) -> bool {
					colour_t colour = NULL_COLOUR;
					Religion::icon_t icon = 0;
					bool pagan = false;

					bool ret = expect_dictionary_keys(
						"icon", ONE_EXACTLY, expect_uint(assign_variable_callback_uint("religion icon", icon)),
						"color", ONE_EXACTLY, expect_colour(assign_variable_callback(colour)),
						"pagan", ZERO_OR_ONE, expect_bool(assign_variable_callback(pagan))
					)(value);
					ret &= add_religion(key, colour, religion_group, icon, pagan);
					return ret;
				}
			)(religion_group_value);
		}
	)(root);
	lock_religions();
	return ret;
}

#include "LeaderTrait.hpp"

#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/defines/MilitaryDefines.hpp"
#include "openvic-simulation/modifier/ModifierEffectCache.hpp"
#include "openvic-simulation/modifier/ModifierManager.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

LeaderTrait::LeaderTrait(std::string_view new_identifier, trait_type_t new_type, ModifierValue&& new_modifiers)
	: Modifier { new_identifier, std::move(new_modifiers), modifier_type_t::LEADER }, trait_type { new_type } {}

bool LeaderTrait::is_personality_trait() const {
	return trait_type == trait_type_t::PERSONALITY;
}

bool LeaderTrait::is_background_trait() const {
	return trait_type == trait_type_t::BACKGROUND;
}

LeaderTraitManager::LeaderTraitManager()
	: leader_prestige_modifier { "leader_prestige", {}, Modifier::modifier_type_t::LEADER } {}

bool LeaderTraitManager::setup_leader_prestige_modifier(
	ModifierEffectCache const& modifier_effect_cache, MilitaryDefines const& military_defines
) {
	if (!leader_prestige_modifier.empty()) {
		Logger::error("Leader prestige modifier already set up!");
		return false;
	}

	bool ret = true;

	if (military_defines.get_leader_prestige_to_morale_factor() != fixed_point_t::_0()) {
		if (modifier_effect_cache.get_morale_leader() != nullptr) {
			leader_prestige_modifier.set_effect(
				*modifier_effect_cache.get_morale_leader(), military_defines.get_leader_prestige_to_morale_factor()
			);
		} else {
			Logger::error("Cannot set leader prestige modifier morale effect - ModifierEffect is null!");
			ret = false;
		}
	}

	if (military_defines.get_leader_prestige_to_max_org_factor() != fixed_point_t::_0()) {
		if (modifier_effect_cache.get_organisation() != nullptr) {
			leader_prestige_modifier.set_effect(
				*modifier_effect_cache.get_organisation(), military_defines.get_leader_prestige_to_max_org_factor()
			);
		} else {
			Logger::error("Cannot set leader prestige modifier organisation effect - ModifierEffect is null!");
			ret = false;
		}
	}

	return ret;
}

bool LeaderTraitManager::add_leader_trait(
	std::string_view identifier, LeaderTrait::trait_type_t type, ModifierValue&& modifiers
) {
	if (identifier.empty()) {
		Logger::error("Invalid leader trait identifier - empty!");
		return false;
	}

	return leader_traits.add_item({ identifier, type, std::move(modifiers) });
}

bool LeaderTraitManager::load_leader_traits_file(ModifierManager const& modifier_manager, ast::NodeCPtr root) {
	const auto trait_callback = [this, &modifier_manager](LeaderTrait::trait_type_t type) -> NodeCallback auto {
		return expect_dictionary_reserve_length(
			leader_traits,
			[this, &modifier_manager, type](std::string_view trait_identifier, ast::NodeCPtr value) -> bool {
				ModifierValue modifiers;

				bool ret = NodeTools::expect_dictionary(
					modifier_manager.expect_leader_modifier(modifiers)
				)(value);

				ret &= add_leader_trait(trait_identifier, type, std::move(modifiers));

				return ret;
			}
		);
	};

	using enum LeaderTrait::trait_type_t;

	const bool ret = expect_dictionary_keys(
		"personality", ONE_EXACTLY, trait_callback(PERSONALITY),
		"background", ONE_EXACTLY, trait_callback(BACKGROUND)
	)(root);

	lock_leader_traits();

	return ret;
}

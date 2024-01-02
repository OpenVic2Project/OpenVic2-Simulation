#pragma once

#include "openvic-simulation/scripts/ConditionalWeight.hpp"
#include "openvic-simulation/scripts/EffectScript.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"

namespace OpenVic {
	struct DecisionManager;

	struct Decision : HasIdentifier {
		friend struct DecisionManager;

	private:
		const bool PROPERTY_CUSTOM_PREFIX(alert, has);
		const bool PROPERTY_CUSTOM_PREFIX(news, is);
		std::string PROPERTY(news_title);
		std::string PROPERTY(news_desc_long);
		std::string PROPERTY(news_desc_medium);
		std::string PROPERTY(news_desc_short);
		std::string PROPERTY(picture);
		ConditionScript PROPERTY(potential);
		ConditionScript PROPERTY(allow);
		ConditionalWeight PROPERTY(ai_will_do);
		EffectScript PROPERTY(effect);

		Decision(
			std::string_view new_identifier, bool new_alert, bool new_news, std::string_view new_news_title,
			std::string_view new_news_desc_long, std::string_view new_news_desc_medium, std::string_view new_news_desc_short,
			std::string_view new_picture, ConditionScript&& new_potential, ConditionScript&& new_allow,
			ConditionalWeight&& new_ai_will_do, EffectScript&& new_effect
		);

		bool parse_scripts(GameManager& game_manager);

	public:
		Decision(Decision&&) = default;
	};

	struct DecisionManager {
	private:
		IdentifierRegistry<Decision> IDENTIFIER_REGISTRY(decision);

	public:
		bool add_decision(
			std::string_view identifier, bool alert, bool news, std::string_view news_title, std::string_view news_desc_long,
			std::string_view news_desc_medium, std::string_view news_desc_short, std::string_view picture,
			ConditionScript&& potential, ConditionScript&& allow, ConditionalWeight&& ai_will_do, EffectScript&& effect
		);

		bool load_decision_file(ast::NodeCPtr root);

		bool parse_scripts(GameManager& game_manager);
	};
}

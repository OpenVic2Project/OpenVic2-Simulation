#include "ConditionalWeight.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

template<typename T>
static NodeCallback auto expect_modifier(std::vector<T>& items) {
	return [&items](ast::NodeCPtr node) -> bool {
		fixed_point_t weight = 0;
		bool successful = false;
		bool ret = expect_key("factor", expect_fixed_point(assign_variable_callback(weight)), &successful)(node);
		if (!successful) {
			Logger::info("ConditionalWeight modifier missing factor key!");
			return false;
		}
		ConditionScript condition;
		ret &= condition.expect_script()(node);
		items.emplace_back(std::make_pair(weight, std::move(condition)));
		return ret;
	};
}

node_callback_t ConditionalWeight::expect_conditional_weight(base_key_t base_key) {
	return expect_dictionary_keys(
		// TODO - add days and years as options with a shared expected count of ONE_EXACTLY
		base_key_to_string(base_key), ONE_EXACTLY, expect_fixed_point(assign_variable_callback(base)),
		"modifier", ZERO_OR_MORE, expect_modifier(condition_weight_items),
		"group", ZERO_OR_MORE, [this](ast::NodeCPtr node) -> bool {
			condition_weight_group_t items;
			const bool ret = expect_dictionary_keys(
				"modifier", ONE_OR_MORE, expect_modifier(items)
			)(node);
			if (!items.empty()) {
				condition_weight_items.emplace_back(std::move(items));
				return ret;
			}
			Logger::error("ConditionalWeight group must have at least one modifier!");
			return false;
		}
	);
}

struct ConditionalWeight::parse_scripts_visitor_t {
	GameManager const& game_manager;

	bool operator()(condition_weight_t& condition_weight) const {
		return condition_weight.second.parse_script(false, game_manager);
	}
	bool operator()(condition_weight_item_t& item) const {
		return std::visit(*this, item);
	}
	template<typename T>
	bool operator()(std::vector<T>& items) const {
		bool ret = true;
		for (T& item : items) {
			ret &= (*this)(item);
		}
		return ret;
	}
};

bool ConditionalWeight::parse_scripts(GameManager const& game_manager) {
	return parse_scripts_visitor_t { game_manager }(condition_weight_items);
}

#pragma once

#include "openvic-simulation/economy/trading/SellResult.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {
	struct GoodDefinition;

	struct GoodMarketSellOrder {
	private:
		const fixed_point_t PROPERTY(quantity);
		std::function<void(const SellResult)> PROPERTY(after_trade);

	public:
		GoodMarketSellOrder(
			const fixed_point_t new_quantity,
			std::function<void(const SellResult)>&& new_after_trade
		);
		GoodMarketSellOrder(GoodMarketSellOrder&&) = default;
	};

	struct MarketSellOrder : GoodMarketSellOrder {
	private:
		GoodDefinition const& PROPERTY(good);

	public:
		MarketSellOrder(
			GoodDefinition const& new_good,
			const fixed_point_t new_quantity,
			std::function<void(const SellResult)>&& new_after_trade
		);
		MarketSellOrder(MarketSellOrder&&) = default;
	};
}
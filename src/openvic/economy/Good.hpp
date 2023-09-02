#pragma once

#include "openvic/types/IdentifierRegistry.hpp"
#include "openvic/dataloader/NodeTools.hpp"

namespace OpenVic {
	struct GoodManager;

	struct GoodCategory : HasIdentifier {
		friend struct GoodManager;

	private:
		GoodCategory(const std::string_view new_identifier);

	public:
		GoodCategory(GoodCategory&&) = default;
	};

	/* REQUIREMENTS:
	 *
	 * ECON-3 , ECON-4 , ECON-5 , ECON-6 , ECON-7 , ECON-8 , ECON-9 , ECON-10, ECON-11, ECON-12, ECON-13, ECON-14,
	 * ECON-15, ECON-16, ECON-17, ECON-18, ECON-19, ECON-20, ECON-21, ECON-22, ECON-23, ECON-24, ECON-25, ECON-26,
	 * ECON-27, ECON-28, ECON-29, ECON-30, ECON-31, ECON-32, ECON-33, ECON-34, ECON-35, ECON-36, ECON-37, ECON-38,
	 * ECON-39, ECON-40, ECON-41, ECON-42, ECON-43, ECON-44, ECON-45, ECON-46, ECON-47, ECON-48, ECON-49, ECON-50
	 *
	 * ECON-123, ECON-124, ECON-125, ECON-126, ECON-127, ECON-128, ECON-129, ECON-130, ECON-131, ECON-132, ECON-133, ECON-134,
	 * ECON-135, ECON-136, ECON-137, ECON-138, ECON-139, ECON-140, ECON-141, ECON-142, ECON-234, ECON-235, ECON-236, ECON-237,
	 * ECON-238, ECON-239, ECON-240, ECON-241, ECON-242, ECON-243, ECON-244, ECON-245, ECON-246, ECON-247, ECON-248, ECON-249,
	 * ECON-250, ECON-251, ECON-252, ECON-253, ECON-254, ECON-255, ECON-256, ECON-257, ECON-258, ECON-259, ECON-260, ECON-261
	 */
	struct Good : HasIdentifierAndColour {
		friend struct GoodManager;

		using price_t = FP;
		static constexpr price_t NULL_PRICE = FP::_0();

	private:
		GoodCategory const& category;
		const price_t base_price;
		price_t price;
		const bool default_available, tradeable, currency, overseas_maintenance;
		bool available;

		Good(const std::string_view new_identifier, colour_t new_colour, GoodCategory const& new_category, price_t new_base_price,
			bool new_default_available, bool new_tradeable, bool new_currency, bool new_overseas_maintenance);

	public:
		Good(Good&&) = default;

		GoodCategory const& get_category() const;
		price_t get_base_price() const;
		price_t get_price() const;
		bool is_default_available() const;
		bool is_available() const;
		void reset_to_defaults();
	};

	struct GoodManager {
	private:
		IdentifierRegistry<GoodCategory> good_categories;
		IdentifierRegistry<Good> goods;

	public:
		GoodManager();

		return_t add_good_category(const std::string_view identifier);
		void lock_good_categories();
		GoodCategory const* get_good_category_by_identifier(const std::string_view identifier) const;

		return_t add_good(const std::string_view identifier, colour_t colour, GoodCategory const* category, Good::price_t base_price,
			bool default_available, bool tradeable, bool currency, bool overseas_maintenance);
		void lock_goods();
		Good const* get_good_by_index(size_t index) const;
		Good const* get_good_by_identifier(const std::string_view identifier) const;
		size_t get_good_count() const;
		std::vector<Good> const& get_goods() const;

		void reset_to_defaults();
		return_t load_good_file(ast::NodeCPtr root);
	};
}

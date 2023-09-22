#pragma once

#include "openvic-simulation/types/IdentifierRegistry.hpp"

namespace OpenVic {

	struct CultureManager;

	struct GraphicalCultureType : HasIdentifier {
		friend struct CultureManager;

	private:
		GraphicalCultureType(const std::string_view new_identifier);

	public:
		GraphicalCultureType(GraphicalCultureType&&) = default;
	};

	struct CultureGroup : HasIdentifier {
		friend struct CultureManager;

	private:
		const std::string leader;
		GraphicalCultureType const& unit_graphical_culture_type;
		const bool is_overseas;

		// TODO - union tag

		CultureGroup(const std::string_view new_identifier, const std::string_view new_leader, GraphicalCultureType const& new_unit_graphical_culture_type, bool new_is_overseas);

	public:
		CultureGroup(CultureGroup&&) = default;

		std::string const& get_leader() const;
		GraphicalCultureType const& get_unit_graphical_culture_type() const;
		bool get_is_overseas() const;
	};

	struct Culture : HasIdentifierAndColour {
		friend struct CultureManager;

	private:
		CultureGroup const& group;
		const std::vector<std::string> first_names, last_names;

		// TODO - radicalism, primary tag

		Culture(const std::string_view new_identifier, colour_t new_colour, CultureGroup const& new_group, std::vector<std::string> const& new_first_names, std::vector<std::string> const& new_last_names);

	public:
		Culture(Culture&&) = default;

		CultureGroup const& get_group() const;
		std::vector<std::string> const& get_first_names() const;
		std::vector<std::string> const& get_last_names() const;
	};

	struct CultureManager {
	private:
		IdentifierRegistry<GraphicalCultureType> graphical_culture_types;
		IdentifierRegistry<CultureGroup> culture_groups;
		IdentifierRegistry<Culture> cultures;

		bool _load_culture_group(size_t& total_expected_cultures, GraphicalCultureType const* default_unit_graphical_culture_type,
			const std::string_view culture_group_key, ast::NodeCPtr culture_group_node);
		bool _load_culture(CultureGroup const* culture_group, const std::string_view culture_key, ast::NodeCPtr node);

	public:
		CultureManager();

		bool add_graphical_culture_type(const std::string_view identifier);
		IDENTIFIER_REGISTRY_ACCESSORS(GraphicalCultureType, graphical_culture_type)

		bool add_culture_group(const std::string_view identifier, const std::string_view leader, GraphicalCultureType const* new_graphical_culture_type, bool is_overseas);
		IDENTIFIER_REGISTRY_ACCESSORS(CultureGroup, culture_group)

		bool add_culture(const std::string_view identifier, colour_t colour, CultureGroup const* group, std::vector<std::string> const& first_names, std::vector<std::string> const& last_names);
		IDENTIFIER_REGISTRY_ACCESSORS(Culture, culture)

		bool load_graphical_culture_type_file(ast::NodeCPtr root);
		bool load_culture_file(ast::NodeCPtr root);
	};
}

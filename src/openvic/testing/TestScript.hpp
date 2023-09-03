#pragma once
#include <testing/Requirement.hpp>
#include <vector>

namespace OpenVic {

	class TestScript {

		std::vector<Requirement> requirements = std::vector<Requirement>();

		public:
		TestScript() {
			
		}

		// expects an overriden method that performs arbitrary code execution
		// so that each script uniquely performs tests
		// for both requirement adding to script and to execute code
		virtual void add_requirements() = 0;
		virtual void execute_script() = 0;

		// Getters
		std::vector<Requirement> get_requirements();
		Requirement get_requirement_at_index(int index);
		Requirement get_requirement_by_id(std::string id);

		// Setters
		void set_requirements(std::vector<Requirement> in_requirements);
		void add_requirement(Requirement req);

	};
}

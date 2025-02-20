#include <thread>

#include "openvic-simulation/types/fixed_point/Atomic.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

#include "Helper.hpp" // IWYU pragma: keep
#include "Numeric.hpp" // IWYU pragma: keep
#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_misc.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic;

TEST_CASE("atomic_fixed_point_t Constructor methods", "[atomic_fixed_point_t][atomic_fixed_point_t-constructor]") {
	CHECK(atomic_fixed_point_t{} == fixed_point_t::_0());
	CHECK(atomic_fixed_point_t{fixed_point_t::_1()} == fixed_point_t::_1());
}

TEST_CASE("atomic_fixed_point_t Operators", "[atomic_fixed_point_t][atomic_fixed_point_t-operators]") {
	atomic_fixed_point_t a { fixed_point_t::_0() };

	CHECK(a++ == fixed_point_t::_0());
	CHECK(a == fixed_point_t::_1());

	CHECK(++a == fixed_point_t::_2());

	CHECK(a-- == fixed_point_t::_2());
	CHECK(a == fixed_point_t::_1());

	CHECK(--a == fixed_point_t::_0());

	CHECK((a = fixed_point_t::_10()) == fixed_point_t::_10());
	CHECK(a == fixed_point_t::_10());
}

TEST_CASE("atomic_fixed_point_t Operators", "[atomic_fixed_point_t][.atomic_fixed_point_t-atomic-behavior]") {
	//slow test from https://en.cppreference.com/w/cpp/atomic/atomic
	atomic_fixed_point_t b { fixed_point_t::_0() };
	{
		std::vector<std::thread> pool;
		for (int n = 0; n < 16; ++n) {
			pool.emplace_back([&b](){
				for (auto n{1024}; n; --n) {
					++b;
				}
			});
		}

		for (std::thread& thread : pool) {
			if (thread.joinable()) {
				thread.join();
			}
		}
	}
	CHECK(b == fixed_point_t {16*1024 });
}
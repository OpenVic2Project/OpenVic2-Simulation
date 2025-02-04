#include <cmath>
#include <cstdlib>

#include "openvic-simulation/types/Vector.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

#include "Approx.hpp"
#include "Helper.hpp" // IWYU pragma: keep
#include "Numeric.hpp"
#include "Vector.hpp"
#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_misc.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic;
using namespace OpenVic::testing::approx_literal;

using VectorValueTypes = snitch::type_list<int32_t, OpenVic::fixed_point_t, double>;

TEMPLATE_LIST_TEST_CASE("vec3_t Constructor methods", "[vec3_t][vec3_t-constructor]", VectorValueTypes) {
	const vec3_t<TestType> vector_empty = vec3_t<TestType>();
	const vec3_t<TestType> vector_zero = vec3_t<TestType>(0, 0, 0);
	CHECK(vector_empty == vector_zero);
}

TEMPLATE_LIST_TEST_CASE("vec3_t Length methods", "[vec3_t][vec3_t-length]", VectorValueTypes) {
	const vec3_t<TestType> vector1 = vec3_t<TestType>(10, 10, 10);
	const vec3_t<TestType> vector2 = vec3_t<TestType>(20, 30, 40);
	CHECK(vector1.length_squared() == 300);
	CHECK(vector2.length_squared() == 2900);
	CHECK(vector1.distance_squared(vector2) == 1400);
}

TEMPLATE_LIST_TEST_CASE("vec3_t Operators", "[vec3_t][vec3_t-operators]", VectorValueTypes) {
	const vec3_t<TestType> int1 = vec3_t<TestType>(4, 5, 9);
	const vec3_t<TestType> int2 = vec3_t<TestType>(1, 2, 3);

	CHECK((int1 + int2) == vec3_t<TestType>(5, 7, 12));
	CHECK((int1 - int2) == vec3_t<TestType>(3, 3, 6));
	CHECK((int1 * int2) == vec3_t<TestType>(4, 10, 27));
	CHECK((int1 * 2) == vec3_t<TestType>(8, 10, 18));
	CHECK(vec3_t<TestType>(ivec3_t(1, 2, 3)) == vec3_t<TestType>(1, 2, 3));
}

TEMPLATE_LIST_TEST_CASE("vec3_t Rounding methods", "[vec3_t][vec3_t-rounding]", VectorValueTypes) {
	const vec3_t<TestType> vector1 = vec3_t<TestType>(1, 3, 5);
	const vec3_t<TestType> vector2 = vec3_t<TestType>(1, -3, -5);
	CHECK(vector1.abs() == vector1);
	CHECK(vector2.abs() == vector1);
}

TEMPLATE_LIST_TEST_CASE("vec3_t Linear algebra methods", "[vec3_t][vec3_t-linear-algebra]", VectorValueTypes) {
	const vec3_t<TestType> vector_x = vec3_t<TestType>(1, 0, 0);
	const vec3_t<TestType> vector_y = vec3_t<TestType>(0, 1, 0);
	const vec3_t<TestType> a = vec3_t<TestType>(3, 8, 2);
	const vec3_t<TestType> b = vec3_t<TestType>(5, 4, 7);

	CHECK(vector_x.dot(vector_y) == 0);
	CHECK(vector_x.dot(vector_x) == 1);
	CHECK((vector_x * 10).dot(vector_x * 10) == 100);
	CHECK(a.dot(b) == 61);
	CHECK(vec3_t<TestType>(-a.x, a.y, -a.z).dot(vec3_t<TestType>(b.x, -b.y, b.z)) == -61);
}

TEST_CASE("fvec3_t Length methods", "[vec3_t][fvec3_t][fvec3_t-length]") {
	const fvec3_t vector1 = fvec3_t(10, 10, 10);
	const fvec3_t vector2 = fvec3_t(20, 30, 40);
	CHECK(vector1.length_squared().sqrt() == testing::approx(testing::SQRT3 * 10));
	CHECK(vector2.length_squared().sqrt() == testing::approx(53.8516480713450403125));
	CHECK(vector1.distance_squared(vector2).sqrt() == testing::approx(37.41657386773941385584));
}

TEST_CASE("dvec3_t Length methods", "[vec3_t][dvec3_t][dvec3_t-length]") {
	const dvec3_t vector1 = dvec3_t(10, 10, 10);
	const dvec3_t vector2 = dvec3_t(20, 30, 40);
	CHECK(std::sqrt(vector1.length_squared()) == testing::approx(10 * testing::SQRT3));
	CHECK(std::sqrt(vector2.length_squared()) == testing::approx(53.8516480713450403125));
	CHECK(std::sqrt(vector1.distance_squared(vector2)) == testing::approx(37.41657386773941385584));
}

TEST_CASE("fvec3_t Operators", "[vec3_t][fvec3_t][fvec3_t-operators]") {
	const fixed_point_t _2_30 = fixed_point_t::_2() + fixed_point_t::_0_20() + fixed_point_t::_0_10();
	const fixed_point_t _4_90 = fixed_point_t::_4() + fixed_point_t::_0_50() + fixed_point_t::_0_20() * 2;
	const fixed_point_t _7_80 = //
		fixed_point_t::_4() + fixed_point_t::_2() + fixed_point_t::_1() + fixed_point_t::_0_50() + fixed_point_t::_0_20() +
		fixed_point_t::_0_10();
	const fixed_point_t _1_20 = fixed_point_t::_0_20() * 6;
	const fixed_point_t _3_40 = fixed_point_t::_0_20() * 17;
	const fixed_point_t _5_60 = fixed_point_t::_4() + fixed_point_t::_1_50() + fixed_point_t::_0_10();
	const fixed_point_t _0_75 = fixed_point_t::_0_25() * 3;
	const fixed_point_t _0_125 = fixed_point_t::_1() / 8;
	const fixed_point_t _0_625 = fixed_point_t::_0_50() + _0_125;

	const fvec3_t decimal1 = { _2_30, _4_90, _7_80 };
	const fvec3_t decimal2 = { _1_20, _3_40, _5_60 };
	const fvec3_t power1 = { _0_75, fixed_point_t::_1_50(), _0_625 };
	const fvec3_t power2 = { fixed_point_t::_0_50(), _0_125, fixed_point_t::_0_25() };
	const fvec3_t int1 = fvec3_t(4, 5, 9);
	const fvec3_t int2 = fvec3_t(1, 2, 3);

	CHECK(decimal1 + decimal2 == testing::approx_vec3(3.5, 8.3, 13.4));
	CHECK(power1 + power2 == testing::approx_vec3(1.25, 1.625, 0.875));

	CHECK(
		decimal1 - decimal2 ==
		testing::approx_vec3 {
			(1.1_a).epsilon(testing::INACCURATE_EPSILON), //
			(1.5_a).epsilon(testing::INACCURATE_EPSILON), //
			(2.2_a).epsilon(testing::INACCURATE_EPSILON) //
		}
	);
	CHECK(power1 - power2 == testing::approx_vec3(0.25, 1.375, 0.375));

	CHECK(
		decimal1 * decimal2 ==
		testing::approx_vec3 {
			(2.76_a).epsilon(testing::INACCURATE_EPSILON), //
			(16.66_a).epsilon(testing::INACCURATE_EPSILON), //
			(43.68_a).epsilon(testing::INACCURATE_EPSILON) //
		}
	);
	CHECK(power1 * power2 == testing::approx_vec3(0.375, 0.1875, 0.15625));

	CHECK(int1 / int2 == testing::approx_vec3(4, 2.5, 3));
	CHECK(decimal1 / decimal2 == testing::approx_vec3(1.91666666666666666, 1.44117647058823529, 1.39285714285714286));
	CHECK(power1 / power2 == testing::approx_vec3(1.5, 12.0, 2.5));

	CHECK(decimal1 * 2 == testing::approx_vec3(4.6, 9.8, 15.6));
	CHECK(power1 * 2 == testing::approx_vec3(1.5, 3, 1.25));

	CHECK(int1 / 2 == testing::approx_vec3(2, 2.5, 4.5));
	CHECK(decimal1 / 2 == testing::approx_vec3(1.15, 2.45, 3.9));
	CHECK(power1 / 2 == testing::approx_vec3(0.375, 0.75, 0.3125));

	CHECK(((ivec3_t)decimal1) == ivec3_t(2, 4, 7));
	CHECK(((ivec3_t)decimal2) == ivec3_t(1, 3, 5));
}

TEST_CASE("dvec3_t Operators", "[vec3_t][dvec3_t][dvec3_t-operators]") {
	const dvec3_t decimal1 = dvec3_t(2.3, 4.9, 7.8);
	const dvec3_t decimal2 = dvec3_t(1.2, 3.4, 5.6);
	const dvec3_t power1 = dvec3_t(0.75, 1.5, 0.625);
	const dvec3_t power2 = dvec3_t(0.5, 0.125, 0.25);
	const dvec3_t int1 = dvec3_t(4, 5, 9);
	const dvec3_t int2 = dvec3_t(1, 2, 3);

	CHECK(decimal1 + decimal2 == testing::approx_vec3(3.5, 8.3, 13.4));
	CHECK(power1 + power2 == dvec3_t(1.25, 1.625, 0.875));

	CHECK(decimal1 - decimal2 == testing::approx_vec3(1.1, 1.5, 2.2));
	CHECK(power1 - power2 == dvec3_t(0.25, 1.375, 0.375));

	CHECK(decimal1 * decimal2 == testing::approx_vec3(2.76, 16.66, 43.68));
	CHECK(power1 * power2 == dvec3_t(0.375, 0.1875, 0.15625));

	CHECK(int1 / int2 == dvec3_t(4, 2.5, 3));
	CHECK(decimal1 / decimal2 == testing::approx_vec3(1.91666666666666666, 1.44117647058823529, 1.39285714285714286));
	CHECK(power1 / power2 == dvec3_t(1.5, 12.0, 2.5));

	CHECK(decimal1 * 2 == testing::approx_vec3(4.6, 9.8, 15.6));
	CHECK(power1 * 2 == dvec3_t(1.5, 3, 1.25));

	CHECK(int1 / 2 == dvec3_t(2, 2.5, 4.5));
	CHECK(decimal1 / 2 == testing::approx_vec3(1.15, 2.45, 3.9));
	CHECK(power1 / 2 == dvec3_t(0.375, 0.75, 0.3125));

	CHECK(((ivec3_t)decimal1) == ivec3_t(2, 4, 7));
	CHECK(((ivec3_t)decimal2) == ivec3_t(1, 3, 5));
}

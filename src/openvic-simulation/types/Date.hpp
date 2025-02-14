#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <charconv>
#include <cstdint>
#include <cstring>
#include <limits>
#include <numeric>
#include <ostream>
#include <string>
#include <system_error>

#include <fmt/core.h>

#include <range/v3/algorithm/max_element.hpp>

#include "openvic-simulation/utility/ErrorMacros.hpp"
#include "openvic-simulation/utility/Utility.hpp"

namespace OpenVic {
	// A relative period between points in time, measured in days
	struct Timespan {
		/* PROPERTY generated getter functions will return timespans by value, rather than const reference. */
		using ov_return_by_value = void;

		using day_t = int64_t;

	private:
		day_t days;

	public:
		constexpr Timespan(day_t value = 0) : days { value } {}

		friend constexpr auto operator<=>(Timespan const&, Timespan const&) = default;
		friend constexpr bool operator==(Timespan const&, Timespan const&) = default;
		constexpr Timespan operator+(Timespan other) const {
			return days + other.days;
		}
		constexpr Timespan operator-(Timespan other) const {
			return days - other.days;
		}
		constexpr Timespan operator*(day_t factor) const {
			return days * factor;
		}
		constexpr Timespan operator/(day_t factor) const {
			return days / factor;
		}
		constexpr Timespan& operator+=(Timespan other) {
			days += other.days;
			return *this;
		}
		constexpr Timespan& operator-=(Timespan other) {
			days -= other.days;
			return *this;
		}
		constexpr Timespan& operator++() {
			days++;
			return *this;
		}
		constexpr Timespan operator++(int) {
			Timespan old = *this;
			++(*this);
			return old;
		}
		constexpr Timespan& operator--() {
			days--;
			return *this;
		}
		constexpr Timespan operator--(int) {
			Timespan old = *this;
			--(*this);
			return old;
		}

		constexpr day_t to_int() const {
			return days;
		}
		explicit constexpr operator day_t() const {
			return days;
		}

		inline std::to_chars_result to_chars(char* first, char* last) const {
			return std::to_chars(first, last, days);
		}

		std::string to_string() const;
		explicit operator std::string() const;

		static constexpr Timespan from_years(day_t num);
		static constexpr Timespan from_months(day_t num);
		static constexpr Timespan from_days(day_t num);
	};
	std::ostream& operator<<(std::ostream& out, Timespan const& timespan);

	// Represents an in-game date
	// Note: Current implementation does not account for leap-years, or dates before Year 0
	struct Date {
		/* PROPERTY generated getter functions will return dates by value, rather than const reference. */
		using ov_return_by_value = void;

		using year_t = uint16_t;
		using month_t = uint8_t;
		using day_t = uint8_t;

		static constexpr std::array DAYS_IN_MONTH =
			std::to_array<Timespan::day_t>({ 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 });

		static constexpr Timespan::day_t MONTHS_IN_YEAR = DAYS_IN_MONTH.size();
		static constexpr Timespan::day_t MAX_DAYS_IN_MONTH = *ranges::max_element(DAYS_IN_MONTH);

		static constexpr Timespan::day_t DAYS_IN_YEAR = std::accumulate(
			DAYS_IN_MONTH.begin(), DAYS_IN_MONTH.end(), Timespan::day_t { 0 }
		);
		static_assert(DAYS_IN_YEAR == 365);

		static constexpr std::array<Timespan::day_t, MONTHS_IN_YEAR> DAYS_UP_TO_MONTH = [] {
			std::array<Timespan::day_t, MONTHS_IN_YEAR> days_up_to_month {};
			Timespan::day_t days = 0;
			for (Timespan::day_t month = 0; month < MONTHS_IN_YEAR; month++) {
				days_up_to_month[month] = days;
				days += DAYS_IN_MONTH[month];
			}
			return days_up_to_month;
		}();

		static constexpr std::array<month_t, DAYS_IN_YEAR> MONTH_FROM_DAY_IN_YEAR = [] {
			std::array<month_t, DAYS_IN_YEAR> month_from_day_in_year {};
			Timespan::day_t days_left = 0;
			month_t month = 0;
			for (Timespan::day_t day = 0; day < DAYS_IN_YEAR; day++) {
				days_left = (days_left > 0 ? days_left : DAYS_IN_MONTH[month++]) - 1;
				month_from_day_in_year[day] = month;
			}
			return month_from_day_in_year;
		}();

		static constexpr char SEPARATOR_CHARACTER = '.';

		static constexpr std::array<std::string_view, MONTHS_IN_YEAR> MONTH_NAMES {
			"January", "February", "March",		"April",   "May",	   "June", //
			"July",	   "August",   "September", "October", "November", "December" //
		};
		static constexpr std::string_view INVALID_MONTH_NAME = "Invalid Month";

	private:
		// Number of days since Jan 1st, Year 0
		Timespan timespan;

		static constexpr Timespan _date_to_timespan(year_t year, month_t month, day_t day) {
			month = std::clamp<month_t>(month, 1, MONTHS_IN_YEAR);
			day = std::clamp<day_t>(day, 1, DAYS_IN_MONTH[month - 1]);
			return year * DAYS_IN_YEAR + DAYS_UP_TO_MONTH[month - 1] + day - 1;
		}

	public:
		// The Timespan is considered to be the number of days since Jan 1st, Year 0
		constexpr Date(Timespan total_days) : timespan { total_days >= 0 ? total_days : 0 } {}
		// Year month day specification
		constexpr Date(year_t year = 0, month_t month = 1, day_t day = 1) : timespan { _date_to_timespan(year, month, day) } {}

		constexpr year_t get_year() const {
			return static_cast<Timespan::day_t>(timespan) / DAYS_IN_YEAR;
		}
		constexpr month_t get_month() const {
			return MONTH_FROM_DAY_IN_YEAR[static_cast<Timespan::day_t>(timespan) % DAYS_IN_YEAR];
		}
		constexpr day_t get_day() const {
			return (static_cast<Timespan::day_t>(timespan) % DAYS_IN_YEAR) - DAYS_UP_TO_MONTH[get_month() - 1] + 1;
		}

		constexpr bool is_month_start() const {
			return get_day() == 1;
		}

		friend constexpr auto operator<=>(Date const&, Date const&) = default;
		friend constexpr bool operator==(Date const&, Date const&) = default;
		constexpr Date operator+(Timespan other) const {
			return timespan + other;
		}
		constexpr Timespan operator-(Date other) const {
			return timespan - other.timespan;
		}
		constexpr Date& operator+=(Timespan other) {
			timespan += other;
			return *this;
		}
		constexpr Date& operator-=(Timespan other) {
			timespan -= other;
			return *this;
		}
		constexpr Date& operator++() {
			timespan++;
			return *this;
		}
		constexpr Date operator++(int) {
			Date old = *this;
			++(*this);
			return old;
		}
		constexpr Date& operator--() {
			timespan--;
			return *this;
		}
		constexpr Date operator--(int) {
			Date old = *this;
			--(*this);
			return old;
		}

		constexpr bool in_range(Date start, Date end) const {
			assert(start <= end);
			return start <= *this && *this <= end;
		}

		constexpr std::string_view get_month_name() const {
			const month_t month = get_month();
			if (1 <= month && month <= MONTHS_IN_YEAR) {
				return MONTH_NAMES[month - 1];
			}
			return INVALID_MONTH_NAME;
		}

		inline std::to_chars_result to_chars( //
			char* first, char* last, bool pad_year = false, bool pad_month = true, bool pad_day = true
		) const {
			const year_t year = get_year();
			if (pad_year) {
				if (last - first < 4) {
					return { first, std::errc::value_too_large };
				}

				if (year < 1000) {
					*first = '0';
					first++;
				}
				if (year < 100) {
					*first = '0';
					first++;
				}
				if (year < 10) {
					*first = '0';
					first++;
				}
			}

			std::to_chars_result result = std::to_chars(first, last, year);
			if (OV_unlikely(result.ec != std::errc {})) {
				return result;
			}

			const size_t required_size_after_year = 4 + pad_month + pad_day;
			if (OV_unlikely(last - result.ptr < required_size_after_year)) {
				result.ec = std::errc::value_too_large;
				result.ptr = last;
				return result;
			}

			*result.ptr = SEPARATOR_CHARACTER;
			result.ptr++;
			const month_t month = get_month();
			if (pad_month && month < 10) {
				*result.ptr = '0';
				result.ptr++;
			}
			result = std::to_chars(result.ptr, last, get_month());
			if (OV_unlikely(result.ec != std::errc {})) {
				return result;
			}

			const size_t required_size_after_month = 2 + pad_day;
			if (OV_unlikely(last - result.ptr < required_size_after_month)) {
				result.ec = std::errc::value_too_large;
				result.ptr = last;
				return result;
			}

			*result.ptr = SEPARATOR_CHARACTER;
			result.ptr++;
			const month_t day = get_day();
			if (pad_day && day < 10) {
				*result.ptr = '0';
				result.ptr++;
			}
			result = std::to_chars(result.ptr, last, get_day());
			return result;
		}

		std::string to_string(bool pad_year = false, bool pad_month = true, bool pad_day = true) const;
		explicit operator std::string() const;

		enum class errc_type : uint8_t { day, month, year };
		struct from_chars_result : std::from_chars_result {
			const char* type_first = nullptr;
			errc_type type;
		};

	private:
		/*
			type is set to errc_type::year
			type_first is set to first
			May return std::from_chars errors for year, year remains unchanged
			If year < 0, ec == not_supported and ptr == first, year remains unchanged
			If year > 65535, ec == value_too_large and ptr == first, year remains unchanged
			If string only includes a valid year value,
				ec == result_out_of_range and ptr == first, only year is changed
			If string doesn't contain a separator,
				ec == invalid_argument and ptr == expected year/month separator position, only year is changed

			type is set to errc_type::month
			type_first is set to month's first
			May return std::from_chars errors for month, month remains unchanged
			If month == 0, ec == not_supported and ptr == month's first, only year is changed
			If month > 12, ec == value_too_large and ptr == month's first, only year is changed
			If string only includes a valid year and month value,
				ec == result_out_of_range and ptr == month's first, only year and month are changed
			If string doesn't contain a separator,
				ec == invalid_argument and ptr == expected month/day separator position, only year and month are changed

			type is set to errc_type::day
			type_first is set to day's first
			May return std::from_chars errors for day, day remains unchanged
			If day == 0, ec == not_supported and ptr == day's first, only year and month are changed
			If day > days in month, ec == value_too_large and ptr == month's first, only year month are changed
		*/
		inline static from_chars_result parse_from_chars( //
			const char* first, const char* last, year_t& year, month_t& month, day_t& day
		) {
			int32_t year_check = year;
			from_chars_result result = { std::from_chars(first, last, year_check) };
			result.type_first = first;
			result.type = errc_type::year;
			if (OV_unlikely(result.ec != std::errc {})) {
				return result;
			}

			if (OV_unlikely(year_check < 0)) {
				result.ec = std::errc::not_supported;
				result.ptr = first;
				return result;
			}
			if (OV_unlikely(year_check > std::numeric_limits<year_t>::max())) {
				result.ec = std::errc::value_too_large;
				result.ptr = first;
				return result;
			}
			year = year_check;

			if (OV_unlikely(result.ptr >= last)) {
				result.ec = std::errc::result_out_of_range;
				result.ptr = first;
				return result;
			}

			if (OV_unlikely(last - result.ptr < 1 || *result.ptr != SEPARATOR_CHARACTER)) {
				result.ec = std::errc::invalid_argument;
				return result;
			}

			result.ptr++;
			first = result.ptr;
			month_t month_check = month;
			result = { std::from_chars(first, last, month_check) };
			result.type_first = first;
			result.type = errc_type::month;
			if (OV_unlikely(result.ec != std::errc {})) {
				return result;
			}

			if (OV_unlikely(month_check == 0)) {
				result.ec = std::errc::not_supported;
				result.ptr = first;
				return result;
			}

			if (OV_unlikely(month_check > MONTHS_IN_YEAR)) {
				result.ec = std::errc::value_too_large;
				result.ptr = first;
				return result;
			}
			month = month_check;

			if (OV_unlikely(result.ptr >= last)) {
				result.ec = std::errc::result_out_of_range;
				result.ptr = first;
				return result;
			}

			if (OV_unlikely(last - result.ptr < 1 || *result.ptr != SEPARATOR_CHARACTER)) {
				result.ec = std::errc::invalid_argument;
				return result;
			}

			result.ptr++;
			first = result.ptr;
			day_t day_check = day;
			result = { std::from_chars(first, last, day_check) };
			result.type_first = first;
			result.type = errc_type::day;
			if (OV_unlikely(result.ec != std::errc {})) {
				return result;
			}

			if (OV_unlikely(day_check == 0)) {
				result.ec = std::errc::not_supported;
				result.ptr = first;
				return result;
			}

			if (OV_unlikely(day_check > DAYS_IN_MONTH[month - 1])) {
				result.ec = std::errc::value_too_large;
				result.ptr = first;
				return result;
			}
			day = day_check;

			return result;
		}

	public:
		// Parsed from string of the form YYYY.MM.DD
		from_chars_result from_chars(const char* first, const char* last) {
			year_t year = 0;
			month_t month = 0;
			day_t day = 0;
			from_chars_result result = parse_from_chars(first, last, year, month, day);
			*this = { year, month, day };
			return result;
		}

		// Parsed from string of the form YYYY.MM.DD
		static Date from_string(std::string_view str, from_chars_result* from_chars = nullptr) {
			Date date {};
			if (from_chars == nullptr) {
				date.from_chars(str.data(), str.data() + str.size());
			} else {
				*from_chars = date.from_chars(str.data(), str.data() + str.size());
			}
			return date;
		}

	private:
		inline static Date handle_from_string_log(std::string_view str, from_chars_result* from_chars) {
			OV_ERR_FAIL_COND_V(from_chars == nullptr, Date {});

			Date date = from_string(str, from_chars);
			OV_ERR_FAIL_COND_V_MSG(
				from_chars->ec == std::errc::invalid_argument && from_chars->type == errc_type::year &&
					from_chars->ptr == from_chars->type_first,
				date, "Could not parse year value."
			);
			OV_ERR_FAIL_COND_V_MSG(
				from_chars->ec == std::errc::value_too_large && from_chars->type == errc_type::year, date,
				"Year value was too large."
			);
			OV_ERR_FAIL_COND_V_MSG(
				from_chars->ec == std::errc::result_out_of_range && from_chars->type == errc_type::year, date,
				"Only year value could be found."
			);
			OV_ERR_FAIL_COND_V_MSG(
				from_chars->ec == std::errc::invalid_argument && from_chars->type == errc_type::year &&
					from_chars->ptr != from_chars->type_first,
				date, fmt::format("Year value was missing a separator (\"{}\").", SEPARATOR_CHARACTER)
			);
			OV_ERR_FAIL_COND_V_MSG(
				from_chars->ec == std::errc::invalid_argument && from_chars->type == errc_type::month &&
					from_chars->ptr == from_chars->type_first,
				date, "Could not parse month value."
			);
			OV_ERR_FAIL_COND_V_MSG(
				from_chars->ec == std::errc::not_supported && from_chars->type == errc_type::month, date,
				"Month value cannot be 0."
			);
			OV_ERR_FAIL_COND_V_MSG(
				from_chars->ec == std::errc::value_too_large && from_chars->type == errc_type::month &&
					from_chars->ptr == from_chars->type_first,
				date, fmt::format("Month value cannot be larger than {}.", MONTHS_IN_YEAR)
			);
			OV_ERR_FAIL_COND_V_MSG(
				from_chars->ec == std::errc::result_out_of_range && from_chars->type == errc_type::month, date,
				"Only year and month value could be found."
			);
			OV_ERR_FAIL_COND_V_MSG(
				from_chars->ec == std::errc::invalid_argument && from_chars->type == errc_type::month &&
					from_chars->ptr != from_chars->type_first,
				date, fmt::format("Month value was missing a separator (\"{}\").", SEPARATOR_CHARACTER)
			);
			OV_ERR_FAIL_COND_V_MSG(
				from_chars->ec == std::errc::invalid_argument && from_chars->type == errc_type::day &&
					from_chars->ptr == from_chars->type_first,
				date, "Could not parse day value."
			);
			OV_ERR_FAIL_COND_V_MSG(
				from_chars->ec == std::errc::not_supported && from_chars->type == errc_type::day, date, "Day value cannot be 0."
			);
			OV_ERR_FAIL_COND_V_MSG(
				from_chars->ec == std::errc::value_too_large && from_chars->type == errc_type::day &&
					from_chars->ptr == from_chars->type_first,
				date,
				fmt::format("Day value cannot be larger than {} for {}.", DAYS_IN_MONTH[date.get_month() - 1], date.get_month())
			);

			return date;
		}

	public:
		// Parsed from string of the form YYYY.MM.DD
		static Date from_string_log(std::string_view str) {
			from_chars_result from_chars {};
			Date date = handle_from_string_log(str, &from_chars);
			return date;
		}

		// Parsed from string of the form YYYY.MM.DD
		static Date from_string_log(std::string_view str, from_chars_result* from_chars) {
			if (from_chars == nullptr) {
				return from_string_log(str);
			}
			Date date = handle_from_string_log(str, from_chars);
			return date;
		}
	};
	std::ostream& operator<<(std::ostream& out, Date date);

	constexpr Timespan Timespan::from_years(day_t num) {
		return num * Date::DAYS_IN_YEAR;
	}
	constexpr Timespan Timespan::from_months(day_t num) {
		return (num / Date::MONTHS_IN_YEAR) * Date::DAYS_IN_YEAR + Date::DAYS_UP_TO_MONTH[num % Date::MONTHS_IN_YEAR];
	}
	constexpr Timespan Timespan::from_days(day_t num) {
		return num;
	}
}

namespace std {
	template<>
	struct hash<OpenVic::Date> {
		[[nodiscard]] std::size_t operator()(OpenVic::Date date) const {
			std::size_t result = 0;
			OpenVic::utility::perfect_hash(result, date.get_day(), date.get_month(), date.get_year());
			return result;
		}
	};
}

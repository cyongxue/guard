/*
 *  weekday.cpp, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年3月16日
 *      Author: yongxue@cyongxue@163.com
 */

#include "util.h"
#include "weekday.h"

namespace util {

static std::vector<std::string> week_day = {
		"sunday", "sun",
		"monday", "mon",
		"tuesday", "tue",
		"wednesday", "wed",
		"thursday", "thu",
		"friday", "fri",
		"saturday", "sat",
		"weekdays", "weekends"
};


bool is_valid_weekday(const std::string& this_day) {
	for (auto it = week_day.begin(); it != week_day.end(); it++) {
		if (*it == this_day) {
			return true;
		}
	}

	return false;
}

/**
 * 输入格式，eg：sunday,weekdays,!tuesday
 */
int WeekDayFlag::parse_day_flags() {
	auto days = split_str(_input_str, ',');
	for (auto it = days.begin(); it != days.end(); it++) {
		auto tmp_day = rm_end_head_char(*it, ' ');
		if (tmp_day.empty()) {
			continue;
		}
		int ng = 0;
		if (tmp_day[0] == '!') {
			ng = 1;
			tmp_day = tmp_day.substr(1);
		}

		if ((tmp_day == week_day[0]) || (tmp_day == week_day[1])) {
			if (ng) {
				_day_flags &= ~ (DayFlag::SUNDAY);
			}
			else {
				_day_flags |= DayFlag::SUNDAY;
			}
		}
		else if ((tmp_day == week_day[2]) || (tmp_day == week_day[3])) {
			if (ng) {
				_day_flags &= ~ (DayFlag::MONDAY);
			}
			else {
				_day_flags |= DayFlag::MONDAY;
			}
		}
		else if ((tmp_day == week_day[4]) || (tmp_day == week_day[5])) {
			if (ng) {
				_day_flags &= ~ (DayFlag::TUESDAY);
			}
			else {
				_day_flags |= DayFlag::TUESDAY;
			}
		}
		else if ((tmp_day == week_day[6]) || (tmp_day == week_day[7])) {
			if (ng) {
				_day_flags &= ~ (DayFlag::WEDNESDAY);
			}
			else {
				_day_flags |= DayFlag::WEDNESDAY;
			}
		}
		else if ((tmp_day == week_day[8]) || (tmp_day == week_day[9])) {
			if (ng) {
				_day_flags &= ~ (DayFlag::THURSDAY);
			}
			else {
				_day_flags |= DayFlag::THURSDAY;
			}
		}
		else if ((tmp_day == week_day[10]) || (tmp_day == week_day[11])) {
			if (ng) {
				_day_flags &= ~ (DayFlag::FRIDAY);
			}
			else {
				_day_flags |= DayFlag::FRIDAY;
			}
		}
		else if ((tmp_day == week_day[12]) || (tmp_day == week_day[13])) {
			if (ng) {
				_day_flags &= ~ (DayFlag::SATURDAY);
			}
			else {
				_day_flags |= DayFlag::SATURDAY;
			}
		}
		else if (tmp_day == week_day[14]) {
			if (ng) {
				_day_flags &= ~ (DayFlag::MONDAY | DayFlag::TUESDAY | DayFlag::WEDNESDAY | DayFlag::THURSDAY | DayFlag::FRIDAY);
			}
			else {
				_day_flags |= DayFlag::MONDAY;
				_day_flags |= DayFlag::TUESDAY;
				_day_flags |= DayFlag::WEDNESDAY;
				_day_flags |= DayFlag::THURSDAY;
				_day_flags |= DayFlag::FRIDAY;
			}
		}
		else if (tmp_day == week_day[15]) {
			if (ng) {
				_day_flags &= ~ (DayFlag::SATURDAY | DayFlag::SUNDAY);
			}
			else {
				_day_flags |= DayFlag::SATURDAY;
				_day_flags |= DayFlag::SUNDAY;
			}
		} else {
			continue;
		}
	}

	if (_day_flags == 0) {
		return -1;
	}
	return 0;
}

bool WeekDayFlag::is_on_day(const std::string& this_day) const {
	auto tmp_day = rm_end_head_char(this_day, ' ');
	if (tmp_day.empty()) {
		return false;
	}

	uint32_t day_flag = 0;
	if ((tmp_day == week_day[0]) || (tmp_day == week_day[1])) {
		day_flag |= DayFlag::SUNDAY;
	}
	else if ((tmp_day == week_day[2]) || (tmp_day == week_day[3])) {
		day_flag |= DayFlag::MONDAY;
	}
	else if ((tmp_day == week_day[4]) || (tmp_day == week_day[5])) {
		day_flag |= DayFlag::TUESDAY;
	}
	else if ((tmp_day == week_day[6]) || (tmp_day == week_day[7])) {
		day_flag |= DayFlag::WEDNESDAY;
	}
	else if ((tmp_day == week_day[8]) || (tmp_day == week_day[9])) {
		day_flag |= DayFlag::THURSDAY;
	}
	else if ((tmp_day == week_day[10]) || (tmp_day == week_day[11])) {
		day_flag |= DayFlag::FRIDAY;
	}
	else if ((tmp_day == week_day[12]) || (tmp_day == week_day[13])) {
		day_flag |= DayFlag::SATURDAY;
	}
	else {
		return false;
	}

	if (day_flag & _day_flags) {
		return true;
	}
	return false;
}

bool WeekDayFlag::is_on_day(int day) const {

	if (day < 0 || day > 6) {
		return false;
	}

	uint32_t day_flag = 0;
	if (day == 0) {
		day_flag |= DayFlag::SUNDAY;
	}
	else if (day == 1) {
		day_flag |= DayFlag::MONDAY;
	}
	else if (day == 2) {
		day_flag |= DayFlag::TUESDAY;
	}
	else if (day == 3) {
		day_flag |= DayFlag::WEDNESDAY;
	}
	else if (day == 4) {
		day_flag |= DayFlag::THURSDAY;
	}
	else if (day == 5) {
		day_flag |= DayFlag::FRIDAY;
	}
	else if (day == 6) {
		day_flag |= DayFlag::SATURDAY;
	}
	else {
		return false;
	}

	if (day_flag & _day_flags) {
		return true;
	}
	return false;
}

std::string WeekDayFlag::print_day() const {
	std::string day_str;

	if (_day_flags & DayFlag::SUNDAY) {
		day_str += "sunday";
	}
	else {
		day_str += "!sunday";
	}
	day_str += ",";

	if (_day_flags & DayFlag::MONDAY) {
		day_str += "monday";
	}
	else {
		day_str += "!monday";
	}
	day_str += ",";

	if (_day_flags & DayFlag::TUESDAY) {
		day_str += "tuesday";
	}
	else {
		day_str += "!tuesday";
	}
	day_str += ",";

	if (_day_flags & DayFlag::WEDNESDAY) {
		day_str += "wednesday";
	}
	else {
		day_str += "!wednesday";
	}
	day_str += ",";

	if (_day_flags & DayFlag::THURSDAY) {
		day_str += "thursday";
	}
	else {
		day_str += "!thursday";
	}
	day_str += ",";

	if (_day_flags & DayFlag::FRIDAY) {
		day_str += "friday";
	}
	else {
		day_str += "!friday";
	}
	day_str += ",";

	if (_day_flags & DayFlag::SATURDAY) {
		day_str += "saturday";
	}
	else {
		day_str += "!saturday";
	}

	return rm_end_head_char(day_str, ',');
}

}



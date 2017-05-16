/*
 *  weekday_test.cpp, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年3月16日
 *      Author: yongxue@cyongxue@163.com
 */

#include <iostream>

#include "weekday.h"
#include "daytime.h"

int main() {

	std::cout << "begin test WeekDayFlag!!!!" << std::endl;
	auto week_day_flags = util::WeekDayFlag("weekdays");
	week_day_flags.parse_day_flags();

	std::cout << std::hex << week_day_flags.day_flags() << std::endl;
	std::cout << week_day_flags.print_day() << std::endl;

	std::cout << week_day_flags.is_on_day("tuesday") << std::endl;
	std::cout << week_day_flags.is_on_day("saturday") << std::endl;

	auto day_time = util::DayTime("8:30");
	day_time.parse_time();
	std::cout << day_time.time() << std::endl;
	std::cout << day_time.is_after_time("00:00") << std::endl;

	return 0;
}


//#define UNIT_TEST

#ifdef UNIT_TEST
#include <iostream>

int main() {

	std::cout << "Test util::split_str" << std::endl;
	std::string src = "/etc,/usr/bin,/usr/sbin";
	auto dest = util::split_str(src, ',');
	for (auto it = dest.begin(); it != dest.end(); it++) {
		std::cout << *it << std::endl;
	}

	std::cout << "Test util::rm_end_head_char" << std::endl;
	std::string src2 = "####/etc,/usr/bin,  /usr/sbin#####";
	std::cout << src2 << std::endl;
	std::cout << util::rm_end_head_char(src2, '#') << std::endl;

	std::cout << "Test util::is_number" << std::endl;
	std::string src3 = "0123456789";
	std::string src4 = "0123a456789";
	std::cout << src3 << ": " << util::is_number(src3) << std::endl;
	std::cout << src4 << ": " << util::is_number(src4) << std::endl;

	return 0;
}

#endif /* UNIT_TEST */

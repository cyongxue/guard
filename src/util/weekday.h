/*
 *  weekday.h, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年3月16日
 *      Author: yongxue@cyongxue@163.com
 */

#ifndef SRC_UTIL_WEEKDAY_H_
#define SRC_UTIL_WEEKDAY_H_

#include <string>

namespace util {

bool is_valid_weekday(const std::string& this_day);

/* weekday判断 */
class WeekDayFlag {
	public:
		enum DayFlag: uint16_t {
			SUNDAY		= 0x0001,		// 周日
			MONDAY		= 0x0002,		// 周一
			TUESDAY		= 0x0004,		// 周二
			WEDNESDAY	= 0x0010,		// 周三
			THURSDAY	= 0x0020,		// 周四
			FRIDAY		= 0x0040,		// 周五
			SATURDAY	= 0x0100,		// 周六
		};

	private:
		int32_t 	_day_flags;
		std::string	_input_str;

	public:
		/* 输入格式为','分割，切支持!区分操作；
		 * */
		WeekDayFlag(const std::string& input): _input_str(input) { _day_flags = 0;};
		~WeekDayFlag() = default;

		int parse_day_flags();
		int32_t day_flags() const { return _day_flags; };

		/* 是否是flag标记的一天 */
		bool is_on_day(const std::string& this_day) const;
		bool is_on_day(int day) const;
		std::string print_day() const;
};

}



#endif /* SRC_UTIL_WEEKDAY_H_ */

/*
 *  daytime.h, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年3月16日
 *      Author: yongxue@cyongxue@163.com
 */

#ifndef SRC_UTIL_DAYTIME_H_
#define SRC_UTIL_DAYTIME_H_

#include <vector>
#include <string>

namespace util {

/**
 * 仅支持24小时制的格式，关注hour和min
 */
class DayTime {
	private:
		std::string 	_input_str;
		std::string 	_times;

	public:
		DayTime(const std::string& input): _input_str(input) {};
		~DayTime() = default;

		int parse_time();
		bool is_after_time(const std::string& input_time);

		std::string time() const { return _times; };

};

}

#endif /* SRC_UTIL_DAYTIME_H_ */

/*
 *  daytime.cpp, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年3月16日
 *      Author: yongxue@cyongxue@163.com
 */

#include "util.h"
#include "daytime.h"

namespace util {

int DayTime::parse_time() {
	if (_input_str.empty()) {
		return -1;
	}

	int ng = 0;
	auto tmp_time = rm_end_head_char(_input_str, ' ');
	if (tmp_time[0] == '!') {
		ng = 1;
		tmp_time = tmp_time.substr(1);
	}

	int hour_flag = 1;
	int hour = 0;
	int min = 0;
	auto hour_min = util::split_str(tmp_time, ':');
	for (auto it = hour_min.begin(); it != hour_min.end(); it++) {
		if (!util::is_number(*it)) {
			return -1;
		}

		if (hour_flag == 1) {
			hour = std::stoi(*it);
			if (hour < 0 || hour >= 24) {
				return -1;
			}
			hour_flag = 0;
		}
		else if (hour_flag == 0) {
			min = std::stoi(*it);
			if (min < 0 || min >= 60) {
				return -1;
			}
		}
	}

	char tmp[20];
	memset(tmp, 0, sizeof(tmp));
	snprintf(tmp, sizeof(tmp), "%c%02d:%02d", ng == 0 ? '.' : '!', hour, min);
	_times = tmp;

	return 0;
}

bool DayTime::is_after_time(const std::string& input_time) {
	if (_times[0] == '!') {
		return false;
	}

	if (input_time >= _times.substr(1)) {
		return true;
	}
	return false;
}
}



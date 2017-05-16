/*
 *  test_regex.cpp, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年3月15日
 *      Author: yongxue@cyongxue@163.com
 */

#include <regex>
#include <vector>
#include <iostream>

int main() {

	std::cout << "test regex icase:" << std::endl;

	std::regex cpp_regex("(\\w)+\\.(cpp|hpp)$", std::regex::icase);
//	std::regex cpp_regex;

	std::vector<std::string> test_filename = {
		"regex.cpp", "iostream.h", "template.CPP", "class.hPP", "Ah, regex.cpp", "regex.cpp Ah"
	};

	for (auto it = test_filename.begin(); it != test_filename.end(); it++) {
		if (std::regex_match(*it, cpp_regex)) {
			std::cout << *it << std::endl;
		}
	}

	return 0;
}



/*
 *  util.cpp, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年3月2日
 *      Author: yongxue@cyongxue@163.com
 */

#include "util.h"
#include "log.h"

Chroot* Chroot::_instance = nullptr;

static Chroot* Chroot::instance(std::string server_name, std::string base_dir) {
	if (_instance == nullptr) {
		_instance = new Chroot(server_name, base_dir);
	}
	return _instance;
}

std::string Chroot::full_file_path(const std::string file_name) {

	std::string ret_file_name;

	if (file_name.empty()) {
		fatal(_server_name.c_str(), "Input file_name is empty.");
	}

	if (_is_chroot) {
		ret_file_name = file_name;
	}
	else {
		ret_file_name = _base_dir + file_name;
	}

	if (ret_file_name.empty()) {
		fatal(_server_name.c_str(), "Get full file path Failed from chroot");
	}
	return ret_file_name;
}

int Chroot::do_chroot() {
	_is_chroot = true;

	if (chdir(_base_dir.c_str()) < 0) {
		return -1;
	}

	if (chroot(_base_dir.c_str()) < 0) {
		return -1;
	}

	if (chdir("/") < 0) {
		return -1;
	}
	return 0;
}

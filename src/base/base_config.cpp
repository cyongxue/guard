/*
 *  base_config.cpp, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年3月4日
 *      Author: yongxue@cyongxue@163.com
 */

#include <cstdlib>

#include "base_config.h"

std::string BaseConfig::read_opt_file(const std::string high_name, const std::string low_name, const std::string file) {
	std::string ret = "";

	FILE *fp = fopen(file.c_str(), "r");
	if (!fp) {
		error(_server_name.c_str(), "Could not open file '%s' due to [(%d)-(%s)].", file.c_str(), errno, strerror(errno));
		return ret;
	}

	if (high_name.empty() || low_name.empty()) {
		error(_server_name.c_str(), "Attempted to use null string. ");
		fclose(fp);
		return ret;
	}

	char *buf_ptr;
	char *low_ptr;
	char buf[OS_SIZE_1024] = '\0';
	while(fgets(buf, OS_SIZE_1024, fp) != NULL) {
		if (buf[0] == '#' || buf[0] == ' ' || buf[0] == '\n') {
			continue;
		}

		buf_ptr = strchr(buf, '.');
		if (!buf_ptr) {
			error(_server_name.c_str(), "Invalid line on file '%s': %s.", file.c_str(), buf);
			continue;
		}

		*buf_ptr = '\0';
		buf_ptr++;
		if(strcmp(buf, high_name.c_str()) != 0) {
			continue;
		}

		low_ptr = buf_ptr;
		buf_ptr = strchr(buf_ptr, '=');
		if (!buf_ptr) {
			error(_server_name.c_str(), "Invalid line on file '%s': %s.", file.c_str(), buf);
			continue;
		}

		*buf_ptr = '\0';
		buf_ptr++;
		if (strcmp(low_ptr, low_name.c_str()) != 0) {
			continue;
		}

		low_ptr = strrchr(buf_ptr, '\n');
		if (low_ptr) {
			*low_ptr = '\0';
		}
		low_ptr = strrchr(buf_ptr, '\r');
		if (low_ptr) {
			*low_ptr = '\0';
		}

		ret = std::string(buf_ptr);
	}

	fclose(fp);
	return ret;
}

int BaseConfig::get_define_int(const std::string high_name, const std::string low_name, int min, int max) {
	std::string full_path = Chroot::instance()->full_file_path(_opt_file);

	std::string value = read_opt_file(high_name, low_name, full_path);
	if (value.empty()) {
		fatal(_server_name.c_str(), "Definition not found for: '%s.%s'.", high_name.c_str(), low_name.c_str());
	}

	for (auto it = value.begin(); it != value.end(); it++) {
		if (!isdigit(*it)) {
			fatal(_server_name.c_str(), "Invalid definition for %s.%s: '%s'.", high_name.c_str(), low_name.c_str(), value.c_str());
		}
	}

	auto ret = atoi(value.c_str());
	if ((ret < min) || (ret > max)) {
		fatal(_server_name.c_str(), "Invalid definition for %s.%s: '%s'.", high_name.c_str(), low_name.c_str(), value.c_str());
	}

	return ret;
}

int BaseConfig::get_define_int(const std::string high_name, const std::string low_name, int min, int max, int def_value) {
	std::string full_path = Chroot::instance()->full_file_path(_opt_file);

	std::string value = read_opt_file(high_name, low_name, full_path);
	if (value.empty()) {
		error(_server_name.c_str(), "Definition not found for: '%s.%s'.", high_name.c_str(), low_name.c_str());
		return def_value;
	}

	for (auto it = value.begin(); it != value.end(); it++) {
		if (!isdigit(*it)) {
			error(_server_name.c_str(), "Invalid definition for %s.%s: '%s'.", high_name.c_str(), low_name.c_str(), value.c_str());
			return def_value;
		}
	}

	auto ret = atoi(value.c_str());
	if ((ret < min) || (ret > max)) {
		error(_server_name.c_str(), "Invalid definition for %s.%s: '%s'.", high_name.c_str(), low_name.c_str(), value.c_str());
		return def_value;
	}

	return ret;
}

std::string BaseConfig::get_define_str(const std::string high_name, const std::string low_name) {
	std::string full_path = Chroot::instance()->full_file_path(_opt_file);

	auto value = read_opt_file(high_name, low_name, full_path);
	if (value.empty()) {
		fatal(_server_name.c_str(), "Definition not found for: '%s.%s'.", high_name.c_str(), low_name.c_str());
	}

	return value;
}

std::string BaseConfig::get_define_str(const std::string high_name, const std::string low_name, const std::string def_value) {
	std::string full_path = Chroot::instance()->full_file_path(_opt_file);

	auto value = read_opt_file(high_name, low_name, full_path);
	if (value.empty()) {
		error(_server_name.c_str(), "Definition not found for: '%s.%s'.", high_name.c_str(), low_name.c_str());
	}
	else {
		value = def_value;
	}

	return value;
}






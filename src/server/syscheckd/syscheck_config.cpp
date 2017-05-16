/*
 *  syscheck_config.cpp, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年3月14日
 *      Author: yongxue@cyongxue@163.com
 */

#include <glob.h>
#include <memory>

#include "defs.h"
#include "util.h"
#include "syscheck_config.h"
#include "log.h"


SyscheckConfigData::SyscheckConfigData() {
	_rootcheck = 0;
	_syscheck_disable = 0;
	_skip_nfs = 0;
	_time = 600;

	_tsleep = 20;				// 这两个暂定这样
	_sleep_after = 20;
}

bool SyscheckConfigData::is_dir_have(const std::string& dir) {

	for (auto it = _dirs_infos.begin(); it != _dirs_infos.end(); it++) {
		if (dir == it->_dir) {
			return true;
		}
	}

	return false;
}

int SyscheckConfigData::add_syscheck_entry(const std::string& dir, const util::FileCheckOpt& opts,
		int reg, const std::string& restrict_file) {
	if (reg == 1) {
		// todo: 支持windows功能
	}
	else {
		if (!restrict_file.empty()) {
			// regex相关：http://blog.jobbole.com/105606/
			auto ptr = std::make_shared<std::regex>(restrict_file);
			if (ptr != nullptr) {
				_dirs_infos.push_back(DirInfo(opts, dir, ptr));
			}
			else {
				error("SyscheckConfigData", "make shared 'std::regex' failed.");
				_dirs_infos.push_back(DirInfo(opts, dir));
			}
		}
		else {
			_dirs_infos.push_back(DirInfo(opts, dir));
		}
	}

	return 0;
}

/**
 * return 	-1  失败
 * 			0	成功
 */
int SyscheckConfigData::set_prefilter_cmd(const std::string& input) {
	if (input.empty()) {
		return -1;
	}

	std::string new_input;
	auto index = input.find_first_of(' ');
	if (index != std::string::npos) {
		new_input = input.substr(0, index);
	}
	else {
		new_input = input;
	}

	struct stat statbuf;
	if (stat(new_input.c_str(), &statbuf) == 0) {
		_prefilter_cmd = new_input;
	}
	else {
		return -1;
	}

	return 0;
}

bool SyscheckConfigData::is_in_vector(const std::vector<std::string>& vec, const std::string& str) const {
	for (auto it = vec.begin(); it != vec.end(); it++) {
		if (*it == str) {
			return true;
		}
	}
	return false;
}


/**
 * return 	-1  失败
 * 			0	成功
 */
int SyscheckConfigData::set_nodiffs(const std::string& content, const std::vector<std::string>& attributes, const std::vector<std::string>& values) {
	if (!attributes.empty() && !values.empty()) {
		/* 取第一个就行 */
		if ((attributes[0] == "type") || (values[0] == "sregex")) {
			_nodiff_regexs.push_back(std::regex(content));
		}
	}
	else {
		if (!is_in_vector(_nodiffs, content)) {
			_nodiffs.push_back(content);
		}
	}

	return 0;
}

/**
 * return 	-1  失败
 * 			0	成功
 */
int SyscheckConfigData::set_ignores(const std::string& content, const std::vector<std::string>& attributes, const std::vector<std::string>& values) {
	if (!attributes.empty() && !values.empty()) {
		/* 取第一个就行 */
		if ((attributes[0] == "type") || (values[0] == "sregex")) {
			_ignore_regexs.push_back(std::regex(content));
		}
	}
	else {
		if (!is_in_vector(_ignores, content)) {
			_ignores.push_back(content);
		}
	}

	return 0;
}


bool SyscheckConfigData::is_have_dir() const {
	if (_dirs_infos.empty()) {
		return false;
	}
	return true;
}


/**
 * return -1		失败
 * 		  0			成功
 */
int SyscheckConfig::parse_dir_opts(const std::vector<std::string>& attributes,
		const std::vector<std::string>& values, util::FileCheckOpt& opts, std::string& restrict_file) {

	auto attr_it = attributes.begin();
	auto value_it = values.begin();
	while ((attr_it != attributes.end()) && (value_it != values.end())) {
		if (*attr_it == "check_all") {
			if (*value_it == "yes") {
				opts.add_opt(util::FileCheckOpt::CHECK_MD5SUM);
				opts.add_opt(util::FileCheckOpt::CHECK_SHA1SUM);
				opts.add_opt(util::FileCheckOpt::CHECK_PERM);
				opts.add_opt(util::FileCheckOpt::CHECK_SIZE);
				opts.add_opt(util::FileCheckOpt::CHECK_OWNER);
				opts.add_opt(util::FileCheckOpt::CHECK_GROUP);
			}
			else if (*value_it == "no") {
				opts.del_opt(util::FileCheckOpt::CHECK_MD5SUM | util::FileCheckOpt::CHECK_SHA1SUM
						| util::FileCheckOpt::CHECK_PERM | util::FileCheckOpt::CHECK_SIZE
						| util::FileCheckOpt::CHECK_OWNER | util::FileCheckOpt::CHECK_GROUP);
			}
			else {
				error(_server_name.c_str(), "Invalid option '%s' for attribute '%s'", attr_it->c_str(), value_it->c_str());
				return -1;
			}
		}
		else if (*attr_it == "check_sum") {
			if (*value_it == "yes") {
				opts.add_opt(util::FileCheckOpt::CHECK_MD5SUM);
				opts.add_opt(util::FileCheckOpt::CHECK_SHA1SUM);
			}
			else if (*value_it == "no") {
				opts.del_opt(util::FileCheckOpt::CHECK_MD5SUM | util::FileCheckOpt::CHECK_SHA1SUM);
			}
			else {
				error(_server_name.c_str(), "Invalid option '%s' for attribute '%s'", attr_it->c_str(), value_it->c_str());
				return -1;
			}
		}
		else if (*attr_it == "check_md5sum") {
			if (*value_it == "yes") {
				opts.add_opt(util::FileCheckOpt::CHECK_MD5SUM);
			}
			else if (*value_it == "no") {
				opts.del_opt(util::FileCheckOpt::CHECK_MD5SUM);
			}
			else {
				error(_server_name.c_str(), "Invalid option '%s' for attribute '%s'", attr_it->c_str(), value_it->c_str());
				return -1;
			}
		}
		else if (*attr_it == "check_sha1sum") {
			if (*value_it == "yes") {
				opts.add_opt(util::FileCheckOpt::CHECK_SHA1SUM);
			}
			else if (*value_it == "no") {
				opts.del_opt(util::FileCheckOpt::CHECK_SHA1SUM);
			}
			else {
				error(_server_name.c_str(), "Invalid option '%s' for attribute '%s'", attr_it->c_str(), value_it->c_str());
				return -1;
			}
		}
		else if (*attr_it == "check_perm") {
			if (*value_it == "yes") {
				opts.add_opt(util::FileCheckOpt::CHECK_PERM);
			}
			else if (*value_it == "no") {
				opts.del_opt(util::FileCheckOpt::CHECK_PERM);
			}
			else {
				error(_server_name.c_str(), "Invalid option '%s' for attribute '%s'", attr_it->c_str(), value_it->c_str());
				return -1;
			}
		}
		else if (*attr_it == "check_size") {
			if (*value_it == "yes") {
				opts.add_opt(util::FileCheckOpt::CHECK_SIZE);
			}
			else if (*value_it == "no") {
				opts.del_opt(util::FileCheckOpt::CHECK_SIZE);
			}
			else {
				error(_server_name.c_str(), "Invalid option '%s' for attribute '%s'", attr_it->c_str(), value_it->c_str());
				return -1;
			}
		}
		else if (*attr_it == "check_owner") {
			if (*value_it == "yes") {
				opts.add_opt(util::FileCheckOpt::CHECK_OWNER);
			}
			else if (*value_it == "no") {
				opts.del_opt(util::FileCheckOpt::CHECK_OWNER);
			}
			else {
				error(_server_name.c_str(), "Invalid option '%s' for attribute '%s'", attr_it->c_str(), value_it->c_str());
				return -1;
			}
		}
		else if (*attr_it == "check_group") {
			if (*value_it == "yes") {
				opts.add_opt(util::FileCheckOpt::CHECK_GROUP);
			}
			else if (*value_it == "no") {
				opts.del_opt(util::FileCheckOpt::CHECK_GROUP);
			}
			else {
				error(_server_name.c_str(), "Invalid option '%s' for attribute '%s'", attr_it->c_str(), value_it->c_str());
				return -1;
			}
		}
		else if (*attr_it == "realtime") {
			if (*value_it == "yes") {
				opts.add_opt(util::FileCheckOpt::CHECK_REALTIME);
			}
			else if (*value_it == "no") {
				opts.del_opt(util::FileCheckOpt::CHECK_REALTIME);
			}
			else {
				error(_server_name.c_str(), "Invalid option '%s' for attribute '%s'", attr_it->c_str(), value_it->c_str());
				return -1;
			}
		}
		else if (*attr_it == "report_changes") {
			if (*value_it == "yes") {
				opts.add_opt(util::FileCheckOpt::CHECK_SEECHANGES);
			}
			else if (*value_it == "no") {
				opts.del_opt(util::FileCheckOpt::CHECK_SEECHANGES);
			}
			else {
				error(_server_name.c_str(), "Invalid option '%s' for attribute '%s'", attr_it->c_str(), *value_it->c_str());
				return -1;
			}
		}
		else if (*attr_it == "restrict") {
			restrict_file = *value_it;
		}
		else {
			error(_server_name.c_str(), "Invalid attribute '%s' for directory option.", attr_it->c_str());
			return -1;
		}

		attr_it++;
		value_it++;
	}

	return 0;
}

/**
 * 读取directories的配置属性
 * return -1		失败
 * 		  0			成功
 */
int SyscheckConfig::read_dir_attr(const std::string& dir_str, const std::vector<std::string>& attributes, const std::vector<std::string>& values) {

	if (dir_str.empty()) {
		return -1;
	}

	if (attributes.empty() || values.empty()) {
		warn(_server_name.c_str(), "No option provided for directories: '%s', ignoring it.", dir_str.c_str());
		return -1;
	}

	/* 属性解析 */
	util::FileCheckOpt opts;
	std::string restrict_file;
	if (parse_dir_opts(attributes, values, opts, restrict_file) == -1) {
		error(_server_name.c_str(), "Invalid options for directory '%s'.", dir_str.c_str());
		return -1;
	}
	if (opts == 0) {
		error(_server_name.c_str(), "No option provided for directories: '%s', ignoring it.", dir_str.c_str());
		return -1;
	}

	auto dirs = util::split_str(dir_str, ',');
	for (auto it = dirs.begin(); it != dirs.end(); it++) {
		/* 移除首尾的' ' */
		auto tmp_dir = util::rm_end_head_char(*it, ' ');
		if (_config_data->is_dir_have(tmp_dir)) {
			error(_server_name.c_str(), "Duplicated directory given: '%s'.", tmp_dir.c_str());
			return -1;
		}

#ifndef __MINGW32__
		if ((tmp_dir.find_first_of('*') != std::string::npos) ||
				(tmp_dir.find_first_of('?') != std::string::npos) ||
				(tmp_dir.find_first_of('[') != std::string::npos)) {
			glob_t g;
			if (glob(tmp_dir.c_str(), 0, nullptr, &g) != 0) {
				error(_server_name.c_str(), "Glob error. Invalid pattern: '%s'.", tmp_dir.c_str());
				return -1;
			}

			if (g.gl_pathv[0] == nullptr) {
				error(_server_name.c_str(), "No file found by pattern: '%s'.", tmp_dir.c_str());
				return -1;
			}

			int gindex = 0;
			while(g.gl_pathv[gindex]) {
				(void)_config_data->add_syscheck_entry(g.gl_pathv[gindex], opts, 0, restrict_file);
				gindex++;
			}
		}
		else {
			(void)_config_data->add_syscheck_entry(tmp_dir, opts, 0, restrict_file);
		}
#else
		(void)_config_data->add_syscheck_entry(tmp_dir, opts, 0, restrict_file);
#endif
	}

	return 0;
}

/* 解析具体的syscheck的配置 */
int SyscheckConfig::parse_syscheck_config(const std::vector<xml::Node>& nodes) {
	info(_server_name.c_str(), "SyscheckConfig Do parse syscheck config...");

	for (auto it = nodes.begin(); it != nodes.end(); it++) {
		if (it->_element.empty()) {
			error(_server_name.c_str(), "Invalid NULL element in the configuration.");
			return OS_INVALID;
		}
		else if (it->_content.empty()) {
			error(_server_name.c_str(), "Invalid NULL content for element: %s.", it->_element.c_str());
			return OS_INVALID;
		}
		else if (it->_element == "directories") {
			if (read_dir_attr(it->_content, it->_attributes, it->_values)) {
				return OS_INVALID;
			}
		}
		else if (it->_element == "windows_registry") {
			// todo: 待windows处理
		}
		else if (it->_element == "frequency") {
			if (!util::is_number(it->_content)) {
				error(_server_name.c_str(), "Invalid value for element '%s': %s.Error: %s", it->_element.c_str(), it->_content.c_str());
				return -1;
			}
			_config_data->set_time(std::stoi(it->_content));
		}
		else if (it->_element == "scan_time") {
			auto tmp_time = std::unique_ptr<util::DayTime>(new util::DayTime(it->_content));
			if ((tmp_time == nullptr) || (tmp_time->parse_time() == -1)) {
				error(_server_name.c_str(), "Invalid value for element '%s': %s.Error: %s", it->_element.c_str(), it->_content.c_str());
				return -1;
			}
			_config_data->set_daytime(std::move(tmp_time));
		}
		else if (it->_element == "scan_day") {
			auto tmp_day = std::unique_ptr<util::WeekDayFlag>(new util::WeekDayFlag(it->_content));
			if ((tmp_day == nullptr) || (tmp_day->parse_day_flags() == -1)) {
				error(_server_name.c_str(), "Invalid value for element '%s': %s.Error: %s", it->_element.c_str(), it->_content.c_str());
				return -1;
			}
			_config_data->set_weekday_flag(std::move(tmp_day));
		}
		else if (it->_element == "scan_on_start") {
			if (it->_content == "yes") {
				_run_data->set_scan_on_start(1);
			}
			else if (it->_content == "no") {
				_run_data->set_scan_on_start(0);
			}
			else {
				error(_server_name.c_str(), "Invalid value for element '%s': %s.Error: %s", it->_element.c_str(), it->_content.c_str());
				return -1;
			}
		}
		else if (it->_element == "disabled") {
			if (it->_content == "yes") {
				_config_data->set_syscheck_disable(1);
			}
			else if (it->_content == "no") {
				_config_data->set_syscheck_disable(0);
			}
			else {
				error(_server_name.c_str(), "Invalid value for element '%s': %s.Error: %s", it->_element.c_str(), it->_content.c_str());
				return -1;
			}
		}
		else if (it->_element == "skip_nfs") {
			if (it->_content == "yes") {
				_config_data->set_skip_nfs(1);
			}
			else if (it->_content == "no") {
				_config_data->set_skip_nfs(0);
			}
			else {
				error(_server_name.c_str(), "Invalid value for element '%s': %s.Error: %s", it->_element.c_str(), it->_content.c_str());
				return -1;
			}
		}
		else if (it->_element == "ignore") {
			_config_data->set_ignores(it->_content, it->_attributes, it->_values);
		}
		else if (it->_element == "registry_ignore") {
			// todo: 待windows相关的处理
		}
		else if (it->_element == "nodiff") {
			_config_data->set_nodiffs(it->_content, it->_attributes, it->_values);
		}
		else if (it->_element == "auto_ignore") {
			/* auto_ignore is not read here */
		}
		else if (it->_element == "alert_new_files") {
			/* alert_new_files option is not read here */
		}
		else if (it->_element == "prefilter_cmd") {
			if(_config_data->set_prefilter_cmd(it->_content) == -1) {
				error(_server_name.c_str(), "Invalid value for element '%s': %s.Error: %s", it->_element.c_str(), it->_content.c_str());
				return -1;
			}
		}
		else {
			error(_server_name.c_str(), "Invalid element in the configuration: '%s'.", it->_element.c_str());
			return OS_INVALID;
		}
	}

	return 0;
}

/**
 * 读取配置文件
 */
int SyscheckConfig::read_config() {
	info(_server_name.c_str(), "SyscheckConfig Read Config File ......");

	/* 读取conf文件 */
	if (read_syscheck_config() == -1) {
		error(_server_name.c_str(), "Read syscheck config faild.");
		return -1;
	}
	debug(_server_name.c_str(), "Read syscheck config OK.");

	/* 读取agent.conf文件 */
	if (read_agent_config() == -1) {
		error(_server_name.c_str(), "Read agent config faild.");
		return -1;
	}
	debug(_server_name.c_str(), "Read agent config OK.");

	if (!_config_data->is_have_dir()) {
		error(_server_name.c_str(), "No directory provided for syscheck to monitor.");
	}

	return 0;
}

/**
 * guard.conf的配置项
 */
int SyscheckConfig::read_syscheck_config() {
	_modules |= ServerType::SYSCHECK;

	_config_data = std::make_shared<SyscheckConfigData>();
	_run_data = std::make_shared<SyscheckRunData>();

	debug(_server_name.c_str(), "Reading Configuration [%s].", _cfg_file.c_str());

	if (util::file_mod_time(_cfg_file) < 0) {
		error(_server_name.c_str(), "No file '%s'", _cfg_file.c_str());
		return -1;
	}

	/* 读取配置文件 */
	if (read_xml_file() == -1) {
		error(_server_name.c_str(), "read xml file failed '%s'.", _cfg_file.c_str());
		return -1;
	}

	return 0;
}

/**
 * agent.conf配置是agent自定义的配置，和guard.conf中对于策略的配置项是一样
 */
int SyscheckConfig::read_agent_config() {
	_modules |= ServerType::AGENT_CONFIG;

	debug(_server_name.c_str(), "Reading Client Configuration [%s].", "agent.conf path");

	if (util::file_mod_time(_agent_conf) < 0) {
		error(_server_name.c_str(), "No file '%s'", _agent_conf.c_str());
		return -1;
	}

	if (read_xml_file() == -1) {
		error(_server_name.c_str(), "read xml file failed '%s'.", _agent_conf.c_str());
		return -1;
	}

	return 0;
}

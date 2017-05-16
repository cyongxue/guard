/*
 *  rootcheck_config.cpp, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年3月18日
 *      Author: yongxue@cyongxue@163.com
 */

#include "rootcheck_config.h"

RkConfigData::RkConfigData() {
	_daemon = 1;
	_notify = BaseServer::NotifyType::QUEUE;

	_scan_all = 0;
	_read_all = 0;
	_disabled = 0;
	_skip_nfs = 0;
	_time = ROOTCHECK_WAIT;

	_fp = nullptr;
}

void RkConfigData::add_unix_audit(const std::string& unix_audit) {
	if (unix_audit.size()) {
		_unix_audit.push_back(unix_audit);
	}
	return;
}

void RkConfigData::add_ignore(const std::string& ignore) {
	if (ignore.size()) {
		_ignore.push_back(ignore);
	}
	return;
}

/**
 * 增加一个对象
 */
void RkRunData::add_rk_file(const std::string& file, const std::string& name) {
	RkSysFile one_file;
	one_file._index = _rk_count;
	one_file._rk_file = file;
	one_file._rk_name = name;

	_rk_files.push_back(one_file);
	_rk_count++;
	return;
}

/**
 * RkRunData::AlertMsgPool中操作
 * 在queue的end尝试add一个msg
 */
bool RkRunData::AlertMsgPool::is_have(const std::string& msg) const {
	for (auto const& one: _alert_msg) {
		if (one == msg) {
			return true;
		}
	}
	return false;
}

/**
 * 向queue中加入一个alert msg
 * 在queue的开头删除一个，并将删除对象返回
 */
int RkRunData::AlertMsgPool::add_end(const std::string& msg) {
	if (is_have(msg)) {
		return 0;
	}

	if (_alert_msg.size() >= MAX_ALERT_MSG_COUNT) {
		return -1;
	}

	_alert_msg.push(msg);
	return 0;
}

const std::string& RkRunData::AlertMsgPool::pop_head() {
	if (_alert_msg.empty()) {
		return "";
	}

	auto msg = _alert_msg.front();
	_alert_msg.pop();
	return msg;
}

void RkRunData::AlertMsgPool::clear_queue() {
	while (!_alert_msg.empty()) {
		_alert_msg.pop();
	}
	return;
}

int RkConfig::read_config() {
	info(_server_name.c_str(), "RootcheckConfig Read Config File ......");

	/* guard.conf */
	if (read_rootcheck_config() == -1) {
		error(_server_name.c_str(), "Read rootcheck config failed ......");
		return -1;
	}
	debug(_server_name.c_str(), "Read rootcheck config OK!!!");

	/* agent.conf */
	if (read_agent_config() == -1) {
		error(_server_name.c_str(), "Read agent config failed ...");
		return -1;
	}
	debug(_server_name.c_str(), "Read agent config OK");

	// todo: 可能存在操作

	return 0;
}

/**
 * guard.conf的配置
 */
int RkConfig::read_rootcheck_config() {
	_modules != ServerType::ROOTCHECK;

	_config_data = std::make_shared<RkConfigData>();

	debug(_server_name.c_str(), "Reading Configuration [%s].", _cfg_file.c_str());

	if (util::file_mod_time(_cfg_file) < 0) {
		error(_server_name.c_str(), "No file '%s'.", _cfg_file.c_str());
		return -1;
	}

	if (read_xml_file() == -1) {
		error(_server_name.c_str(), "read xml file failed '%s'.", _cfg_file.c_str());
		return -1;
	}

	return 0;
}

/**
 * agent.conf的配置
 */
int RkConfig::read_agent_config() {
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

/*  */
int RkConfig::parse_rootcheck_config(const std::vector<xml::Node>& nodes) {
	info(_server_name.c_str(), "Rootcheck Do parse rootcheck config ...");

	for (auto it = nodes.begin(); it != nodes.end(); it++) {
		if (it->_element.empty()) {
			error(_server_name.c_str(), "Invalid NULL element in the configuration.");
			return -1;
		}
		else if (it->_content.empty()) {
			error(_server_name.c_str(), "Invalid NULL content for element: %s.", it->_element);
			return -1;
		}

		if (it->_element == "frequency") {
			if (!util::is_number(it->_content)) {
				error(_server_name.c_str(), "Invalid value for element '%s': %s.", it->_element.c_str(), it->_content.c_str());
				return -1;
			}
			_config_data->set_time(atoi(it->_content.c_str()));
		}
		else if (it->_element == "scanall") {
			auto tmp = util::eval_bool(it->_content);
			if (tmp == -1) {
				error(_server_name.c_str(), "Invalid value for element '%s': %s.", it->_element.c_str(), it->_content.c_str());
				return -1;
			}
			_config_data->set_scan_all((bool)tmp);
		}
		else if (it->_element == "disabled") {
			auto tmp = util::eval_bool(it->_content);
			if (tmp == -1) {
				error(_server_name.c_str(), "Invalid value for element '%s': %s.", it->_element.c_str(), it->_content.c_str());
				return -1;
			}
			_config_data->set_disable((bool)tmp);
		}
		else if (it->_element == "skip_nfs") {
			auto tmp = util::eval_bool(it->_content);
			if (tmp == -1) {
				error(_server_name.c_str(), "Invalid value for element '%s': %s.", it->_element.c_str(), it->_content.c_str());
				return -1;
			}
			_config_data->set_skip_nfs((bool)tmp);
		}
		else if (it->_element == "readall") {
			auto tmp = util::eval_bool(it->_content);
			if (tmp == -1) {
				error(_server_name.c_str(), "Invalid value for element '%s': %s.", it->_element.c_str(), it->_content.c_str());
				return -1;
			}
			_config_data->set_read_all(tmp);
		}
		else if (it->_element == "rootkit_files") {
			_config_data->set_rootkit_file(it->_content);
		}
		else if (it->_element == "rootkit_trojans") {
			_config_data->set_rootkit_trojans(it->_content);
		}
		else if (it->_element == "windows_audit") {
			_config_data->set_win_audit(it->_content);
		}
		else if (it->_element == "system_audit") {
			_config_data->add_unix_audit(it->_content);
		}
		else if (it->_element == "ignore") {
			_config_data->add_ignore(it->_content);
		}
		else if (it->_element == "windows_malware") {
			_config_data->set_win_malware(it->_content);
		}
		else if (it->_element == "windows_apps") {
			_config_data->set_win_apps(it->_content);
		}
		else if (it->_element == "base_directory") {
			_config_data->set_basedir(it->_content);
		}
		else if (it->_element == "check_dev") {
			auto tmp = util::eval_bool(it->_content);
			if (tmp == -1) {
				error(_server_name.c_str(), "Invalid value for element '%s': %s.", it->_element.c_str(), it->_content.c_str());
				return -1;
			}
			_config_data->set_checkflags_dev(tmp);
		}
		else if (it->_element == "check_files") {
			auto tmp = util::eval_bool(it->_content);
			if (tmp == -1) {
				error(_server_name.c_str(), "Invalid value for element '%s': %s.", it->_element.c_str(), it->_content.c_str());
				return -1;
			}
			_config_data->set_checkflags_files(tmp);
		}
		else if (it->_element == "check_if") {
			auto tmp = util::eval_bool(it->_content);
			if (tmp == -1) {
				error(_server_name.c_str(), "Invalid value for element '%s': %s.", it->_element.c_str(), it->_content.c_str());
				return -1;
			}
			_config_data->set_checkflags_if(tmp);
		}
		else if (it->_element == "check_pids") {
			auto tmp = util::eval_bool(it->_content);
			if (tmp == -1) {
				error(_server_name.c_str(), "Invalid value for element '%s': %s.", it->_element.c_str(), it->_content.c_str());
				return -1;
			}
			_config_data->set_checkflags_pid(tmp);
		}
		else if (it->_element == "check_ports") {
			auto tmp = util::eval_bool(it->_content);
			if (tmp == -1) {
				error(_server_name.c_str(), "Invalid value for element '%s': %s.", it->_element.c_str(), it->_content.c_str());
				return -1;
			}
			_config_data->set_checkflags_port(tmp);
		}
		else if (it->_element == "check_sys") {
			auto tmp = util::eval_bool(it->_content);
			if (tmp == -1) {
				error(_server_name.c_str(), "Invalid value for element '%s': %s.", it->_element.c_str(), it->_content.c_str());
				return -1;
			}
			_config_data->set_checkflags_sys(tmp);
		}
		else if (it->_element == "check_trojans") {
			auto tmp = util::eval_bool(it->_content);
			if (tmp == -1) {
				error(_server_name.c_str(), "Invalid value for element '%s': %s.", it->_element.c_str(), it->_content.c_str());
				return -1;
			}
			_config_data->set_checkflags_trojans(tmp);
		}
		else if (it->_element == "check_unixaudit") {
			auto tmp = util::eval_bool(it->_content);
			if (tmp == -1) {
				error(_server_name.c_str(), "Invalid value for element '%s': %s.", it->_element.c_str(), it->_content.c_str());
				return -1;
			}
			_config_data->set_checkflags_unixaudix(tmp);
		}
		else if (it->_element == "check_winapps") {
#ifdef WIN
			auto tmp = util::eval_bool(it->_content);
			if (tmp == -1) {
				error(_server_name.c_str(), "Invalid value for element '%s': %s.", it->_element.c_str(), it->_content.c_str());
				return -1;
			}
			_config_data->set_checkflags_winapps(tmp);
#endif
		}
		else if (it->_element == "check_winaudit") {
#ifdef WIN
			auto tmp = util::eval_bool(it->_content);
			if (tmp == -1) {
				error(_server_name.c_str(), "Invalid value for element '%s': %s.", it->_element.c_str(), it->_content.c_str());
				return -1;
			}
			_config_data->set_checkflags_winaudit(tmp);
#endif
		}
		else if (it->_element == "check_winmalware") {
#ifdef WIN
			auto tmp = util::eval_bool(it->_content);
			if (tmp == -1) {
				error(_server_name.c_str(), "Invalid value for element '%s': %s.", it->_element.c_str(), it->_content.c_str());
				return -1;
			}
			_config_data->set_checkflags_winmalware(tmp);
#endif
		}
		else {
			error(_server_name.c_str(), "Invalid element in the configuration: '%s'.", it->_element);
			return -1;
		}
	}

	return 0;
}

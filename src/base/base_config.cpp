/*
 *  base_config.cpp, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年3月4日
 *      Author: yongxue@cyongxue@163.com
 */

#include <cstdlib>
#include <regex>

#include "agent_info.h"
#include "base_config.h"

int BaseConfig::read_main_elements(const xml::Xml& xml_in, const std::vector<xml::Node>& nodes) {
	for (auto it = nodes.begin(); it != nodes.end(); it++) {
		auto child_nodes = xml_in.get_element_by_node(*it);
		if (child_nodes.empty()) {
			error(_server_name.c_str(), "Invalid element in the configuration: '%s'.", it->_element.c_str());
			return -1;
		}

		if (it->_element.empty()) {
			error(_server_name.c_str(), "Invalid NULL element in the configuration.");
			return -1;
		}
		else if (it->_element == "global") {
			if ((_modules & ServerType::GLOBAL) || (_modules & ServerType::MAIL)) {
				// todo: parse_global_config;
			}
		}
		else if (it->_element == "email_alerts") {
			if (_modules & ServerType::MAIL) {
				// todo:: parse_email_alerts_config;
			}
		}
		else if (it->_element == "database_output") {
			if (_modules & ServerType::DBD) {
				// todo: parse_db_config;
			}
		}
		else if (it->_element == "syslog_output") {
			if (_modules & ServerType::SYSLOGD) {
				// todo: parse_syslog_config;
			}
		}
		else if (it->_element == "rules") {
			if (_modules & ServerType::RULES) {
				// todo: parse_rules_config;
			}
		}
		else if (it->_element == "syscheck") {
			if (_modules & ServerType::SYSCHECK) {
				if (parse_syscheck_config(child_nodes) < 0) {
					error(_server_name.c_str(), "parse syscheck config failed.");
					return -1;
				}
			}
		}
		else if (it->_element == "rootcheck") {
			if (_modules & ServerType::ROOTCHECK) {
				if (parse_rootcheck_config(child_nodes) < 0) {
					error(_server_name.c_str(), "parse rootcheck config failed.");
					return -1;
				}
			}
		}
		else if (it->_element == "alerts") {
			if (_modules & ServerType::ALERTS) {
				// todo: parse_alerts_config;
			}
		}
		else if (it->_element == "localfile") {
			if (_modules & ServerType::LOCALFILE) {
				// todo: parse_localfile_config;
			}
		}
		else if (it->_element == "remote") {
			if (_modules & ServerType::REMOTE) {
				// todo: parse_remote_config;
			}
		}
		else if (it->_element == "client") {
			if (_modules & ServerType::CLIENT) {
				// todo: parse_client_config;  agent.conf中的部分配置可能从这里读取
			}
		}
		else if (it->_element == "command") {
			if (_modules & ServerType::AR) {
				// todo: parse_ar_config;
			}
		}
		else if (it->_element == "active-response") {
			if (_modules & ServerType::AR) {
				// todo: parse_ar_config
			}
		}
		else if (it->_element == "reports") {
			if (_modules & ServerType::REPORTS) {
				// todo: parse_report_config;
			}
		}
		else {
			error(_server_name.c_str(), "Invalid element in the configuration: '%s'.", it->_element.c_str());
			return -1;
		}
	}

	return 0;
}

int BaseConfig::parse_rootcheck_config(const std::vector<xml::Node>& nodes) {
	info(_server_name.c_str(), "BaseConfig Do parse rootcheck config...");
	return 0;
}

int BaseConfig::parse_syscheck_config(const std::vector<xml::Node>& nodes) {
	info(_server_name.c_str(), "BaseConfig Do parse syscheck config...");
	return 0;
}

int BaseConfig::parse_client_config(const std::vector<xml::Node>& nodes) {
	info(_server_name.c_str(), "BaseConfig Do parse client config...");;
	return 0;
}

int BaseConfig::read_xml_file() {

	xml::Xml xml_in(_cfg_file);

	if (xml_in.read_xml() < 0) {
		error(_server_name.c_str(), "Error reading XML file '%s': %s (line %d)",
				_cfg_file.c_str(), xml_in.err().c_str(), xml_in.err_line());
		return OS_INVALID;
	}

	xml::Node xml_node(NODE_INVALID);
	auto nodes = xml_in.get_element_by_node(xml_node);
	if (!nodes.size()) {
		warn(_server_name.c_str(), "No config element.");
		return 0;
	}

	for (auto it = nodes.begin(); it != nodes.end(); it++) {
		if (it->_element.empty()) {
			error(_server_name.c_str(), "Invalid NULL element in the configuration.");
			return OS_INVALID;
		}
		else if (!(_modules & ServerType::AGENT_CONFIG) && (it->_element == "guard_config")) {
			/* 非agent配置处理 */
			auto child_nodes = xml_in.get_element_by_node(*it);
			if (child_nodes.size()) {
				if (read_main_elements(xml_in, child_nodes) < 0) {
					error(_server_name.c_str(), "Configuration error at '%s'. Exiting.", _cfg_file.c_str());
					return OS_INVALID;
				}
			}
		}
		else if ((_modules & ServerType::AGENT_CONFIG) && (it->_element == "agent_config")) {
			/* agent配置处理 */
			if (parse_agent_config(xml_in, *it) < 0) {
				error(_server_name.c_str(), "parse agent config failed: '%s'.", _agent_conf.c_str());
				return OS_INVALID;
			}
		}
		else {
			error(_server_name.c_str(), "Invalid element in the configuration: '%s'.", it->_element);
			return OS_INVALID;
		}
		debug(_server_name.c_str(), "The element: '%s'.", it->_element.c_str());

		it++;
	}

	return 0;
}

int BaseConfig::parse_agent_config(const xml::Xml& xml_in, const xml::Node& node) {
	int passed_agent_test = 1;
	AgentInfo agent_info(GUARD_INFO_FILE);

	if (node._attributes.size() && node._values.size()) {

		auto attr_it = node._attributes.begin();
		auto val_it = node._values.begin();
		for (; (attr_it != node._attributes.end()) && (val_it != node._values.end()); ) {

			if (*attr_it == "guard_id") {
				auto guard_id = agent_info.agent_guard_id();
				if (guard_id.size()) {
					std::regex id_regex(guard_id);
					if (std::regex_match(*val_it, id_regex)) {
						passed_agent_test = 1;
					}
					else {
						warn(_server_name.c_str(), "input guard_id not match file guard_id.");
						passed_agent_test = 0;
					}
				}
				else {
					passed_agent_test = 0;
				}
			}
			else if (*attr_it == "guard_os") {
				auto agent_os = util::get_uname();
				if (!agent_os.empty()) {
					std::regex os_regex(agent_os);
					if (!std::regex_match(*val_it, os_regex)) {
						passed_agent_test = 0;
						warn(_server_name.c_str(), "did not Matched agent.conf os '%s'", agent_os.c_str());
					}
				}
				else {
					passed_agent_test = 0;
					error(_server_name.c_str(), "Unable to retrieve uname.");
				}
			}
			else if (*attr_it == "profile") {
				auto profile = agent_info.agent_profile();
				if (profile.size()) {
					std::regex profile_reg(profile);
					if (std::regex_match(*val_it, profile_reg)) {
						passed_agent_test = 1;
						debug(_server_name.c_str(), "Matched agent.conf profile name '%s'", profile.c_str());
					}
					else {
						passed_agent_test = 0;
						warn(_server_name.c_str(), "did not Matched agent.conf profile name '%s'", profile.c_str());
					}
				}
				else {
					passed_agent_test = 0;
				}
			}
			else if (*attr_it == "overwrite") {		/* 暂不做处理 */
			}
			else {
				error(_server_name.c_str(), "Invalid attribute '%s' in the configuration: '%s'.", attr_it->c_str(), _agent_conf.c_str());
			}

			attr_it++;
			val_it++;
		}
	}
	else {
		/**
		 * agent.conf file does not have any attribute，so check if agent has a profile.
		 * if agent has a profile, then read guard.conf continue;
		 * if agent does not have a profile, then read block;
		 */
		debug(_server_name.c_str(), "agent_config element does not have any attributes..");
		if (agent_info.agent_profile().size()) {
			passed_agent_test = 1;
		}
		else {
			passed_agent_test = 0;
			warn(_server_name.c_str(), "agent does not hava profile name.");
		}
	}

	// parse main elements again
	auto child_node = xml_in.get_element_by_node(node);
	if (!child_node.empty()) {
		if (passed_agent_test && (read_main_elements(xml_in, child_node) < 0)) {
			error(_server_name.c_str(), "Configuration error at '%s'. Exiting.", _agent_conf.c_str());
			return -1;
		}
	}

	return 0;
}

std::string BaseConfig::read_opt_file(const std::string& high_name, const std::string& low_name, const std::string& file) {
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

int BaseConfig::get_define_int(const std::string& high_name, const std::string& low_name, int min, int max) {
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

int BaseConfig::get_define_int(const std::string& high_name, const std::string& low_name, int min, int max, int def_value) {
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

std::string BaseConfig::get_define_str(const std::string& high_name, const std::string& low_name) {
	std::string full_path = Chroot::instance()->full_file_path(_opt_file);

	auto value = read_opt_file(high_name, low_name, full_path);
	if (value.empty()) {
		fatal(_server_name.c_str(), "Definition not found for: '%s.%s'.", high_name.c_str(), low_name.c_str());
	}

	return value;
}

std::string BaseConfig::get_define_str(const std::string& high_name, const std::string& low_name, const std::string& def_value) {
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






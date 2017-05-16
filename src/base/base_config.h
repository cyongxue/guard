/*
 *  base_config.h, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年3月4日
 *      Author: yongxue@cyongxue@163.com
 */

#ifndef SRC_BASE_BASE_CONFIG_H_
#define SRC_BASE_BASE_CONFIG_H_

#include "defs.h"
#include "log.h"
#include "xml.h"

enum class ServerType:uint32_t {
	GLOBAL 			= 0000001,
	RULES 			= 0000002,
	SYSCHECK 		= 0000004,
	ROOTCHECK 		= 0000010,
	ALERTS 			= 0000020,
	LOCALFILE 		= 0000040,
	REMOTE 			= 0000100,
	CLIENT 			= 0000200,
	MAIL 			= 0000400,
	AR 				= 0001000,
	DBD 			= 0002000,
	SYSLOGD 		= 0004000,
	AGENTLESS 		= 0020000,
	REPORTS 		= 0040000,
	AGENT_CONFIG 	= 0010000,
};


class BaseConfig {
	private:
		std::string 		_cfg_file;
		std::string			_opt_file;
		std::string 		_agent_conf;							// agent.conf的文件名

		std::string			_server_name;

		std::shared_ptr<BaseServer>		_server;

		int _modules;						// 标记模块占用的

	public:
		BaseConfig(const std::string& server_name, const std::string& cfg_file,
				const std::string& opt_file, const std::string& agent_conf):
			_cfg_file(cfg_file), _opt_file(opt_file), _server_name(server_name), _agent_conf(agent_conf) {
			_server = nullptr;
			_modules = 0;
		}
		virtual ~BaseConfig() = default;

		std::string cfg_file() const { return _cfg_file; }
		void set_server(std::shared_ptr<BaseServer> server) { _server = server; return; }

		std::shared_ptr<BaseServer> server() const { return _server; }

		const std::string& server_name() const { return _server_name; }

		virtual int read_config() {
			info(_server_name.c_str(), "Read Config File ......");
			return 0;
		}

		/* xml 文件 */
		int read_xml_file();
		int read_main_elements(const xml::Xml& xml_in, const std::vector<xml::Node>& nodes);
		int parse_agent_config(const xml::Xml& xml_in, const xml::Node& node);

		/* option file文件 */
		std::string read_opt_file(const std::string& high_name, const std::string& low_name, const std::string& file);
		/* 取整型配置 */
		int get_define_int(const std::string& high_name, const std::string& low_name, int min, int max);
		int get_define_int(const std::string& high_name, const std::string& low_name, int min, int max, int def_value);
		/* 取string类型配置 */
		std::string get_define_str(const std::string& high_name, const std::string& low_name);
		std::string get_define_str(const std::string& high_name, const std::string& low_name, const std::string& def_value);

	protected:
		virtual int parse_rootcheck_config(const std::vector<xml::Node>& nodes);
		virtual int parse_syscheck_config(const std::vector<xml::Node>& nodes);
		virtual int parse_client_config(const std::vector<xml::Node>& nodes);
};

#endif /* SRC_BASE_BASE_CONFIG_H_ */

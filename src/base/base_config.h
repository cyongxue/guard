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

		std::string			_server_name;
		BaseServer*			_server;

	public:
		BaseConfig(const std::string cfg_file, const std::string opt_file, const std::string server_name):
			_cfg_file(cfg_file), _opt_file(opt_file), _server_name(server_name) {
			_server = nullptr;
		}
		virtual ~BaseConfig() = default;

		void set_server(BaseServer* server) {
			_server = server;
			return;
		}

		virtual int read_config() {
			info(_server_name.c_str(), "Read Config File ......");
			return 0;
		}

		std::string read_opt_file(const std::string high_name, const std::string low_name, const std::string file);
		/* 取整型配置 */
		int get_define_int(const std::string high_name, const std::string low_name, int min, int max);
		int get_define_int(const std::string high_name, const std::string low_name, int min, int max, int def_value);
		/* 取string类型配置 */
		std::string get_define_str(const std::string high_name, const std::string low_name);
		std::string get_define_str(const std::string high_name, const std::string low_name, const std::string def_value);
};

#endif /* SRC_BASE_BASE_CONFIG_H_ */

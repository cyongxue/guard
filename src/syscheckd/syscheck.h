/*
 *  syscheck.h, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年3月2日
 *      Author: yongxue@cyongxue@163.com
 */

#ifndef SRC_SYSCHECKD_SYSCHECK_H_
#define SRC_SYSCHECKD_SYSCHECK_H_

#include <vector>
#include <unordered_map>

#include "log.h"
#include "base_server.h"
#include "base_config.h"

#ifndef SERVER
#define SERVER "claw-syscheckd"
#endif

class Syscheckd : public BaseServer {
	private:


	public:
		Syscheckd():BaseServer(SERVER) {};
};

/* 考虑对于inotify的支持处理 */
class RtFim {
	private:
		int _fd;
		std::unordered_map<std::string, std::string> _dir_tb;
};

/* 配置数据 */
class SyscheckConfigData: public std::enable_shared_from_this<SyscheckConfigData> {
	private:
		uint32_t		_tsleep;
		int32_t			_sleep_after;

		int32_t			_rootcheck;
		int32_t			_syscheck_disable;

		int32_t			_skip_nfs;
		int32_t 		_time;

		std::vector<int32_t>	_opts;

		std::string		_remote_db;
		std::string		_db;

		std::string		_scan_day;
		std::string		_scan_time;

		std::vector<std::string>	_ignore;
		std::vector<std::string>	_ignore_regex;			// 正则

		std::vector<std::string>	_nodiff;
		std::vector<std::string>	_nodiff_regex;			// 正则

		std::vector<std::string>	_dir;
		std::vector<std::string>	_file_restrict;			// 正则

		std::unordered_map<std::string, std::string>	_fp;

		std::shared_ptr<RtFim>							_realtime;				// realtime时间

		std::string		_prefilter_cmd;

	public:
		SyscheckConfigData();
		~SyscheckConfigData() = default;
};

/* 运行数据 */
class SyscheckRunData: std::enable_shared_from_this<SyscheckRunData> {
	private:
		int32_t		_scan_on_start;
		int32_t		_realtime_count;

		int			_syscheck_queue;
};

/* syscheck配置文件 */
class SyscheckConfig: public BaseConfig {
	private:
		std::shared_ptr<SyscheckConfigData> _config_data;
		std::shared_ptr<SyscheckRunData> _run_data;				// 是否需要重建，比较谨慎

		int _modules;

	public:
		SyscheckConfig():BaseConfig(CLAWCONF, CLAW_DEFINES, SERVER) {
			_modules = 0;
		};

		int read_syscheck_config();
		int read_agent_config();
};


#endif /* SRC_SYSCHECKD_SYSCHECK_H_ */

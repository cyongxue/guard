/*
 *  syscheck_config.h, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年3月14日
 *      Author: yongxue@cyongxue@163.com
 */

#ifndef SRC_SERVER_SYSCHECKD_SYSCHECK_CONFIG_H_
#define SRC_SERVER_SYSCHECKD_SYSCHECK_CONFIG_H_

#include <string>
#include <stdexcept>
#include <regex>

#include "file_flag.h"
#include "net_client.h"
#include "base_config.h"

#ifndef SERVER
#define SERVER "guard-syscheckd"
#endif

/* 配置数据 */
class SyscheckConfigData: public std::enable_shared_from_this<SyscheckConfigData> {
	public:
		class DirInfo {
			public:
				util::FileCheckOpt			_opts;
				std::string					_dir;
				std::shared_ptr<std::regex>	_file_restrict;

				DirInfo(const util::FileCheckOpt& opts, const std::string& dir):
					_opts(opts), _dir(dir) {
					_file_restrict = nullptr;
				}
				DirInfo(const util::FileCheckOpt& opts, const std::string& dir, const std::shared_ptr<std::regex> file_restrict):
					_opts(opts), _dir(dir), _file_restrict(file_restrict) {}
		};

	private:
		uint32_t		_tsleep;						// sleep time
		int32_t			_sleep_after;					// how many read file do, so can sleep

		int32_t			_rootcheck;
		int32_t			_syscheck_disable;

		int32_t			_skip_nfs;
		int32_t 		_time;							/* frequency (secs) for syscheck to run */// 扫描的时间间隔

		std::string		_remote_db;
		std::string		_db;

		std::shared_ptr<util::WeekDayFlag>		_scan_day;
		std::shared_ptr<util::DayTime>			_scan_time;

		std::vector<std::string>	_ignores;
		std::vector<std::regex>		_ignore_regexs;			// 正则

		std::vector<std::string>	_nodiffs;
		std::vector<std::regex>		_nodiff_regexs;			// 正则

		std::vector<DirInfo>		_dirs_infos;

		std::string		_prefilter_cmd;

	private:
		bool is_in_vector(const std::vector<std::string>& vec, const std::string& str) const;

	public:
		SyscheckConfigData();
		~SyscheckConfigData() = default;

		bool is_dir_have(const std::string& dir);
		int add_syscheck_entry(const std::string& dir, const util::FileCheckOpt& opts,
				int reg, const std::string& restrict_file);

		void set_tsleep(uint32_t tsleep) { _tsleep = tsleep; }
		uint32_t tsleep() const { return _tsleep; }
		void set_sleep_after(uint32_t sleep_after) { _sleep_after = sleep_after; }
		uint32_t sleep_after() const { return _sleep_after; }

		void set_rootcheck(int32_t rootcheck) { _rootcheck = rootcheck; }
		int32_t rootcheck() const { return _rootcheck; }
		void set_syscheck_disable(int flag) { _syscheck_disable = flag; return; }
		int32_t syscheck_disable() const { return _syscheck_disable; }

		void set_skip_nfs(int flag) { _skip_nfs = flag; return; }
		int32_t skip_nfs() const { return _skip_nfs; }
		void set_time(int32_t time) { _time = time; return;}
		int32_t time() const { return _time; }

		void set_remote_db(const std::string& db) { _remote_db = db; }
		const std::string& remote_db() const { return _remote_db; }
		void set_db(const std::string& db) { _db = db; }
		const std::string& db() const { return _db; }

		void set_daytime(std::shared_ptr<util::DayTime> ptr) { _scan_time = ptr; return; };
		std::shared_ptr<util::DayTime> daytime() const { return _scan_time; }
		void set_weekday_flag(std::shared_ptr<util::WeekDayFlag> ptr) {  _scan_day = ptr; return; };
		std::shared_ptr<util::WeekDayFlag> weekday_flag() const { return _scan_day; }

		int set_prefilter_cmd(const std::string& input);
		const std::string& prefilter_cmd() const { return _prefilter_cmd; }

		int set_ignores(const std::string& content, const std::vector<std::string>& attributes, const std::vector<std::string>& values);
		const std::vector<std::string>& ignores() const { return _ignores; }
		const std::vector<std::regex>& ignores_regexs() const { return _ignore_regexs; }

 		int set_nodiffs(const std::string& content, const std::vector<std::string>& attributes, const std::vector<std::string>& values);
 		const std::vector<std::string>& nodiffs() const { return _nodiffs; }
		const std::vector<std::regex>& nodiffs_regexs() const { return _nodiff_regexs; }

		const std::vector<DirInfo>& dirs_infos() const { return _dirs_infos; }

		bool is_have_dir() const;
};

/* 运行数据 */
class SyscheckRunData: public std::enable_shared_from_this<SyscheckRunData>, public RunData {
	private:
		int32_t		_scan_on_start;

		int32_t		_realtime_count;
		std::shared_ptr<RtFim>					_realtime;				// realtime时间
		std::shared_ptr<FileDb>					_file_db;

		std::shared_ptr<net::UnixDomainClient>	_sys_queue;

	public:
		SyscheckRunData() {
			_scan_on_start = 1;
			_realtime_count = 0;
			_sys_queue = nullptr;
		}

		std::shared_ptr<RtFim> realtime() const { return _realtime; }
		void set_realtime(const std::shared_ptr<RtFim> realtime) { _realtime = realtime; }

		std::shared_ptr<FileDb> file_db() const { return _file_db; }
		void set_file_db(const std::shared_ptr<FileDb> file_db) { _file_db = file_db; }

		void set_scan_on_start(int flag) { _scan_on_start = flag; return; }
		int32_t scan_on_start() const { return _scan_on_start; }
		void set_realtime_count(int realtime_count) { _realtime_count = realtime_count; }
		int32_t realtime_count() const { return _realtime_count; }
		void set_sys_queue(std::shared_ptr<net::UnixDomainClient> sys_queue) {
			_sys_queue = sys_queue;
		}
		std::shared_ptr<net::UnixDomainClient> sys_queue() const { return _sys_queue; }
};

/* syscheck配置文件 */
class SyscheckConfig: public BaseConfig {
	private:
		std::shared_ptr<SyscheckConfigData> _config_data;
		std::shared_ptr<SyscheckRunData> _run_data;				// 是否需要重建，比较谨慎

	public:
		SyscheckConfig():BaseConfig(SERVER, GUARDCONF, GUARD_DEFINES, AGENT_CONF) {};

		std::shared_ptr<SyscheckConfigData> config_data() const { return _config_data; }
		std::shared_ptr<SyscheckRunData> run_data() const { return _run_data; }

		/* 重载BaseConfig中read_config */
		virtual int read_config();

		int read_syscheck_config();
		int read_agent_config();

	protected:
		/* 解析具体的syscheck的配置 */
		virtual int parse_syscheck_config(const std::vector<xml::Node>& nodes);

	private:
		/* 解析dir的监控属性 */
		int parse_dir_opts(const std::vector<std::string>& attributes, const std::vector<std::string>& values,
				util::FileCheckOpt& opts, std::string& restrict_file);
		/* 读取directories的配置属性 */
		int read_dir_attr(const std::string& dir_str, const std::vector<std::string>& attributes,
				const std::vector<std::string>& values);
};


#endif /* SRC_SERVER_SYSCHECKD_SYSCHECK_CONFIG_H_ */

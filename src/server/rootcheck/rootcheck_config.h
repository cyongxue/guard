/*
 *  rootcheck_config.h, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年3月18日
 *      Author: yongxue@cyongxue@163.com
 */

#ifndef SRC_SERVER_ROOTCHECK_ROOTCHECK_CONFIG_H_
#define SRC_SERVER_ROOTCHECK_ROOTCHECK_CONFIG_H_

#include <string>
#include <queue>

#include "defs.h"
#include "net_client.h"
#include "base_config.h"
#include "base_run.h"

#ifndef SERVER
#define SERVER "guard-rootcheck"
#endif

/* rootcheck的配置数据 config data */
class RkConfigData: public std::enable_shared_from_this<RkConfigData> {
	public:
		class ChecksFlag {
			public:
				short	_rc_dev;
				short	_rc_files;
				short	_rc_if;
				short 	_rc_pids;
				short	_rc_ports;
				short	_rc_sys;
				short	_rc_trojans;
#ifdef WIN
				short	_rc_win_audit;
				short	_rc_win_malware;
				short	_rc_win_apps;
#else
				short	_rc_unix_audit;
#endif

			public:
				ChecksFlag() {
					_rc_dev = 1;
					_rc_files = 1;
					_rc_if = 1;
					_rc_pids = 1;
					_rc_ports = 1;
					_rc_sys = 1;
					_rc_trojans = 1;
#ifdef WIN
					_rc_win_audit = 1;
					_rc_win_malware = 1;
					_rc_win_apps = 1;
#else
					_rc_unix_audit = 1;
#endif
				};
		};

	private:
		std::string 		_workdir;
		std::string 		_basedir;
		std::string			_rootkit_file;
		std::string			_rootkit_trojans;

		std::vector<std::string>	_unix_audit;
		std::vector<std::string>	_ignore;

		std::string			_win_audit;			/* audit审计 */
		std::string			_win_malware;		/* malware恶意软件 */
		std::string			_win_apps;

		FILE *	_fp;
		int 	_daemon;
		BaseServer::NotifyType _notify;

		short 	_scan_all;
		short 	_read_all;
		short 	_disabled;
		short	_skip_nfs;

		int		_time;				/* 执行rootcheck的时间间隔 */

		ChecksFlag	_checks;			/* 检查项 */

	public:
		RkConfigData();
		~RkConfigData() = default;

		void set_workdir(const std::string& workdir) { _workdir = workdir; }
		const std::string& workdir() const { return _workdir; }
		void set_basedir(const std::string& basedir) { _basedir = basedir; }
		const std::string& basedir() const { return _basedir; }
		void set_rootkit_file(const std::string& rootkit_file) { _rootkit_file = rootkit_file; }
		const std::string& rootkit_file() const { return _rootkit_file; }
		void set_rootkit_trojans(const std::string& rootkit_trojans) { _rootkit_trojans = rootkit_trojans; }
		const std::string& rootkit_trojans() const { return _rootkit_trojans; }

		void add_unix_audit(const std::string& unix_audit);
		const std::vector<std::string>& unix_audit() const { return _unix_audit; }
		void add_ignore(const std::string& ignore);
		const std::vector<std::string>& ignore() const { return _ignore; }

		void set_win_audit(const std::string& win_audit) { _win_audit = win_audit; }
		int win_audit() const { return _win_audit; }
		void set_win_malware(const std::string& win_malware) { _win_malware = win_malware; }
		int win_malware() const { return _win_malware; }
		void set_win_apps(const std::string& win_apps) { _win_apps = win_apps; }
		int win_apps() const { return _win_apps; }

		void set_fp(FILE* fp) { _fp = fp; }
		const FILE* fp() const { return _fp; }
		void set_daemon(int daemon) { _daemon = daemon; }
		int daemon() const { return _daemon; }
		void set_notify(BaseServer::NotifyType notify) { _notify = notify; }
		BaseServer::NotifyType notify() const { return _notify; }

		void set_scan_all(short scan_all) { _scan_all = scan_all; }
		short scan_all() const { return _scan_all; }
		void set_read_all(short read_all) { _read_all = read_all; }
		short read_all() const { return _read_all; }
		void set_disable(short disable) { _disabled = disable; }
		short disable() const { return _disabled; }
		void set_skip_nfs(short skip_nfs) { _skip_nfs = skip_nfs; }
		short skip_nfs() const { return _skip_nfs; }

		void set_time(int time) { _time = time; }
		int time() const { return _time; }

		void set_checkflags_dev(short dev) { _checks._rc_dev = dev; }
		short checkflags_dev() const { return _checks._rc_dev; }
		void set_checkflags_files(short files) { _checks._rc_files = files; }
		short checkflags_files() const { return _checks._rc_files; }
		void set_checkflags_if(short flag) { _checks._rc_if = flag; }
		short checkflags_if() const { return _checks._rc_if; }
		void set_checkflags_pid(short flag) { _checks._rc_pids = flag; }
		short checkflags_pid() const { return _checks._rc_pids; }
		void set_checkflags_port(short flag) { _checks._rc_ports = flag; }
		short checkflags_port() const { return _checks._rc_ports; }
		void set_checkflags_sys(short flag) { _checks._rc_sys = flag; }
		short checkflags_sys() const { return _checks._rc_sys; }
		void set_checkflags_trojans(short flag) { _checks._rc_trojans = flag; }
		short checkflags_trojans() const { return _checks._rc_trojans; }
#ifdef WIN
		void set_checkflags_winaudit(short flag) { _checks._rc_win_audit = flag; }
		short checkflags_winaudit() const { return _checks._rc_win_audit; }
		void set_checkflags_winmalware(short flag) { _checks._rc_win_malware = flag; }
		short checkflags_winmalware() const { return _checks._rc_win_malware; }
		void set_checkflags_winapps(short flag) { _checks._rc_win_apps = flag; }
		short checkflags_winapps() const { return _checks._rc_win_apps; }
#else
		void set_checkflags_unixaudix(short flag) { _checks._rc_unix_audit = flag; }
		short checkflags_unixaudix() const { return _checks._rc_unix_audit; }
#endif
};

class RkRunData: public std::enable_shared_from_this<RkRunData>, public RunData {
	public:
		/**
		 * 初步定义alert msg pool允许的容量为256
		 */
		class AlertMsgPool {
			public:
				const int MAX_ALERT_MSG_COUNT = 256;
			private:
				std::queue<std::string>		_alert_msg;
			public:
				bool is_have(const std::string& msg) const;

				int add_end(const std::string& msg);
				const std::string& pop_head();
				void clear_queue();

				const std::queue<std::string>& alert_msg() const { return _alert_msg; }
		};

	public:
		const int MAX_RK_SYS = 512;
		class RkSysFile {
			public:
				std::string _rk_file;
				std::string _rk_name;
				int 		_index;
		};

	private:
		std::vector<RkSysFile>		_rk_files;
		int							_rk_count;

		std::string				 	_udp_ports;
		std::string				 	_tcp_ports;

		std::shared_ptr<net::UnixDomainClient>	_rk_queue;

		std::shared_ptr<AlertMsgPool>			_alert_msg;

	public:
		void set_rk_queue(std::shared_ptr<net::UnixDomainClient> queue) { _rk_queue = queue; }
		std::shared_ptr<net::UnixDomainClient> rk_queue() const { return _rk_queue; }

		void init_rk_file() {
			_rk_count = 0;
			_rk_files.clear();
		}
		void clear_rk_file() {
			_rk_count = 0;
			_rk_files.clear();
		}
		int rk_count() const { return _rk_count; }
		std::vector<RkSysFile>& rk_files() const { return _rk_files; }
		void add_rk_file(const std::string& file, const std::string& name);

		std::shared_ptr<AlertMsgPool> alert_msg() const { return _alert_msg; }
};

/* rootcheck config */
class RkConfig: public BaseConfig {
	private:
		std::shared_ptr<RkConfigData>	_config_data;
		std::shared_ptr<RkRunData>		_run_data;

	public:
		RkConfig(const std::string& server_name, const std::string& cfg_config,
				const std::string& agent_conf):
			BaseConfig(server_name, cfg_config, "", agent_conf) {};
		~RkConfig() = default;

		virtual int read_config();

		int read_rootcheck_config();
		int read_agent_config();

		std::shared_ptr<RkConfigData> config_data() const { return _config_data; }
		std::shared_ptr<RkRunData> run_data() const { return _run_data; }

	protected:
		/* 具体解析rootcheck的配置 */
		virtual int parse_rootcheck_config(const std::vector<xml::Node>& nodes);
};



#endif /* SRC_SERVER_ROOTCHECK_ROOTCHECK_CONFIG_H_ */

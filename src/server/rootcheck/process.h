/*
 *  unix_process.h, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年4月11日
 *      Author: yongxue@cyongxue@163.com
 */

#ifndef SRC_SERVER_ROOTCHECK_PROCESS_H_
#define SRC_SERVER_ROOTCHECK_PROCESS_H_

#include "defs.h"
#include "base_run.h"

#ifdef WIN
// todo:
#else

class Process {
	public:
		const int _MAX_PID = 32768;
};

/**
 * todo：对于进程信息process info，后续考虑从/proc/文件中获取
 */
class ProcessInfo {
	public:
		int					_pid;				// pid id
		std::string 		_name;				// cmd line
		std::string			_path;				// exe file

	public:
		ProcessInfo(int pid, const std::string& name, const std::string& path) {
			_pid = pid;
			_name = name;
			_path = path;
		}
		ProcessInfo(int pid): _pid(pid) {};
};

/**
 * 暂时参考ossec的实现采用ps的方式处理
 */
class ProcessList {
	private:
		std::string					_ps;
		std::list<ProcessInfo>		_list;

		std::shared_ptr<RunData>	_run_data;

	private:
		std::string get_name_by_pid(int pid);

	public:
		ProcessList(std::shared_ptr<RunData> run_data): _run_data(run_data) {}

		/* 初始化的一步 */
		int set_ps();

		void add_items();

		bool is_process(const std::string& value) const;
};

class CheckPids: public Process {
	public:
		enum Position {
			PROC	= 0,				// 对应/proc目录
			PID		= 1,				// 对应/proc/pid目录
			TASK	= 2					// 对应/proc/pid/task目录
		};

	private:
		std::shared_ptr<BaseConfig>	_config;

		int		_errors;
		int		_total;

		bool	_have_proc;					// proc文件的存在性，缺省看做无

		pid_t	_my_pid;
		bool 	_proc_pid_found;			// 找到对应的pid

	private:
		void loop_all_pids(const std::string& ps);

		/*/proc/pid 目录存在性判断 */
		bool proc_stat(int pid);
		/* 利用/proc中查找pid判断 */
		bool proc_read(int pid);
		/* 切换chdir检查/proc/pid文件 */
		bool proc_chdir(int pid);

		/**
		 * read the /proc directory(if present) and
		 * check if it can find the given pid(as a pid or as a thread)
		 */
		bool check_proc(int pid);
		int read_proc_dir(const std::string& dir_name, int pid, int position);
		int read_proc_file(const std::string& file_name, int pid, int position);

	public:
		CheckPids(std::shared_ptr<BaseConfig> config): _config(config) {
			_my_pid = getpid();
			_errors = 0;
			_total = 0;
			_have_proc = false;
			_proc_pid_found = false;
		}

		void check_rc_pids();
};
#endif

#endif /* SRC_SERVER_ROOTCHECK_PROCESS_H_ */

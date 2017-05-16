/*
 *  unix_process.cpp, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年4月11日
 *      Author: yongxue@cyongxue@163.com
 */

#include "process.h"

#include "log.h"
#include "util.h"

#ifdef WIN
// todo:
#else
// todo:

#include <signal.h>

/**
 * 检查ps
 */
int ProcessList::set_ps() {
	std::string ps;

	ps = "/bin/ps";
	if (!util::is_file(ps)) {
		error("ProcessList", "%s not found", ps.c_str());
		ps = "/usr/bin/ps";
		if (!util::is_file(ps)) {
			error("ProcessList", "%s not found", ps.c_str());
			return -1;
		}
	}

	return 0;
}

/**
 * 根据pid设置name
 */
std::string ProcessList::get_name_by_pid(int pid) {
	std::string pid_name;

	char command[OS_SIZE_1024 + 1];
	memset(command, 0, sizeof(command));
	snprintf(command, OS_SIZE_1024, "%s -p %d 2> /dev/null", _ps, pid);

	char buf[OS_SIZE_2048 + 1];
	FILE *fp = popen(command, "r");
	if (fp) {
		memset(buf, 0, sizeof(buf));
		while (fgets(buf, OS_SIZE_2048, fp) != NULL) {
			char *tmp_str = strchr(buf, ':');
			if (!tmp_str) {
				memset(buf, 0, sizeof(buf));
				continue;
			}

			char *nbuf = tmp_str++;

			tmp_str = strchr(nbuf, ' ');
			if (!tmp_str) {
				memset(buf, 0, sizeof(buf));
				continue;
			}
			tmp_str++;

			/* Remove whitespaces */
			while (*tmp_str == ' ') {
				tmp_str++;
			}

			nbuf = tmp_str;

			tmp_str = strchr(nbuf, '\n');
			if (tmp_str) {
				*tmp_str = '\0';
			}

			pclose(fp);
			pid_name = nbuf;
			return pid_name;
		}

		pclose(fp);
	}

	return pid_name;
}

/**
 * 新增一个item
 */
void ProcessList::add_items() {
	if (_ps.empty()) {
		if (set_ps() == -1) {
			return;
		}
	}

	for (auto i = 1; i <= MAX_PID; i++) {
		/* Check if the pid is present */
		if (!((getsid(i) == -1) && (errno == ESRCH)) &&
				!((getpgid(i) == -1) && (errno == ESRCH))) {

			auto name = get_name_by_pid(i);
			if (name.empty()) {
				continue;
			}

			ProcessInfo one(i, name, "");
			_list.push_back(one);
		}
	}
}

/**
 * 判断是否是process
 */
bool ProcessList::is_process(const std::string& value) const {
	if (value.empty()) {
		return false;
	}
	// run data转换为rootkit run data
	auto run_data = std::static_pointer_cast<RkRunData>(_run_data);

	for (const auto &one: _list) {
		// process匹配处理
		if (util::pattern_match(one._path, value)) {
			info("ProcessList", "Process: %s", one._path.c_str());

			std::string msg = "Process: " + one._path;
			// todo: 设置alert msg
			(void)run_data->alert_msg()->add_end(msg);
			return true;
		}
	}

	return false;
}

/**
 * 检查pids
 */
void CheckPids::check_rc_pids() {
	std::string ps = "/bin/ps";
	if (!util::is_file(ps)) {
		ps = "/usr/bin/ps";
		if (!util::is_file(ps)) {
			ps = "";
		}
	}

	std::string proc_dir = "/proc";
	std::string proc_pid1 = "/proc/1";
	if (!util::is_file(proc_dir) && !util::is_file(proc_pid1)) {
		_have_proc = true;
	}

	loop_all_pids(ps);

	if (_errors == 0) {
		std::string op_msg = "No hidden process by Kernel-level rootkits.\n		" + ps
				+ " is not trojaned. Analyzed " + _total + " processes.";
		// todo: notify_rk(ALERT_OK, op_msg);
	}

	return;
}

void CheckPids::loop_all_pids(const std::string& ps) {
	auto config = std::static_pointer_cast<RkConfig>(_config);
	auto server_name = config->server_name();

	for (auto i = 1; i <= _MAX_PID; i++) {
		_total++;

		/* kill pid 0	检测进程的存在性 */
		bool kill_test0 = false;
		if (!((kill(i, 0) == -1) && (errno == ESRCH))) {
			kill_test0 = true;
		}
		/* getsid test */
		bool sid_test0 = false;
		if (!((getsid(i) == -1) && (errno == ESRCH))) {
			sid_test0 = true;
		}
		/* getpgid test */
		bool pgid_test0 = false;
		if (!((getpgid(i) == -1) && (errno == ESRCH))) {
			pgid_test0 = true;
		}

		bool proc_dir = proc_stat(i);
		bool proc_rd = proc_read(i);
		bool proc_ch = proc_chdir(i);

		/* if PID does not exist, move on */
		if (!kill_test0 && !sid_test0 && !pgid_test0 &&
				!proc_dir && !proc_rd && !proc_ch) {
			continue;
		}

		/* ignore our own pid */
		if (i == _my_pid) {
			continue;
		}

		/* check the number of errors */
		if (_errors > 15) {
			std::string op_msg = "Excessive number of hidden processes. "
					"It maybe a false-positive or something really bad is going on.";
			// todo: notify_rk(ALERT_SYSTEM_CRIT, op_msg);
			return;
		}

		/* check if the process appears in ps(1) output */
		bool check_ps = false;
		if (!ps.empty()) {
			std::string cmd = ps + " -p " + i + " > /dev/null 2>&1";
			debug(server_name.c_str(), "Check pids command: %s", cmd.c_str());
			if (system(cmd.c_str()) == 0) {
				check_ps = true;
			}
		}

		/* if we are run in the context of HIDS(主机入侵检测系统), sleep here(no rush) */
		sleep(2);

		/* everything fine, move on */
		if (check_ps && kill_test0 && sid_test0 && pgid_test0 && proc_dir && proc_rd && proc_ch) {
			continue;
		}

		/* here, check_ps = false. this is ps(1) false，but kill or getsid system call got the pid.
		 * or no have ps.
		 * then, check again......
		 * */
		bool kill_test1 = false;
		if (!((kill(i, 0) == -1) && (errno == ESRCH))) {
			kill_test1 = true;
		}
		bool sid_test1 = false;
		if (!((getsid(i) == -1) && (errno == ESRCH))) {
			sid_test1 = true;
		}
		bool pgid_test1 = false;
		if (!((getpgid(i) == -1) && (errno == ESRCH))) {
			pgid_test1 = true;
		}
		proc_dir = proc_stat(i);			/*/proc/pid is file */
		proc_rd = proc_read(i);				/* pid is in /proc/ */
		proc_ch = proc_chdir(i);			/* 可以进入/proc/pid目录 */

		/* if it matches, process was terminated in the meantime, so move on */
		if (!kill_test1 && !sid_test1 && !pgid_test1 &&
				!proc_dir && !proc_rd && !proc_ch) {
			continue;
		}
#ifdef AIX
		// todo:
#endif
		if ((sid_test0 == sid_test1) && (kill_test0 == kill_test1) &&
				sid_test0 != kill_test0) {
			/**
			 * if kill worked, but getsid and getpgid did not, it may be a defunct process, so ignore.
			 * defunct: 死的，非现存的
			 */
			if (!(kill_test0 == true && sid_test0 == false && pgid_test0 == false)) {
				std::string op_msg = "Process '" + i + "' hidden from kill(" + kill_test0 + ") or getsid(" +
						sid_test0 + "). Possible kernel-level rootkit.";
				// todo: notify_rk(ALERT_ROOTKIT_FOUND, op_msg);
				_errors++;
			}
		}
		else if ((kill_test1 != sid_test1) || (kill_test1 != pgid_test1) || (pgid_test1 != sid_test1)) {
			/**
			 * ignore defunct process.
			 */
			if (!(kill_test1 == true && sid_test1 == false && pgid_test0 == false && sid_test1 == false)) {
				std::string op_msg = "Process '" + i + "' hidden from kill(" + kill_test1 + ") or getsid(" +
						sid_test1 + "). Possible kernel-level rootkit.";
				// todo: notify_rk(ALERT_ROOTKIT_FOUND, op_msg);
				_errors++;
			}
		}
		else if (proc_rd != proc_dir || proc_rd != proc_ch || proc_dir != kill_test1) {
			/* check if the pid is a thread (not showing in /proc) */
			if (_have_proc && !check_proc(i)) {
				std::string msg = "Process '" + i + "' hidden from /proc. Possible kernel level rootkit.";
				// todo: notify_rk(ALERT_ROOTKIT_FOUND, op_msg);
				_errors++;
			}
		}
		else if (sid_test1 && kill_test1 && ps.empty()) {
			/* checking if the pid is a thread, because not showing on ps */
			if (check_proc((int)i) == false) {
				std::string op_msg = "Process '" + i + "' hidden from ps. Possible trojaned version installed.";;
				// todo: notify_rk(ALERT_ROOTKIT_FOUND, op_msg);
				_errors++;
			}
		}
	}
	return;
}

/**
 * read the /proc directory(if present) and
 * check if it can find the given pid(as a pid or as a thread)
 * if it is pid, /proc/pid
 * and else if it is thread id(tid), /proc/pid/task/tid
 * return 	true
 * 			false
 */
bool CheckPids::check_proc(int pid) {
	auto config = std::static_pointer_cast<RkConfig>(_config);
	auto server_name = config->server_name();

	/* check /proc/.pid first */
	std::string pid_dir = "/proc/." + pid;
	if (util::is_file(pid_dir)) {
		debug(server_name.c_str(), "proc file: %s", pid_dir.c_str());
		return true;
	}

	/* check /proc，内部依次check：/proc/pid、/proc/pid/task/pid */
	(void)read_proc_dir("/proc", pid, CheckPids::Position::PROC);

	return _proc_pid_found;
}

/**
 * 分析读取/proc/目录
 */
int CheckPids::read_proc_dir(const std::string& dir_name, int pid, int position) {
	auto config = std::static_pointer_cast<RkConfig>(_config);
	auto server_name = config->server_name();

	if (dir_name.c_str()) {
		error(server_name.c_str(), "%s: Invalid directory given");
		return -1;
	}

	std::string str_pid = std::to_string(pid);

	DIR *dp = opendir(dir_name.c_str());
	if (!dp) {
		error(server_name.c_str(), "%s: open dir failed.");
		return 0;
	}
	struct dirent * entry;
	while ((entry = readdir(dp)) != nullptr) {
		if ((strcmp(entry->d_name, ".") == 0) || (strcmp(entry->d_name, "..") == 0)) {
			continue;
		}

		if (position == CheckPids::Position::PROC) {
			/**
			 * 分析pid目录，current path：/proc/
			 */
			char *tmp_str = entry->d_name;
			while (*tmp_str != '\0') {
				if (!std::isdigit((int)*tmp_str)) {			// 不是数字字符的，就break；
					break;
				}
				tmp_str++;
			}
			if (*tmp_str != '\0') {
				continue;
			}

			std::string file_name = dir_name + "/" + entry->d_name;
			(void)read_proc_file(file_name, pid, position + 1);
		}
		else if (position == CheckPids::Position::PID) {
			/**current path: /proc/[pid], match value: /proc/439/task
			 */
			if (strcmp(entry->d_name, "task") == 0) {
				std::string file_name = dir_name + "/" + entry->d_name;		// eg: /proc/439/task
				(void)read_proc_file(file_name, pid, position + 1);
			}
		}
		else if (position == CheckPids::Position::TASK) {
			/**current path: /proc/[pid]/task
			 * check under proc/pid/task/, eg: /proc/439/task/439
			 */
			if (strcmp(entry->d_name, str_pid.c_str()) == 0) {
				_proc_pid_found = true;
				break;
			}
		}
		else {
			break;
		}
	}
	closedir(dp);

	return 0;
}

int CheckPids::read_proc_file(const std::string& file_name, int pid, int position) {
	struct stat statbuf;
	if (lstat(file_name.c_str(), &statbuf) < 0) {
		return -1;
	}

	/* if directory, read the directory */
	if (S_ISDIR(statbuf.st_mode)) {
		return read_proc_dir(file_name, pid, position);
	}

	return 0;
}

bool CheckPids::proc_stat(int pid) {
	if (!_have_proc) {
		return false;
	}

	std::string proc_dir = "/proc/" + pid;
	if (util::is_file(proc_dir)) {
		return true;
	}
	return false;
}

bool CheckPids::proc_read(int pid) {
	if (!_have_proc) {
		return false;
	}

	std::string dir = pid;
	if (util::is_file_on_dir(dir, "/proc")) {
		return true;
	}

	return false;
}

bool CheckPids::proc_chdir(int pid) {
	if (!_have_proc) {
		return false;
	}

	char curr_dir[OS_SIZE_1024 + 1];
	if (getcwd(curr_dir, OS_SIZE_1024) == nullptr) {
		return false;
	}

	if (chdir("/proc") == -1) {
		return false;
	}

	bool ret = false;
	std::string pid_dir = "/proc/" + pid;
	if (chdir(pid_dir.c_str()) == 0) {
		ret = true;
	}

	if (chdir(curr_dir) == -1) {
		return false;
	}

	return ret;
}
#endif


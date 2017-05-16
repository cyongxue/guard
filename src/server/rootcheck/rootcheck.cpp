/*

 *  rootcheck.cpp, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年3月18日
 *      Author: yongxue@cyongxue@163.com
 */

#include "rootcheck.h"

#include "process.h"

int RootCheck::read_config() {
	info(_server_name.c_str(), "RootCheck Read config ......");

	if (util::file_mod_time(_config->cfg_file()) < 0) {
		error(_server_name.c_str(), "Configuration file not found: '%s'.", _config->cfg_file().c_str());
		return -1;
	}

	if (_config->read_config() < 0) {
		error(_server_name.c_str(), "Configuration error at '%s'", _config->cfg_file().c_str());
		return -1;
	}

	return 0;
}

int RootCheck::create_write_queue() {
	auto queue_path = Chroot::instance()->full_file_path(UNIX_QUEUE);

	std::shared_ptr<net::UnixDomainClient> rk_queue = net::write_msg_queue(queue_path);
	if (rk_queue == nullptr) {
		error(_server_name.c_str(), "create_write_queue failed.");
		return -1;
	}

	auto run_data = std::static_pointer_cast<RkConfig>(_config)->run_data();
	run_data->set_rk_queue(rk_queue);

	return 0;
}

/**
 * return 	-1			失败
 * 			0 			成功
 * 			1			不生效rootcheck
 */
int RootCheck::initial() {
	info(_server_name.c_str(), "RootCheck Begin initial ......");

	auto rk_config = std::static_pointer_cast<RkConfig>(_config);
	auto config_data = rk_config->config_data();
	if (config_data->disable()) {
		info(_server_name.c_str(), "Rootcheck disabled. Exiting.");
		return - 1;
	}

	if (config_data->unix_audit().empty()) {
		warn(_server_name.c_str(), "System audit file not configured..");
	}

	if (config_data->workdir().empty()) {
		config_data->set_workdir(base_dir());
	}

	if (create_write_queue() < 0) {
		error(_server_name.c_str(), "rootcheck create write queue failed.");
		return -1;
	}

	return 0;
}

int RootCheck::send_rk_notify(AlertType type, const std::string& msg) {
	auto rk_config = std::static_pointer_cast<RkConfig>(_config);
	auto config_data = rk_config->config_data();

	if (config_data->notify() != NotifyType::QUEUE) {
		// 标准输出重定向到syslog？？？？
		// todo:
		return 0;
	}

	if (type <= AlertType::ALERT_SYSTEM_ERR) {
		return 0;
	}

	// todo: send_rk_msg

	return 0;
}

/**
 * 执行rootcheck处理
 */
void RootCheck::run_rk_check() {
	auto rk_config = std::static_pointer_cast<RkConfig>(_config);
	auto config_data = rk_config->config_data();
	auto run_data = rk_config->run_data();

	// 设置base dir
	if (config_data->basedir().empty()) {
#ifndef WIN
		std::string base_dir = "/";
		if ((base_dir.length() > 0) && (base_dir[base_dir.length() - 1] == '/')) {
			base_dir = base_dir.substr(0, base_dir.length() - 1);
		}
#else
		std::string base_dir = "C:\\";
#endif
		config_data->set_basedir(base_dir);
	}

	time_t time1 = time(0);
	if (config_data->notify() != QUEUE) {
		info(_server_name.c_str(), "\n");
		info(_server_name.c_str(), "Be patient, it may take a few minutes to complete...\n\n");
	}

	// 初始化rk file/name
	run_data->init_rk_file();

	(void)send_rk_notify(AlertType::ALERT_POLICY_VIOLATION, "Starting rootcheck scan.");
	if (config_data->notify() == NotifyType::QUEUE) {
		info(_server_name.c_str(), "Starting rootcheck scan.");
	}

	// rootkit file
	(void)do_check_rc_files();
	// rootkit trojans
	(void)do_check_rc_trojans();

#ifdef WIN
	/* Windows audit check/Windows malware/Windows Apps */
	// todo:
#else
	(void)do_check_rc_unixaudit();
#endif

	/* Check for files in the /dev filesystem，检查/dev的文件系统 */
	(void)check_rc_dev();

	/* Scan the whole system for additional issues */
	(void)check_rc_sys();

	/* Check processes，检查进程process */
	(void)check_rc_pids();

	/* Check all ports，检查port */
	(void)check_rc_ports();

	/* Check interfaces，检查interface */
	(void)check_rc_if();

	debug(_server_name.c_str(), "Completed with all checks..");

	/* clear rk system files */
	run_data->clear_rk_file();

	/* Final message */
	auto time2 = time(0);
	if (config_data->notify() != NotifyType::QUEUE) {
		info(_server_name.c_str(), "\n");
		info(_server_name.c_str(), "- Scan completed in %s secondes.\n\n", time2 - time1);
	}
	else {
		sleep(5);
	}

	/* send scan ending message */
	// todo:notify_rk(ALERT_POLICY_VIOLATION, "Ending rootcheck scan.");
	if (config_data->notify() == NotifyType::QUEUE) {
		info(_server_name.c_str(), "Ending rootcheck scan.");
	}

	debug(_server_name.c_str(), "Leaving RootCheck::run_rk_check");
	return;
}

#ifdef WIN
// todo:
#else
int RootCheck::do_check_rc_unixaudit() {
	auto rk_config = std::static_pointer_cast<RkConfig>(_config);
	auto config_data = rk_config->config_data();

	if (config_data->checkflags_unixaudix()) {
		if (config_data->unix_audit()) {
			// todo: process_list
			ProcessList pro_list(rk_config->run_data());
			(void)pro_list.set_ps();
			pro_list.add_items();

			for (auto one: config_data->unix_audit()) {
				auto fp = fopen(one.c_str(), "r");
				if (fp) {
					Rcl rcl(_server_name, one, "System Audit:");
					rcl.get_entry(pro_list);
					fclose(fp);
				}
				else {
					error(_server_name.c_str(), "No unixaudit file: '%s'", one.c_str());
				}
			}
		}
	}

	return 0;
}
#endif

int RootCheck::check_rc_if() {
	auto rk_config = std::static_pointer_cast<RkConfig>(_config);
	auto config_data = rk_config->config_data();

	if (config_data->checkflags_if()) {
		debug(_server_name.c_str(), "checking interface...");
		CheckInterface check_if(rk_config);
		check_if.check_if();
		info(_server_name.c_str(), "check over interface.");
	}
	else {
		info(_server_name.c_str(), "No enable check interface Flags.");
	}
	return 0;
}

int RootCheck::check_rc_ports() {
	auto rk_config = std::static_pointer_cast<RkConfig>(_config);
	auto config_data = rk_config->config_data();

	if (config_data->checkflags_port()) {
		debug(_server_name.c_str(), "checking ports....");

		CheckPorts ck_ports(rk_config);
		debug(_server_name.c_str(), "going into check rc ports.");
		ck_ports.check_ports();								// 检查端口，netstat检测
		debug(_server_name.c_str(), "going into check open ports.");
		ck_ports.check_open_ports();						// 对于open的ports，进一步检测

		info(_server_name.c_str(), "check over ports.");
	}
	else {
		info(_server_name.c_str(), "No enable check ports.");
	}
	return 0;
}

int RootCheck::check_rc_pids() {
	auto rk_config = std::static_pointer_cast<RkConfig>(_config);
	auto config_data = rk_config->config_data();

	if (config_data->checkflags_pid()) {
		debug(_server_name.c_str(), "going check rc pids.");
		CheckPids check_pids(rk_config);
		check_pids.check_rc_pids();
	}
	else {
		info(_server_name.c_str(), "No enable check pids.");
	}

	return 0;
}

int RootCheck::check_rc_sys() {
	auto rk_config = std::static_pointer_cast<RkConfig>(_config);
	auto config_data = rk_config->config_data();

	if (config_data->checkflags_sys()) {
		debug(_server_name.c_str(), "going check rc system");
		CheckSys check_sys(rk_config);
		check_sys.check_sys();
	} else {
		info(_server_name.c_str(), "No enable check system");
	}

	return 0;
}

int RootCheck::check_rc_dev() {
	auto rk_config = std::static_pointer_cast<RkConfig>(_config);
	auto config_data = rk_config->config_data();

	if (config_data->checkflags_dev()) {
		debug(_server_name.c_str(), "going check check dev");
		CheckDev check_dev(rk_config);
		check_dev.check_dev();
	}
	else {
		info(_server_name.c_str(), "No enable check dev.");
	}

	return 0;
}


/**
 * rootkit_trojans.txt
 * 文件内容如下：
 * 	  ls          !bash|^/bin/sh|dev/[^clu]|\.tmp/lsfile|duarawkz|/prof|/security|file\.h!
 */
int RootCheck::check_rc_trojans(FILE *fp) {
	auto rk_config = std::static_pointer_cast<RkConfig>(_config);
	auto config_data = rk_config->config_data();

#ifndef WIN
	std::vector<std::string> all_paths;
	all_paths.push_back("bin");
	all_paths.push_back("sbin");
	all_paths.push_back("usr/bin");
	all_paths.push_back("usr/sbin");
#else
	std::vector<std::string> all_paths;
	all_paths.push_back("C:\\Windows\\");
	all_paths.push_back("D:\\Windows\\");
#endif
	debug(_server_name.c_str(), "Starting on check_rc_trojans.");

	int total = 0, hit_flag = 0;			/* total:扫描的文件数；hit_flag：Trojan命中flag */
	char buf[OS_SIZE_1024 + 1];
	while (fgets(buf, OS_SIZE_1024, fp) != nullptr) {
		/* remove end of line */
		char *nbuf = strchr(buf, '\n');
		if (nbuf) {
			*nbuf = '\0';
		}

		nbuf = util::rm_end_head_space_tab(nbuf);
		if (*nbuf == '\0' || *nbuf == '#') {		/* 空 or 注释 */
			continue;
		}

		/* file now may be valid */
		char * name = nbuf;
		char * str_to_look = strchr(name, '!');
		if (!str_to_look) {
			continue;
		}
		*str_to_look = '\0';
		str_to_look++;

		char * msg = strchr(str_to_look, '!');
		if (!msg) {
			continue;
		}
		*msg = '\0';
		msg++;

		name = util::rm_end_head_space_tab(name);
		str_to_look = util::rm_end_head_space_tab(str_to_look);
		msg = util::rm_end_head_space_tab(msg);
		if ((*name == '\0') || (*str_to_look == '\0')) {
			continue;
		}

		total++;

		std::string file_path;
		for (auto one: all_paths) {
			if (*name != '/') {
				file_path = config_data->basedir() + "/" + one + "/" + name;
			}
			else {
				file_path = name;
			}
			/* 进行匹配match */
			auto regex_check = Strings(file_path, str_to_look);
			if (util::is_file(file_path) && regex_check.match()) {
				hit_flag = 1;

				/* 找到rootkit */
				char *tmp_msg = (*msg == '\0' ? "Generic" : msg);
				std::string notify_msg = "Trojaned version of file '" + file_path + "' detected. Signature used: '"
						+ str_to_look + "' (" + tmp_msg + ").";
				send_rk_notify(AlertType::ALERT_ROOTKIT_FOUND, notify_msg);
			}

			if (*name == '/') {
				break;
			}
		}
		continue;
	}

	if (hit_flag == 0) {
		std::string notify_msg = "No binaries with any trojan detected. Analyzed " + total + " files.";
		send_rk_notify(AlertType::ALERT_OK, notify_msg);
	}

	return 0;
}


int RootCheck::do_check_rc_trojans() {
	auto rk_config = std::static_pointer_cast<RkConfig>(_config);
	auto config_data = rk_config->config_data();

	if (config_data->checkflags_trojans()) {
		if (config_data->rootkit_trojans().empty()) {
			error(_server_name.c_str(), "No rootcheck_trojans file configured.");
		}
		else {
			auto fp = fopen(config_data->rootkit_file().c_str(), "r");
			if (!fp) {
				error(_server_name.c_str(), "rootcheck_trojans file open failed: '%s'", config_data->rootkit_file().c_str());
			}
			else {
				(void)check_rc_trojans(fp);
				fclose(fp);
			}
		}
	}

	return 0;
}


/**
 * etc/shared/rootkit_files.txt 文件中内容如下：
 * # Bash door
 * tmp/mcliZokhb           ! Bash door ::/rootkits/bashdoor.php
 * tmp/mclzaKmfa           ! Bash door ::/rootkits/bashdoor.php
 */
int RootCheck::check_rc_files(FILE *fp) {
	debug(_server_name.c_str(), "Starting on check_rc_files.");

	char * file = nullptr;
	char * name = nullptr;
	char * link = nullptr;

	int errors = 0;
	int total = 0;

	auto rk_config = std::static_pointer_cast<RkConfig>(_config);
	auto config_data = rk_config->config_data();
	auto run_data = rk_config->run_data();

	char buf[OS_SIZE_1024 + 1];
	while(fgets(buf, OS_SIZE_1024, fp) != nullptr) {
		/* Remove newline at the end */
		char *nbuf = strchr(buf, '\n');
		if (nbuf) {
			*nbuf = '\0';
		}

		nbuf = buf;
		/* Skip comments and blank lines */
		while(*nbuf != '\0') {
			if (*nbuf == ' ' || *nbuf == '\t') {
				nbuf++;
				continue;
			}
			else if (*nbuf == '#') {
				goto newline;
			}
			else {
				break;
			}
		}
		if (*nbuf == '\0') {
			goto newline;
		}

		file = nbuf;
		name = nbuf;
		/* Get the file and the rootkit name */
		while (*nbuf != '\0') {
			if (*nbuf == ' ' || *nbuf == '\t') {
				*nbuf = '\0';
				nbuf++;
				break;
			}
			else {
				nbuf++;
			}
		}
		if (*nbuf == '\0') {
			goto newline;
		}

		/* Some ugly code to remove spaces and \t */
		while (*nbuf != '\0') {
			if (*nbuf == '!') {
				nbuf++;
				if (*nbuf == ' ' || *nbuf == '\t') {
					nbuf++;
					name = nbuf;
					break;
				}
			}
			else if (*nbuf == ' ' || *nbuf == '\t') {
				nbuf++;
				continue;
			} else {
				goto newline;
			}
		}

		/* Get the link (if present) */
		link = strchr(nbuf, ':');
		if (link) {
			*link = '\0';

			link++;
			if (*link == ':') {
				link++;
			}
		}

		/* Clean any space or tab at the end */
		nbuf = strchr(nbuf, ' ');
		if (nbuf) {
			*nbuf = '\0';

			nbuf = strchr(nbuf, '\t');
			if (nbuf) {
				*nbuf = '\0';
			}
		}

		total++;
		if (*file == '*') {
			if (run_data->rk_count() >= RkRunData::MAX_RK_SYS) {
				error(_server_name.c_str(), "Maximum number of global files reached: %d", RkRunData::MAX_RK_SYS);
			}
			else {
				/* Remove all slashes from the file */
				file++;
				if (*file == '/') {
					file++;
				}
				run_data->add_rk_file(file, name);
			}
			continue;
		}

		std::string file_path = config_data->basedir() + "/" + file;
		if (util::is_file(file_path)) {
			errors = 1;

			std::string msg = "Rootkit '" + name + "' detected by the presence of file '" + file_path + "'.";
			send_rk_notify(AlertType::ALERT_ROOTKIT_FOUND, msg);
		}
newline:
		continue;
	}

	if (errors == 0) {
		std::string msg = "No presence of public rootkits detected. Analyzed " + total + " files.";
		send_rk_notify(AlertType::ALERT_OK, msg);
	}

	return 0;
}

/**
 * etc/shared/rootkit_files.txt 文件中内容如下：
 * # Bash door
 * tmp/mcliZokhb           ! Bash door ::/rootkits/bashdoor.php
 * tmp/mclzaKmfa           ! Bash door ::/rootkits/bashdoor.php
 * 依次的格式为：
 * 		file		!name	::link
 */
int RootCheck::do_check_rc_files() {
	auto rk_config = std::static_pointer_cast<RkConfig>(_config);
	auto config_data = rk_config->config_data();

	if (config_data->checkflags_files()) {
		if (config_data->rootkit_file().empty()) {
			error(_server_name.c_str(), "No rootcheck_files file configured.");
		}
		else {
			FILE* fp = fopen(config_data->rootkit_file().c_str(), "r");
			if (fp) {
				(void)check_rc_files(fp);
				fclose(fp);
			}
			else {
				error(_server_name.c_str(), "No rootcheck_files file: '%s'", config_data->rootkit_file().c_str());
			}
		}
	}
	return 0;
}

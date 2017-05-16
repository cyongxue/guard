/*
 *  main.cpp, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年3月2日
 *      Author: yongxue@cyongxue@163.com
 */

#include "syscheck.h"
#include "net_client.h"

int Syscheckd::read_config() {
	info(_server_name.c_str(), "Syscheckd Read config ......");

	if (util::file_mod_time(_config->cfg_file()) < 0) {
		error(_server_name.c_str(), "Configuration file not found: '%s'.", _config->cfg_file().c_str());
		return -1;
	}

	if (_config->read_config() < 0) {
		error(_server_name.c_str(), "Configuration error at '%s'", _config->cfg_file().c_str());
		return -1;
	}

	/**
	 * rootcheck配置的读取
	 */
#ifdef ROOTCHECK_IN_SYSCHECK
	if (_rk_check->read_config() < 0) {
		error(_server_name.c_str(), "rootcheck read config failed.");
		return -1;
	}
#endif

	return 0;
}

void Syscheckd::read_internal_opt() {
	info(_server_name.c_str(), "Read internal optinal ......");

	auto tsleep = _config->get_define_int(_server_name, "sleep", 0, 64);
	auto sleep_after = _config->get_define_int(_server_name, "sleep_after", 1, 9999);

	auto syscheck_config = std::static_pointer_cast<SyscheckConfig>(_config);
	auto config_data = syscheck_config->config_data();

	config_data->set_tsleep(tsleep);
	config_data->set_sleep_after(sleep_after);

	return;
}


void Syscheckd::print_syscheck_info() {

	auto syscheck_config = std::static_pointer_cast<SyscheckConfig>(_config);
	auto config_data = syscheck_config->config_data();

	for (auto one: config_data->dirs_infos()) {
		info(_server_name.c_str(), "Monitoring directory: '%s', with options %s.", one._dir.c_str(), one._opts.opts_str().c_str());
		if (one._opts.is_opt(util::FileCheckOpt::CHECK_REALTIME)) {
#ifdef INOTIFY_ENABLED
			info(_server_name.c_str(), "Directory set for real time monitoring: '%s'.", one._dir.c_str());
#else
			warn(_server_name.c_str(), "Ignoring flag for real time monitoring on directory: '%s'.", one._dir.c_str());
#endif
		}
	}

	for (auto one: config_data->ignores()) {
		info(_server_name.c_str(), "ignoring: '%s'", one.c_str());
	}

	for (auto one: config_data->nodiffs()) {
		info(_server_name.c_str(), "No diff for file: '%s'", one.c_str());
	}

	return;
}

int Syscheckd::create_write_queue() {
	auto queue_path = Chroot::instance()->full_file_path(UNIX_QUEUE);

	std::shared_ptr<net::UnixDomainClient> sys_queue = net::write_msg_queue(queue_path);
	if (sys_queue == nullptr) {
		error(_server_name.c_str(), "create_write_queue failed.");
		return -1;
	}

	auto run_data = std::static_pointer_cast<SyscheckConfig>(_config)->run_data();
	run_data->set_sys_queue(sys_queue);

	return 0;
}

bool Syscheckd::get_day_scanned(int& curr_day) {
	auto sys_config = std::static_pointer_cast<SyscheckConfig>(_config);
	auto sys_config_data = sys_config->config_data();

	bool day_scanned = false;
	if (sys_config_data->daytime() || sys_config_data->weekday_flag()) {
		auto curr_time = time(0);
		auto p = localtime(&curr_time);

		char curr_hour[12];
		memset(curr_hour, '\0', sizeof(curr_hour));
		snprintf(curr_hour, 9, "%02d:%02d:%02d", p->tm_hour, p->tm_min, p->tm_sec);
		curr_day = p->tm_mday;

		if (sys_config_data->daytime() || sys_config_data->weekday_flag()) {
			if (sys_config_data->weekday_flag()->is_on_day(p->tm_wday) &&
					sys_config_data->daytime()->is_after_time(curr_hour)) {
				day_scanned = true;
			}
		}
		else if (sys_config_data->daytime()) {
			if (sys_config_data->daytime()->is_after_time(curr_hour)) {
				day_scanned = true;
			}
		}
		else if (sys_config_data->weekday_flag()) {
			if (sys_config_data->weekday_flag()->is_on_day(p->tm_wday)) {
				day_scanned = true;
			}
		}
	}
	else {
		return day_scanned;
	}

	return day_scanned;
}


int Syscheckd::initial() {
	info(_server_name.c_str(), "Syscheckd Begin initial ......");

	auto syscheck_config = std::static_pointer_cast<SyscheckConfig>(_config);
	auto config_data = syscheck_config->config_data();

	/** syscheck的初始化 */
	if (config_data->syscheck_disable()) {
		error(_server_name.c_str(), "No directory provided for syscheck to monitor.");
	}
	if (config_data->dirs_infos().empty()) {
		warn(_server_name.c_str(), "Syscheck disabled.");
	}

	/** rootcheck的初始化 */
#ifdef ROOTCHECK_IN_SYSCHECK
	auto rk_ret = _rk_check->initial();
	if (rk_ret == 0) {
		config_data->set_rootcheck(1);
	}
	else if (rk_ret == 1) {
		config_data->set_rootcheck(0);
		warn(_server_name.c_str(), "Rootcheck module disabled.");
	}
	else {
		config_data->set_rootcheck(0);
		error(_server_name.c_str(), "rootcheck init failed.");
	}
#endif

	/* sleep */
	sleep(config_data->tsleep() + 2);

	/** create write queue */
	if (create_write_queue() < 0) {
		error(_server_name.c_str(), "create write queue failed.");
		return -1;
	}

	if (config_data->rootcheck()) {
		info(_server_name.c_str(), "Rootcheck start in syscheck.");
	}

	/** print syscheck info */
	print_syscheck_info();

	sleep(config_data->tsleep() + 10);					// why??????

	return 0;
}

/**
 * 创建file db
 */
void Syscheckd::create_file_db() {
	auto sys_config = std::static_pointer_cast<SyscheckConfig>(_config);

	auto sys_config_data = sys_config->config_data();
	if (sys_config_data->dirs_infos().empty()) {
		return;
	}
	info(_server_name.c_str(), "Starting syscheck scan (forwarding file db).");
	// todo: send_syscheck_msg

	auto file_db = std::make_shared<FileDb>(sys_config);
	if (file_db == nullptr) {
		fatal(_server_name.c_str(), "Create file db failed.");
	}
	auto sys_run_data = sys_config->run_data();
	sys_run_data->set_file_db(file_db);

	sys_run_data->set_realtime(std::make_shared<RtFim>(file_db));
	sleep(sys_config_data->tsleep() + 10);

	info(_server_name.c_str(), "Ending syscheck scan (forwarding file db).");
	// todo: send_syscheck_msg

	return;
}

bool Syscheckd::deal_day_and_time(time_t& prev_time_rk, time_t& prev_time_sk, int& curr_day) {

	auto sys_config = std::static_pointer_cast<SyscheckConfig>(_config);
	auto sys_config_data = sys_config->config_data();
	auto sys_run_data = sys_config->run_data();

#ifdef ROOTCHECK_IN_SYSCHECK
	auto rk_config = std::static_pointer_cast<RkConfig>(_rk_check->config());
	auto rk_config_data = rk_config->config_data();
#endif

	debug(_server_name.c_str(), "before run deal day and time.");
	sleep(sys_config_data->tsleep() * 10);

	/* 设置扫描频率 */
	if (sys_config_data->daytime() || sys_config_data->weekday_flag()) {
		sys_config_data->set_time(DAY_7);					// 如果设置了day和weekday，则说明time为7天
#ifdef ROOTCHECK_IN_SYSCHECK
		rk_config_data->set_time(DAY_7);
#endif
	}

	/* 可能启动即开始扫描 */
	if (sys_run_data->scan_on_start()) {
		sleep(sys_config_data->tsleep() * 15);
		create_file_db();
	}
	else {
		prev_time_rk = time(0);
	}
	prev_time_sk = time(0);
	sleep(sys_config_data->tsleep() * 10);

	return get_day_scanned(curr_day);
}

void Syscheckd::get_day_scanned_inwhile(time_t curr_time, int& curr_day, bool& run_now, bool& day_scanned) {
	struct tm *p;
	char curr_hour[12];
	memset(curr_hour, '\0', 12);

	auto config_data = std::static_pointer_cast<SyscheckConfig>(_config)->config_data();

	auto day_time = config_data->daytime();
	auto week_day = config_data->weekday_flag();
	if (day_time || week_day) {
		p = localtime(&curr_time);

		/* Day changed */
		if (curr_day != p->tm_mday) {
			day_scanned = false;
			curr_day = p->tm_mday;
		}

		/* Check for the time of the scan */
		if (!(day_scanned) && day_time && week_day) {			// 限制day和time
			/* Assign hour/min/sec values */
			snprintf(curr_hour, 9, "%02d:%02d:%02d", p->tm_hour, p->tm_min, p->tm_sec);

			if ((day_time->is_after_time(curr_hour)) &&
					(week_day->is_on_day(p->tm_wday))) {
				day_scanned = true;
				run_now = true;
			}
		} else if (!(day_scanned) && day_time) {						// 仅限制time
			/* Assign hour/min/sec values */
			snprintf(curr_hour, 9, "%02d:%02d:%02d", p->tm_hour, p->tm_min, p->tm_sec);

			if (day_time->is_after_time(curr_hour)) {
				run_now = true;
				day_scanned = true;
			}
		} else if (!(day_scanned) && week_day) {							// 仅限制day
			/* Check for the day of the scan */
			if (week_day->is_on_day(p->tm_wday)) {
				run_now = true;
				day_scanned = true;
			}
		}
	}

	return ;
}

/**
 * return true			配置文件发生变更
 * 		  false			配置文件没有变更
 */
bool Syscheckd::sleep_while() {
	auto config = std::static_pointer_cast<SyscheckConfig>(_config);
	auto run_data = config->run_data();

	bool is_conf_change = false;

#ifdef INOTIFY_ENABLED
	if (run_data->realtime() && (run_data->realtime()->fd() >= 0)) {
		struct timeval select_time;
		select_time.tv_sec = 300;
		select_time.tv_usec = 0;

		fd_set rfds;
		FD_ZERO(&rfds);
		FD_SET(run_data->realtime()->fd(), &rfds);
		auto run_flag = select(run_data->realtime()->fd() + 1, &rfds, nullptr, nullptr, &select_time);
		if (run_flag < 0) {
			error(_server_name.c_str(), "Select failed for realtime fim");
			sleep(300);
		}
		else if (run_flag == 0) {
			/* Timeout */
		} else if (FD_ISSET(run_data->realtime()->fd(), &rfds)) {
			run_data->realtime()->process();			/* 处理事件 */
		}
	}
	else {
		// todo: sleep时间要考虑如何设置，暂定为300s
		sleep(300);
	}
#elif defined(WIN)
	// todo：
#else
	(void)run_data;
	sleep(300);
#endif

	return is_conf_change;
}

/* syscheck run */
void Syscheckd::run() {
	info(_server_name.c_str(), "Begin syscheck running ......");

	time_t prev_time_rk = 0;
	time_t prev_time_sk = 0;
	int curr_day = 0;
	bool day_scanned = deal_day_and_time(prev_time_rk, prev_time_sk, curr_day);

	auto config = std::static_pointer_cast<SyscheckConfig>(_config);
	auto config_data = config->config_data();
	auto run_data = config->run_data();

	/**
	 * runloop执行
	 */
	while(true) {
		bool run_now = false;
		time_t curr_time = time(0);
		// todo: check_restart_syscheck

		get_day_scanned_inwhile(curr_time, curr_day, run_now, day_scanned);

		if (config_data->rootcheck()) {
#ifdef ROOTCHECK_IN_SYSCHECK
			auto rk_config = std::static_pointer_cast<RkConfig>(_rk_check->config());
			if (((curr_time - prev_time_rk) > rk_config->config_data()->time()) || run_now) {
				_rk_check->run_rk_check();
				prev_time_rk = time(0);
			}
#endif
		}

		if (((curr_time - prev_time_sk) > config_data->time()) || run_now) {
			do_syscheck();
			prev_time_sk = time(0);
		}

		// sleep，并尽快得知config file的变更
		if (sleep_while()) {
			// todo: reload config 该功能实现需要再考虑考虑
		}
	}

	return;
}

void Syscheckd::do_syscheck() {
	auto config = std::static_pointer_cast<SyscheckConfig>(_config);
	auto config_data = config->config_data();
	auto run_data = config->run_data();

	if (run_data->scan_on_start() == 0) {
		/* Need to create the db if scan on start is not set */
		sleep(config_data->tsleep() * 10);
		create_file_db();
		sleep(config_data->tsleep() * 10);
		run_data->set_scan_on_start(1);
	}
	else {
		if (!config_data->dirs_infos().empty()) {
			info(_server_name.c_str(), "Starting syscheck scan.");
			// todo: send_syscheck_msg
		}
#ifdef WIN32
		// todo: windows
#endif
		//todo: run_db_check
	}

	sleep(config_data->tsleep() * 20);
	if (!config_data->dirs_infos().empty()) {
		info(_server_name.c_str(), "Ending syscheck scan.");
		// todo: send_syscheck_msg
	}

	// todo: send_syscheck_msg
	debug(_server_name.c_str(), "Sending database completed message..");
	return;
}

/**
 * syscheck main入口
 */
int main(int argc, char **argv) {

	auto config = std::make_shared<SyscheckConfig>();
	auto server = std::make_shared<Syscheckd>();

	config->set_server(server);
	server->set_config(config);						// server挂config

#ifdef ROOTCHECK_IN_SYSCHECK
	auto rk_config = std::make_shared<RkConfig>(SERVER, GUARDCONF, AGENT_CONF);
	auto rk_server = std::make_shared<RootCheck>();
	rk_config->set_server(rk_server);
	rk_server->set_config(std::move(rk_config));	// server挂config

	server->set_rk_server(rk_server);
#endif

	server->main(argc, argv);						// 执行main

	return 0;
}



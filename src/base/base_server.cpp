/*
 *  base_server.cpp, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年3月2日
 *      Author: yongxue@cyongxue@163.com
 */

#include <functional>

#include "base_server.h"

/* 对应BaseServer中的项，尝试读取opt */
void BaseServer::read_internal_opt() {
	info(_server_name.c_str(), "Read internal optinal ......");

	_log_flag = _config->get_define_int(_server_name, "debug", (int)LogLevel::NO, (int)LogLevel::ALL);

	return;
}

void BaseServer::main(int argc, char** argv) {

	Chroot::instance(_server_name, _base_dir);
	Log::instance();						// 此时之后的日志是打印在终端

	// 参数处理
	deal_opt(argc, argv);

	// 读取选项opt的参数
	read_internal_opt();

	// 创建日志实例，并设置level
	Log::instance()->set_log_level(_log_flag);

	// 读取claw.conf的配置，和安全相关的
	if (read_config() == -1) {
		fatal(_server_name.c_str(), "Read config file failed.");
	}

	/* Exit if testing config */
	if (_test_flag) {
		exit(0);
	}

	if (_daemon_flag) {
		do_daemon();
		Log::instance()->set_daemon(_daemon_flag);
	}

	// 具体初始化处理
	initial();

	if (create_pid(getpid()) < 0) {
		error(_server_name.c_str(), "Unable to create PID file.");
	}

	// 信号处理的调用
	deal_signal();
	info(_server_name.c_str(), "Started (pid: %d).", getpid());

	if (_chroot_flag == true) {
		Chroot::instance()->do_chroot()();
	}

	// 基类提供接口，派生类继承实现
	run();

	return;
}

void BaseServer::deal_signal() {
	auto func = [](int sig) {
		error("BaseServer", "SIGNAL [(%d)-(%s)] Received. Exit Cleaning...", sig, strsignal(sig));
		error("BaseServer", "Delete Pid file.");
		exit(1);
	};
	set_signal(func);
}

void BaseServer::set_signal(void (*func)(int)) {

	auto handle_pip = [](int sig) {return;};

	signal(SIGHUP, SIG_IGN);
	signal(SIGINT, func);
	signal(SIGQUIT, func);
	signal(SIGTERM, func);
	signal(SIGALRM, func);
	signal(SIGPIPE, handle_pip);
	return;
}

void BaseServer::set_cfg_file(std::string cfg_file) {
	_cfg_file = cfg_file;
	return;
}

/**
 * 设置进程Damon处理
 */
void BaseServer::do_daemon() {
	int fd;
	pid_t pid;

	pid = fork();
	if (pid < 0) {
		error(_server_name.c_str(), "Could not fork due to [(%d)-(%s)]", errno, strerror(errno));
		return;
	} else if (pid) {
		exit(0);
	}

	/* Become session leader */
	if (setsid() < 0) {
		error(_server_name.c_str(), "Error during setsid()-call due to [(%d)-(%s)]", errno, strerror(errno));
		return;
	}

	/* Fork again */
	pid = fork();
	if (pid < 0) {
		error(_server_name.c_str(), "Could not fork due to [(%d)-(%s)]", errno, strerror(errno));
		return;
	} else if (pid) {
		exit(0);
	}

	/* Dup stdin, stdout and stderr to /dev/null */
	if ((fd = open("/dev/null", O_RDWR)) >= 0) {
		dup2(fd, 0);
		dup2(fd, 1);
		dup2(fd, 2);

		close(fd);
	}

	/* Go to / */
	if (chdir("/") == -1) {
		error(_server_name.c_str(), "Unable to chdir to directory '%s' due to [(%d)-(%s)]", "/", errno, strerror(errno));
	}
	return;
}

/**
 * 设置服务名
 */
std::string BaseServer::server_name() const {
	return _server_name;
}

int BaseServer::create_pid(int pid) {
	std::string full_file_path = Chroot::instance()->full_file_path(OS_PIDFILE);
	full_file_path += "/" + _server_name + "-" + pid + ".pid";

	FILE *fp = fopen(full_file_path.c_str(), "a");
	if (!fp) {
		return -1;
	}

	fprintf(fp, "%d\n", pid);
	if (chmod(full_file_path.c_str(), 0640) != 0) {
		fclose(fp);
		return (-1);
	}

	fclose(fp);
	return 0;
}

void BaseServer::deal_opt(int argc, char **argv) {
	int c;

	while ((c == getopt(argc, argv, "Vtdhfc:")) != -1) {
		switch (c) {
			case 'V':
				print_version();
				break;
			case 'h':
				help_usage();
				break;
			case 'd':
				_log_flag = LogLevel::DEBUG;
				break;
			case 'f':
				_daemon_flag = 0;
				break;
			case 'c':
				if (!optarg) {
					fatal(_server_name.c_str(), "-c needs an argument.");
				}
				_cfg_file = optarg;
				break;
			case 't':
				_test_flag = 1;
				break;
			default:
				help_usage();
				break;
		}
	}

	return;
}

void BaseServer::print_header() {
	std::cerr << " " << std::endl;
	std::cerr << __claw_name << " " << __version << " - " << __author << " (" << __contact << ")" << std::endl;
	std::cerr << __site << std::endl;
	return;
}

void BaseServer::print_version() {
	std::cerr << " " << std::endl;
	std::cerr << __claw_name << " " << __version << " - " << __author << std::endl;
	std::cerr << " " << std::endl;
	std::cerr << __license << std::endl;

	exit(1);
}

void BaseServer::help_usage() {
	std::string full_file_path = Chroot::instance()->full_file_path(_cfg_file);

	print_header();
	std::cerr << "  " << _server_name << ": -[Vhdtf] [-c config]" << std::endl;
	std::cerr << "    -V          Version and license message"  << std::endl;
	std::cerr << "    -h          This help message" << std::endl;
	std::cerr << "    -d          Execute in debug mode. This parameter" << std::endl;
	std::cerr << "                can be specified multiple times" << std::endl;
	std::cerr << "                to increase the debug level." << std::endl;
	std::cerr << "    -t          Test configuration" << std::endl;
	std::cerr << "    -f          Run in foreground" << std::endl;
	std::cerr << "    -c <config> Configuration file to use (default: %s)" << std::endl;
	std::cerr << " " << std::endl;

	exit(1);
}


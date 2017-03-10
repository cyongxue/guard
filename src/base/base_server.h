/*
 *  base_server.h, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年3月2日
 *      Author: yongxue@cyongxue@163.com
 */

#ifndef SRC_BASE_BASE_SERVER_H_
#define SRC_BASE_BASE_SERVER_H_

#include "defs.h"
#include "log.h"
#include "base_config.h"

class BaseServer {
	private:
		std::string _server_name;

		LogLevel	_log_flag;				// 日志级别
		int 		_daemon_flag;

		bool		_chroot_flag;
		int			_test_flag;

		std::string	_cfg_file;
		std::string _base_dir;				// 程序的基础目录

		std::unique_ptr<BaseConfig> 	_config;				// 服务相关的配置信息

	protected:
		void print_header();
		void deal_opt(int argc, char **argv);

		int create_pid(int pid);

		void set_signal(void (*func)(int));

	public:

		BaseServer(std::string server_name): _server_name(server_name) {
			_log_flag = 0;
			_chroot_flag = 0;
			_daemon_flag = 0;
			_cfg_file = "";
			_test_flag = 0;
			_config = nullptr;
		};
		virtual ~BaseServer() = default;

		void set_config(BaseConfig* config) {
			_config = std::move(config);
			return;
		}

		void set_cfg_file(std::string cfg_file);

		void do_daemon();
		std::string server_name() const;

		void print_version();
		void help_usage();

		virtual void read_internal_opt();

		virtual int read_config() {
			info(_server_name.c_str(), "Read config ......");
			return 0;
		}

		virtual void initial() {
			info(_server_name.c_str(), "Begin initial ......");
			return;
		}

		virtual void deal_signal();

		virtual void run() {
			info(_server_name.c_str(), "Begin running ......");
			return;
		}

		void main(int argc, char** argv);

};


#endif /* SRC_BASE_BASE_SERVER_H_ */

/*
 *  rootcheck.h, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年3月18日
 *      Author: yongxue@cyongxue@163.com
 */

#ifndef SRC_SERVER_ROOTCHECK_ROOTCHECK_H_
#define SRC_SERVER_ROOTCHECK_ROOTCHECK_H_

#include <list>
#include <unordered_map>
#include <string>
#include <regex>

#include "log.h"
#include "base_server.h"
#include "rootcheck_config.h"

class RootCheck : public BaseServer {
	public:
		enum AlertType:uint8_t {
			ALERT_OK = 0,
			ALERT_SYSTEM_ERR = 1,
			ALERT_SYSTEM_CRIT = 2,
			ALERT_ROOTKIT_FOUND = 3,
			ALERT_POLICY_VIOLATION = 4,
		};
	private:
		std::list<std::string>		_rk_sys_names;
		std::list<std::string>		_rk_sys_files;

	protected:
		int send_rk_notify(AlertType type, const std::string& msg);

		int check_rc_files(FILE *fp);
		int do_check_rc_files();

		int check_rc_trojans(FILE *fp);
		int do_check_rc_trojans();

#ifdef WIN
		int do_check_rc_win();
#else
		int do_check_rc_unixaudit();
#endif
		int check_rc_if();
		int check_rc_ports();
		int check_rc_pids();
		int check_rc_sys();
		int check_rc_dev();

	public:
		RootCheck():BaseServer(SERVER) {};

		/* config 读取 */
		virtual int read_config();
		/* initial初始化 */
		virtual int initial();

		int create_write_queue();

		void run_rk_check();

		/* run */
		virtual void run();
};

#endif /* SRC_SERVER_ROOTCHECK_ROOTCHECK_H_ */

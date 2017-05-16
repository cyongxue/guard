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
#include <string>
#include <regex>

#include "log.h"
#include "base_server.h"
#include "syscheck_config.h"

#ifdef ROOTCHECK_IN_SYSCHECK
#include "rootcheck_config.h"
#include "rootcheck.h"
#endif

#define DAY_7			604800

class Syscheckd : public BaseServer {
	private:
#ifdef ROOTCHECK_IN_SYSCHECK
		std::shared_ptr<RootCheck> 		_rk_check;
#endif

	protected:
		void print_syscheck_info();
		int create_write_queue();

		/* 创建file db */
		void create_file_db();
		bool deal_day_and_time(time_t& prev_time_rk, time_t& prev_time_sk, int& curr_day);
		bool get_day_scanned(int& curr_day);

		void get_day_scanned_inwhile(time_t curr_time, int& curr_day, bool& run_now, bool& day_scanned);
		/* 休眠并返回配置文件变更处理 */
		bool sleep_while();

		/* 执行文件检查 */
		void do_syscheck();

	public:
		Syscheckd():BaseServer(SERVER) {};

#ifdef ROOTCHECK_IN_SYSCHECK
		void set_rk_server(std::shared_ptr<RootCheck> rk_server) {
			_rk_check = rk_server;
		}
#endif
		/* 读取options配置 */
		virtual void read_internal_opt();

		/* syscheck config 读取 */
		virtual int read_config();
		/* syscheck initial初始化 */
		virtual int initial();

		/* syscheck的run */
		void run();
};

#endif /* SRC_SYSCHECKD_SYSCHECK_H_ */

/*
 *  check_sys.h, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年5月9日
 *      Author: yongxue@cyongxue@163.com
 */

#ifndef SRC_SERVER_ROOTCHECK_CHECK_SYS_H_
#define SRC_SERVER_ROOTCHECK_CHECK_SYS_H_

#include <stdio.h>

#include "base_config.h"
#include "log.h"

class CheckSys {
#define CheckSysRwFile 		"rootcheck-rw-rw-rw-.txt"
#define CheckSysRwxFile 	"rootcheck-rwxrwxrwx.txt"
#define CheckSysSuidFile	"rootcheck-suid-files.txt"

	private:
		std::shared_ptr<BaseConfig>	_config;

		int		_errors;
		int 	_total;

		int		_did;					/* device id */

		FILE *  _rw;
		FILE *  _rwx;
		FILE *	_suid;

	private:
		int read_sys_file(const std::string& file_name, int do_read);

		int scan_dir(const std::string& dir_name, int do_read);
		int read_sys_dir(const std::string& dir_name, int do_read);

	public:
		CheckSys(std::shared_ptr<BaseConfig> config): _config(config) {
			_errors = 0;
			_total = 0;
			_did = 0;
			_rw = nullptr;
			_rwx = nullptr;
			_suid = nullptr;
		}
		~CheckSys();

		void check_sys();

};


#endif /* SRC_SERVER_ROOTCHECK_CHECK_SYS_H_ */

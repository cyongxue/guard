/*
 *  check_dev.h, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年5月13日
 *      Author: yongxue@cyongxue@163.com
 */

#include <stdio.h>

#include "base_config.h"

#ifndef SRC_SERVER_ROOTCHECK_CHECK_DEV_H_
#define SRC_SERVER_ROOTCHECK_CHECK_DEV_H_

class CheckDev {
	private:
		std::shared_ptr<BaseConfig>	_config;

		int		_errors;
		int 	_total;

		int read_dir(const std::string& dir_name);
		int read_file(const std::string& file_name);

	public:
		CheckDev(std::shared_ptr<BaseConfig> config): _config(config) {
			_errors = 0;
			_total = 0;
		}

		void check_dev();
};



#endif /* SRC_SERVER_ROOTCHECK_CHECK_DEV_H_ */

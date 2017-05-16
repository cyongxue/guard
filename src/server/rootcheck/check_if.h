/*
 *  check_if.h, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年4月22日
 *      Author: yongxue@cyongxue@163.com
 */

#ifndef SRC_SERVER_ROOTCHECK_CHECK_IF_H_
#define SRC_SERVER_ROOTCHECK_CHECK_IF_H_

#include <unistd.h>

#include "log.h"
#include "rootcheck_config.h"

class CheckInterface {
	private:
		std::shared_ptr<BaseConfig>	_config;

	public:
		CheckInterface(std::shared_ptr<BaseConfig> config): _config(config) { }

		void check_if();
		bool if_promisc_mode(const char* if_name);
};



#endif /* SRC_SERVER_ROOTCHECK_CHECK_IF_H_ */

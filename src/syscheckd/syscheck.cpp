/*
 *  main.cpp, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年3月2日
 *      Author: yongxue@cyongxue@163.com
 */

#include "syscheck.h"

SyscheckConfigData::SyscheckConfigData() {
	_rootcheck = 0;
	_syscheck_disable = 0;
	_skip_nfs = 0;
	_time = 600;

	_tsleep = 20;				// 这两个暂定这样
	_sleep_after = 20;
}


int SyscheckConfig::read_syscheck_config() {
	_modules |= ServerType::SYSCHECK;

	_config_data = std::make_shared<SyscheckConfigData>();
	debug(_server_name.c_str(), "Reading Configuration [%s]", _cfg_file.c_str());

	/* 读取配置文件 */


	return 0;
}

int SyscheckConfig::read_agent_config() {
	_modules |= ServerType::AGENT_CONFIG;
}


/**
 * syscheck main入口
 */
int main(int argc, char **argv) {

	auto config = new SyscheckConfig();
	auto server = new Syscheckd();

	config->set_server(server);
	server->set_config(std::move(config));

	server->main(argc, argv);

	return 0;
}



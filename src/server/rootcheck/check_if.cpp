/*
 *  check_if.cpp, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年4月22日
 *      Author: yongxue@cyongxue@163.com
 */

#ifdef WIN

#include "check_if.h"

void CheckInterface::check_if() {
	return;
}

#else
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include "check_if.h"

/**
 * execute the ifconfig command to know if is promiscuous mode
 * promiscuous mode: 混杂模式，可以监听所有进过网卡的数据包，可以实现流量分析。
 */
bool CheckInterface::if_promisc_mode(const char* if_name) {
	auto config = std::static_pointer_cast<RkConfig>(_config);

	std::string ifconfig = "ifconfig " + if_name + " | grep PROMISC > /dev/null 2>&1";
	if (system(ifconfig.c_str()) == 0) {
		debug(config->server_name().c_str(), "'%s' interface is promiscuous.");
		return true;
	}
	debug(config->server_name().c_str(), "'%s' interface not promiscuous.");
	return false;
}

/* Check all interfaces for promiscuous mode */
void CheckInterface::check_if() {
	auto config = std::static_pointer_cast<RkConfig>(_config);
	auto server_name = config->server_name();

	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		error(server_name.c_str(), "Error checking interfaces (socket)");
		return;
	}

	struct ifreq tmp_str[16];
	memset(tmp_str, 0, sizeof(tmp_str));

	struct ifconf if_conf;
	if_conf.ifc_len = sizeof(tmp_str);
	if_conf.ifc_buf = (caddr_t)(tmp_str);
	if (ioctl(fd, SIOCGIFCONF, &if_conf) < 0) {
		error(server_name.c_str(), "Error checking interfaces (ioctl)");
		close(fd);
		return;
	}

	struct ifreq* if_conf_end = (struct ifreq*)(void*)((char *)tmp_str + if_conf.ifc_len);
	struct ifreq* if_conf_start = tmp_str;

	/* Loop over all interfaces */
	int errors = 0;
	int total = 0;
	struct ifreq result_if;
	for (; if_conf_start < if_conf_end; if_conf_start++) {
		debug(server_name.c_str(), "detecting interface '%s' ...", if_conf_start->ifr_name);
		strncpy(result_if.ifr_name, if_conf_start->ifr_name, sizeof(result_if.ifr_name));

		/* get information from each interface */
		if (ioctl(fd, SIOCGIFFLAGS, (char *)&result_if) == -1) {
			info(server_name.c_str(), "interface '%s', ioctl ifflags failed.", if_conf_start->ifr_name);
			continue;
		}
		total++;
		/* check if is promisc mode */
		if (result_if.ifr_flags & IFF_PROMISC) {
			if (if_promisc_mode(result_if.ifr_name)) {
				std::string op_msg = "Interface '" + result_if.ifr_name + "' in promiscuous mode.";
				// todo: notify_rk(ALERT_SYSTEM_CRIT, op_msg);
				debug(server_name.c_str(), "interface '%s' IFF_PROMISC and ifconfig also promiscuous.", result_if.ifr_name);
			}
			else {
				std::string op_msg = "Interface '" + result_if.ifr_name + "' in promiscuous mode, but ifconfig is not "
						"showing it(probably trojaned).";
				// todo: notify_rk(ALERT_ROOTKIT_FOUND, op_msg);
				debug(server_name.c_str(), "interface '%s' IFF_PROMISC but ifconfig not promiscuous.", result_if.ifr_name);
			}
			errors++;
		}
	}
	close(fd);

	debug(server_name.c_str(), "check if PROMISC mode: total=%d, errors=%s", total, errors);
	if (errors == 0) {
		std::string op_msg = "No problem detected on ifconfig/ifs. Analyzed " + total + " interfaces.";
		// todo: notify_rk(ALERT_OK, op_msg);
	}

	return;
}
#endif

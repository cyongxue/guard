/*
 *  check_port.cpp, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年4月24日
 *      Author: yongxue@cyongxue@163.com
 */

#include <sys/socket.h>

#include "log.h"
#include "check_port.h"

/**
 * check open ports
 */
void CheckPorts::check_open_ports() {
	_open_ports_str = "The following ports are open:";

	try_access_ports();

	// todo: notify_rk(ALERT_OK, _open_ports_str);

	return;
}

/**
 * check port
 */
void CheckPorts::check_ports() {
	(void)scan_ports(ProtoType::TCP);
	(void)scan_ports(ProtoType::UDP);

	if (_errors == 0) {
		std::string op_msg = "No kernel-level rootkit hiding any port."
				"\n		Netstat is acting correctly. Analyzed " + _total + "ports.";
		// todo: notify_rk(ALERT_OK, op_msg);
	}

	return;
}

void CheckPorts::add_open_port(CheckPorts::ProtoType proto, int port) {
	if (proto == CheckPorts::ProtoType::TCP) {
		for (auto one: _tcp_ports) {
			if (one == port) {
				break;
			}
			_tcp_ports.push_back(port);
		}
	}
	else if (proto == CheckPorts::ProtoType::UDP) {
		for (auto one: _udp_ports) {
			if (one == port) {
				break;
			}
			_udp_ports.push_back(port);
		}
	}
	return;
}

/**
 * 1. 利用socket探测port打开情况；
 * 2. 利用netstat检查。
 * 如果两者不一样，且netstat看不到，而socket检测出来，那么netstat就可能存在问题
 */
int CheckPorts::scan_ports(CheckPorts::ProtoType proto) {
	auto config = std::static_pointer_cast<RkConfig>(_config);
	auto module = config->server_name();

	for (auto i = 0; i <= MAX_PORT_NUM; i++) {
		_total++;
		/* 先查看端口port是否打开 */
		if ((is_open_ipv4_port(proto, i) == 1) || (is_open_ipv6_port(proto, i) == 1)) {
			/* 执行netstat，进一步确认是否打开 */
			if (netstat(proto, i)) {
				debug(module.c_str(), "Port '%d' open, and netstat can see.", i);
				continue;
			}

			/* 否则，sleep 2 second，check again */
			sleep(2);
			if (!netstat(proto, i) && ((is_open_ipv4_port(proto, i) == 1) || (is_open_ipv6_port(proto, i) == 1))) {
				info(module.c_str(), "Port '%d' open, but netstat can't see", i);
				/* netstat看不到，但是bind发现是open的 */
				_errors++;
				std::string type = (proto == CheckPorts::ProtoType::TCP)? "tcp": "udp";
				std::string op_msg = "Port '" + i + "'(" + type + ") hidden. "
						"Kernel-level rootkit or trojaned version of netstat.";
				// todo: notify_rk(ALERT_ROOTKIT_FOUND, op_msg);
			}
		}

		if (_errors > 20) {
			std::string type = (proto == CheckPorts::ProtoType::TCP)? "tcp": "udp";
			std::string op_msg = "Excessive number of '" + type + "' ports hidden. "
					"It maybe a false-positive or something really bad is going on.";
			// todo: notify_rk(ALERT_SYSTEM_CRIT, op_msg);
			return 0;
		}
	}

	return 0;
}

/**
 * 这里是采用的netstat，后续考虑不用命令，而是直接分析系统文件
 */
int CheckPorts::netstat(CheckPorts::ProtoType proto, int port) {
	auto config = std::static_pointer_cast<RkConfig>(_config);

	std::string cmd;
	if (proto == CheckPorts::ProtoType::UDP) {
#if defined(sun) || defined(__sun__)
		cmd = "netstat -an -P udp | grep \"[^0-9]" + port + " \" > /dev/null 2>&1";
#else
		cmd = "netstat -an | grep \"^udp\" | grep \"[^0-9]" + port + " \" > /dev/null 2>&1";
#endif
	}
	else if (proto == CheckPorts::ProtoType::TCP) {
#if defined(sun) || defined(__sun__)
		cmd = "netstat -an -P tcp | grep \"[^0-9]" + port + " \" > /dev/null 2>&1";
#else
		cmd = "netstat -an | grep \"^tcp\" | grep \"[^0-9]" + port + " \" > /dev/null 2>&1";
#endif
	}
	else {
		error(config->server_name().c_str(), "Netstat error (wrong protocol).");
		return 0;
	}

	debug(config->server_name().c_str(), "The cmd: '%s'", cmd.c_str());
	int ret = system(cmd.c_str());
	if (ret == 1) {
		return 0;
	}

	return 1;
}

/**
 * 尝试连接
 * return 	-1			error
 * 			0			no open
 * 			1			open
 */
int CheckPorts::is_open_ipv4_port(CheckPorts::ProtoType proto, int port) {
	auto config = std::static_pointer_cast<RkConfig>(_config);
	auto module = config->server_name();

	/* create socket */
	int sock;
	if (proto == CheckPorts::ProtoType::UDP) {
		if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
			error(module.c_str(), "create socket failed for udp, port: %d.", port);
			return -1;
		}
	}
	else if (proto == CheckPorts::ProtoType::TCP) {
		if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
			error(module.c_str(), "create socket failed for tcp, port: %d", port);
			return -1;
		}
	}
	else {
		error(module.c_str(), "input proto error: %d, port: %d", proto, port);
		return -1;
	}

	struct sockaddr_in server;
	memset(&server, 0, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	/* bind socket, if can't bind, it means the port is open */
	int is_open = 0;
	if (bind(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
		info(module.c_str(), "bind failed, port: %d, maybe is open", port);
		is_open = 1;
	}
	close(sock);

	add_open_port(proto, port);

	return is_open;
}

/**
 * return 	-1		error
 * 			0		no open
 * 			1		open
 */
int CheckPorts::is_open_ipv6_port(CheckPorts::ProtoType proto, int port) {
	auto config = std::static_pointer_cast<RkConfig>(_config);
	auto module = config->server_name();

	/* create socket */
	int sock;
	if (proto == CheckPorts::ProtoType::UDP) {
		if ((sock = socket(PF_INET6, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
			error(module.c_str(), "create socket failed for udp, port: %d.", port);
			return -1;
		}
	}
	else if (proto == CheckPorts::ProtoType::TCP) {
		if ((sock = socket(PF_INET6, SOCK_STREAM, IPPROTO_TCP)) < 0) {
			error(module.c_str(), "create socket failed for tcp, port: %d.", port);
			return -1;
		}
	}
	else {
		error(module.c_str(), "input proto error: %d, port: %d", proto, port);
		return -1;
	}

	struct sockaddr_in6 server;
	memset(&server, 0, sizeof(server));
	server.sin6_family = AF_INET6;
	server.sin6_port = htons(port);
	memcpy(&server.sin6_addr.s6_addr, &in6addr_any, sizeof(in6addr_any));
	/* bind socket, if can't bind, it means the port is open */
	int is_open = 0;
	if (bind(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
		info(module.c_str(), "bind failed, port: %d, maybe is open", port);
		is_open = 1;
	}
	close(sock);

	add_open_port(proto, port);

	return is_open;
}

void CheckPorts::try_access_ports() {
	/* test connection tcp */
	for (auto one: _tcp_ports) {
		if ((connect_ipv4_port(CheckPorts::ProtoType::TCP, one) == 1) ||
				(connect_ipv6_port(CheckPorts::ProtoType::TCP, one) == 1)) {
			std::string op_msg;
			if (_ports_open == 0) {
				op_msg = "\n      " + one + " (tcp),";
			}
			else {
				op_msg = one + " (tcp),";
			}
			_open_ports_str += op_msg;
			_ports_open++;
		}

		if (_ports_open >= 4) {
			_ports_open++;
		}
	}

	/* test connection udp */
	for (auto one: _udp_ports) {
		if ((connect_ipv4_port(CheckPorts::ProtoType::UDP, one) == 1) ||
				(connect_ipv6_port(CheckPorts::ProtoType::UDP, one) == 1)) {
			std::string op_msg;
			if (_ports_open == 0) {
				op_msg = "\n      " + one + " (udp),";
			}
			else {
				op_msg = one + " (udp),";
			}
			_open_ports_str += op_msg;
			_ports_open++;
		}

		if (_ports_open >= 4) {
			_ports_open++;
		}
	}

	return;
}

/**
 * return 	-1		failed
 * 			0		can't connect
 * 			1		can connect
 */
int CheckPorts::connect_ipv4_port(CheckPorts::ProtoType proto, int port) {
	int sock;
	if (proto == CheckPorts::ProtoType::UDP) {
		if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
			return -1;
		}
	}
	else if (proto == CheckPorts::ProtoType::TCP) {
		if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
			return -1;
		}
	} else {
		return -1;
	}

	struct sockaddr_in server;
	memset(&server, 0, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	server.sin_addr.s_addr = inet_addr("127.0.0.1");

	int ret = 0;
	if (connect(sock, (struct sockaddr *)&server, sizeof(server)) == 0) {
		ret = 1;
	}
	close(sock);

	return 0;
}

/**
 * return 	-1		failed
 * 			0		can't connect
 * 			1		can connect
 */
int CheckPorts::connect_ipv6_port(CheckPorts::ProtoType proto, int port) {
	int sock;
	if (proto == IPPROTO_UDP) {
		if ((sock = socket(PF_INET6, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
			return -1;
		}
	} else if (proto == IPPROTO_TCP) {
		if ((sock = socket(PF_INET6, SOCK_STREAM, IPPROTO_TCP)) < 0) {
			return -1;
		}
	}
	else {
		return -1;
	}

	struct sockaddr_in6 server6;
	memset(&server6, 0, sizeof(server6));
#ifdef WIN
	// todo:
#else
	server6.sin6_family = AF_INET6;
	inet_pton(AF_INET6, "::1",&server6.sin6_addr.s6_addr);
#endif
	server6.sin6_port = htons(port);

	int ret = 0;
	if (connect(sock, (struct sockaddr*)&server6, sizeof(server6)) == 0) {
		ret = 1;
	}
	close(sock);

	return ret;
}


/*
 *  check_port.h, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年4月24日
 *      Author: yongxue@cyongxue@163.com
 */

#ifndef SRC_SERVER_ROOTCHECK_CHECK_PORT_H_
#define SRC_SERVER_ROOTCHECK_CHECK_PORT_H_

#include <netinet/in.h>

class CheckPorts {
	public:
		enum ProtoType {
			UDP = IPPROTO_UDP,
			TCP = IPPROTO_TCP,
		};

	private:
		std::shared_ptr<BaseConfig>	_config;

		/* 不区分ipv4、ipv6，只要open就算port开 */
		std::vector<int>			_tcp_ports;
		std::vector<int>			_udp_ports;

		std::string					_open_ports_str;
		int							_ports_open;

		int		_total;
		int 	_errors;

	private:
		int scan_ports(CheckPorts::ProtoType proto);

		int is_open_ipv4_port(CheckPorts::ProtoType proto, int port);
		int is_open_ipv6_port(CheckPorts::ProtoType proto, int port);
		void add_open_port(CheckPorts::ProtoType proto, int port);

		int netstat(CheckPorts::ProtoType proto, int port);

		void try_access_ports();
		int connect_ipv4_port(CheckPorts::ProtoType proto, int port);
		int connect_ipv6_port(CheckPorts::ProtoType proto, int port);

	public:
		const int MAX_PORT_NUM = 65535;

		CheckPorts(std::shared_ptr<BaseConfig> config): _config(config) {
			_ports_open = 0;
			_total = 0;
			_errors = 0;
		}

		void check_ports();
		void check_open_ports();
};



#endif /* SRC_SERVER_ROOTCHECK_CHECK_PORT_H_ */

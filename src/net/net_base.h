/*
 *  net_base.h, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年3月21日
 *      Author: yongxue@cyongxue@163.com
 */

#ifndef SRC_NET_NET_BASE_H_
#define SRC_NET_NET_BASE_H_

#include <unistd.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "log.h"

namespace net {

#define NET		"Net"
#define OS_SIZE_6144    6144

class Net {
	private:
		int 		_fd;

		std::string	_module;

	public:
		Net(): _fd(-1), _module(NET) {
			info(_module.c_str(), "Net struct ...");
		};
		virtual ~Net() {
			info(_module.c_str(), "Net destruct ...");
			if (_fd != -1) {
				close(_fd);
			}
		};

		void close_fd();

		int get_sendbuf_size();
		int get_recvbuf_size();

		virtual int recv();
		virtual int send();
};

class UnixDomain: public Net {
	private:
		std::string				_path;
		struct sockaddr_un		_unix_addr;

		int					_max_msg_size;

	public:
		UnixDomain(const std::string& path): _path(path), _max_msg_size(0) {
			info(_module.c_str(), "UnixDomain struct ...");
			memset(&_unix_addr, 0, sizeof(_unix_addr));
			_unix_addr.sun_family = AF_UNIX;
			strncpy(_unix_addr.sun_path, path.c_str(), sizeof(_unix_addr.sun_path) - 1);
		}
		virtual ~UnixDomain() {
			info(_module.c_str(), "UnixDomain destruct ...");
		}

	private:
		/* 设置option buf size */
		int set_max_sendbuf_size(int max_size);
		int set_max_recvbuf_size(int max_size);
};

class NetSocket: public Net {
	private:
		std::string 		_ip;
		std::string			_port;

		unsigned int 		_proto;

		struct addrinfo		_addr;

	public:
		NetSocket(const std::string& ip, unsigned int port, unsigned int proto):
			_ip(ip), _port(port), _proto(proto) {}
		virtual ~NetSocket() {};

};

}

#endif /* SRC_NET_NET_BASE_H_ */

/*
 *  net_server.h, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年3月21日
 *      Author: yongxue@cyongxue@163.com
 */

#ifndef SRC_NET_NET_SERVER_H_
#define SRC_NET_NET_SERVER_H_

#include "net_base.h"

namespace net {

class RemoteClient {
	private:
		std::string 				_ip;
		int							_fd;
		struct sockaddr_storage 	_addr;

	public:
		RemoteClient(const std::string& ip, int fd, const struct sockaddr_storage& addr):_ip(ip), _fd(fd), _addr(addr) {};
};

class Server {
	public:
		virtual ~Server() = default;

		virtual int bind_server(const int max_msg);
		virtual int to_listen();
		virtual std::shared_ptr<RemoteClient> to_accept();
};


class UnixDomainServer: public Server, public UnixDomain {
	private:
		mode_t			_mode;

	public:
		UnixDomainServer(const std::string& path, mode_t mode): _mode(mode) {
			info(NET, "UnixDomainServer struct ...");
		};
		~UnixDomainServer() {
			info(NET, "UnixDomainServer destruct ...");
		}

		virtual int bind_server(const int max_msg);
};
std::shared_ptr<UnixDomainServer> read_msg_queue(const std::string& queue_path);

class UdpServer: public Server, public NetSocket {
	private:

	public:
		UdpServer(const std::string& ip, unsigned int port): NetSocket(ip, port, IPPROTO_UDP) {}
		~UdpServer() {};

		virtual int bind_server(const int max_msg);
};

class TcpServer: public Server, public NetSocket {
	private:

	public:
		TcpServer(const std::string& ip, unsigned int port): NetSocket(ip, port, IPPROTO_TCP) {}
		~TcpServer() {};

		virtual int bind_server(const int max_msg);
		virtual int to_listen();
		virtual std::shared_ptr<RemoteClient> to_accept();
};

}

#endif /* SRC_NET_NET_SERVER_H_ */

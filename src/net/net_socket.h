/*
 *  net_socket.h, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年6月3日
 *      Author: yongxue@cyongxue@163.com
 */

#ifndef SRC_NET_NET_SOCKET_H_
#define SRC_NET_NET_SOCKET_H_

#include "net_base.h"

namespace net {
/**
 * socket client
 */
class SocketClient: public Client, public NetSocket {
	private:

	public:
		/**
		 * proto：取值为：IPPROTO_TCP		IPPROTO_UDP
		 */
		SocketClient(const std::string& ip, unsigned int port, unsigned int proto): NetSocket(ip, port, proto) {
			// something
		}

		virtual int connect_server(const int max_msg);

};

/**
 * udp server
 */
class UdpServer: public Server, public NetSocket {
	private:

	public:
		/* 构造函数 */
		UdpServer(const std::string& ip, unsigned int port): NetSocket(ip, port, IPPROTO_UDP) {}
		~UdpServer() {};

		virtual int bind_server(const int max_msg);
};

/**
 * tcp server
 */
class TcpServer: public Server, public NetSocket {
	private:

	public:
		/* 构造函数 */
		TcpServer(const std::string& ip, unsigned int port): NetSocket(ip, port, IPPROTO_TCP) {}
		~TcpServer() {};

		virtual int bind_server(const int max_msg);
		virtual int to_listen();
		/* Accept a TCP connection */
		virtual std::shared_ptr<RemoteClient> to_accept();
};

}

#endif /* SRC_NET_NET_SOCKET_H_ */

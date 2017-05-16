/*
 *  net_client.h, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年3月21日
 *      Author: yongxue@cyongxue@163.com
 */

#ifndef SRC_NET_NET_CLIENT_H_
#define SRC_NET_NET_CLIENT_H_

#include "net_base.h"

namespace net {

class Client {
	public:
		virtual ~Client() = default;

		virtual int connect_server(const int max_msg);
};

class UnixDomainClient: public Client, public UnixDomain {
	private:

	public:
		UnixDomainClient(const std::string& path) {
			info(_module.c_str(), "UnixDomainClient struct ...");
		};
		~UnixDomainClient() {
			info(_module.c_str(), "UnixDomainClient destruct ...");
		}

		virtual int connect_server(const int max_msg);
};

std::shared_ptr<UnixDomainClient> write_msg_queue(const std::string& queue_path);

class SocketClient: public Client, public NetSocket {
	private:

	public:
		virtual int connect_server(const int max_msg);
};

}

#endif /* SRC_NET_NET_CLIENT_H_ */

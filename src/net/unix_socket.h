/*
 *  unix_socket.h, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年6月3日
 *      Author: yongxue@cyongxue@163.com
 */

#ifndef SRC_NET_UNIX_SOCKET_H_
#define SRC_NET_UNIX_SOCKET_H_

#include "net_base.h"

namespace net {

/**
 * unix domain client
 */
class UnixDomainClient: public Client, public UnixDomain {
	private:

	public:
		UnixDomainClient(const std::string& path): UnixDomain(path) {
			info(_module.c_str(), "UnixDomainClient struct ...");
		};
		~UnixDomainClient() {
			info(_module.c_str(), "UnixDomainClient destruct ...");
		}

		virtual int connect_server(const int max_msg);

		/* 和业务强相关的消息发送， todo: xxxxxxx*/
//		int send_msg(const std::string& msg, const std::string& mq_msg, char mq_type);
};

/**
 * unix domain server
 */
class UnixDomainServer: public Server, public UnixDomain {
	public:
		class UnixDomainRemoteClient: public UnixDomain {
			public:
				UnixDomainRemoteClient(): UnixDomain() {}

				void set_client_fd(int client_fd) { _fd = client_fd; return; }
				int get_client_fd() const { return _fd; }

				void set_client_add(const struct sockaddr_un& client_addr) {
					_unix_addr = client_addr;
					return;
				}
				struct sockaddr_un get_client_add() const {
					return _unix_addr;
				}
		};
	private:
		mode_t			_mode;
		std::list<std::shared_ptr<UnixDomainRemoteClient>> 	_client_list;

		/* 封装的epoll方式 */
		std::shared_ptr<Epoll>		_epoll;

	public:
		UnixDomainServer(const std::string& path, mode_t mode): _mode(mode), UnixDomain(path) {
			/* 先创建epoll对象 */
			_epoll = std::make_shared<Epoll>();
		};
		~UnixDomainServer() {
			unlink(_path.c_str());
		}

		virtual int bind_server(const int max_msg);
		virtual int to_listen();
		virtual std::shared_ptr<UnixDomainServer::UnixDomainRemoteClient> to_accept();

		std::shared_ptr<Epoll> epoll() const { return _epoll; }
		/* loop的实现，支持forever和no forever */
		int loop_once(int wait_ms);
		int run_loop(int wait_ms, bool is_forever);
};
//std::shared_ptr<UnixDomainServer> read_msg_queue(const std::string& queue_path);

}



#endif /* SRC_NET_UNIX_SOCKET_H_ */

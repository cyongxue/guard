/*
 *  queue.h, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年6月3日
 *      Author: yongxue@cyongxue@163.com
 */

#ifndef SRC_NET_QUEUE_H_
#define SRC_NET_QUEUE_H_

#include "net_base.h"

namespace net {

class UnixDomainQueueIn: public Client, public UnixDomain {
	private:

	public:
		UnixDomainQueueIn(const std::string& path): UnixDomain(path) {
			debug(_module.c_str(), "UnixDomainQueueWrite struct ...");
		};
		~UnixDomainQueueIn() {
			debug(_module.c_str(), "UnixDomainQueueWrite destruct ...");
		}

		virtual int connect_server(const int max_msg);
		/* queue in recv msg, pass */
		virtual int recv_unix_msg(char *buf, int buf_len) { return 0; };
};

class UnixDomainQueueOut: public Server, public UnixDomain {
	private:
		mode_t			_mode;

	public:
		UnixDomainQueueOut(const std::string& path, mode_t mode):
			_mode(mode), UnixDomain(path) {
		}
		~UnixDomainQueueOut() {
			unlink(_path.c_str());
		}

		virtual int bind_server(const int max_msg);
		/* queue in recv msg, pass */
		virtual int send_unix_msg(const std::string& msg) { return 0; };
};

}

#endif /* SRC_NET_QUEUE_H_ */

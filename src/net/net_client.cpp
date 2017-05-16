/*
 *  net_client.cpp, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年3月21日
 *      Author: yongxue@cyongxue@163.com
 */

#include "net_client.h"

namespace net {

int UnixDomainClient::connect_server(const int max_msg) {

	_fd = socket(PF_UNIX, SOCK_DGRAM, 0);
	if (_fd < 0) {
		error(_module.c_str(), "create client socket failed: '%d: %s'", errno, strerror(errno));
		return -1;
	}

	if (connect(_fd, (struct sockaddr *)&_unix_addr, SUN_LEN(&_unix_addr)) < 0) {
		error(_module.c_str(), "connect server failed: '%d: %s'", errno, strerror(errno));
		close_fd();
		return -1;
	}

	if (set_max_sendbuf_size(max_msg) < 0) {
		error(_module.c_str(), "set_max_sendbuf_size failed.");
		close_fd();
		return -1;
	}

	return 0;
}

std::shared_ptr<UnixDomainClient> write_msg_queue(const std::string& queue_path) {
	if (util::file_mod_time(queue_path) < 0) {
		sleep(1);
		if (util::file_mod_time(queue_path) < 0) {
			sleep(5);
			if (util::file_mod_time(queue_path) < 0) {
				sleep(15);
				if (util::file_mod_time(queue_path) < 0) {
					error(NET, "Queue '%s' not accessible: '%s'.", queue_path.c_str(), "Queue not found.");
					return nullptr;
				}
			}
		}
	}

	auto queue = std::make_shared<net::UnixDomainClient>(queue_path);
	if (queue == nullptr) {
		return nullptr;
	}
	if (queue->connect_server(OS_SIZE_6144 + 256) < 0) {
		sleep(1);
		if (queue->connect_server(OS_SIZE_6144 + 256) < 0) {
			sleep(2);
			if (queue->connect_server(OS_SIZE_6144 + 256) < 0) {
				error(NET, "Queue '%s' not accessible.", queue_path.c_str());
				return nullptr;
			}
		}
	}

	debug(NET, "(unix_domain) Maximum send buffer set to: '%d'.", queue->get_sendbuf_size());
	return queue;
}

int SocketClient::connect_server(const int max_msg) {

	if (_ip.empty() || _port.empty()) {
		error(_module.c_str(), "No server addr ...");
		return -1;
	}

	struct addrinfo hints;
	memset(&hints, 0, sizeof(struct addrinfo));
	/* Allow IPv4 or IPv6 if local_ip isn't specified */
	hints.ai_family = AF_UNSPEC;
	hints.ai_protocol = _proto;
	if (_proto == IPPROTO_TCP) {
		hints.ai_socktype = SOCK_STREAM;
	} else if (_proto == IPPROTO_UDP) {
		hints.ai_socktype = SOCK_DGRAM;
	} else {
		error(_module.c_str(), "Input proto type is wrong: '%d'", _proto);
		return -1;
	}
	hints.ai_flags = 0;

	struct addrinfo *result;
	auto ret = getaddrinfo(_ip.c_str(), _port.c_str(), &hints, &result);
	if (ret != 0) {
		error(_module.c_str(), "getaddrinfo failed.");
		return -1;
	}

	int sock = -1;
	struct addrinfo *rp;
	for (rp = result; rp != NULL; rp = rp->ai_next) {
		sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (sock == -1) {
			error(_module.c_str(), "create net client socket failed: '%d:%s'", errno, strerror(errno));
			continue;
		}

		if (connect(sock, rp->ai_addr, rp->ai_addrlen) != -1) {
			break;                  /* Success */
		}
		else {
			error(_module.c_str(), "bind net server socket failed: '%d:%s'", errno, strerror(errno));
			close(sock);
		}
	}

	if (rp == nullptr) {
		error(_module.c_str(), "No address can use for: '%s:%s'", _ip.c_str(), _port.c_str());
		freeaddrinfo(result);           /* No longer needed */
		return -1;
	}

	info(_module.c_str(), "Connected to '%s::%s'", _ip.c_str(), _port.c_str());
	_fd = sock;

	freeaddrinfo(result);           /* No longer needed */
	return 0;
}

}


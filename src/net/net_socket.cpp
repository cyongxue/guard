/*
 *  net_socket.cpp, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年6月3日
 *      Author: yongxue@cyongxue@163.com
 */

#include "net_socket.h"

namespace net {

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

int UdpServer::bind_server(const int max_msg) {
	struct addrinfo hints;

	/* 构造地址类型 */
	memset(&hints, 0, sizeof(struct addrinfo));
#ifdef AI_V4MAPPED
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 and IPv6 */
    hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG | AI_V4MAPPED;
#else
    /* Certain *BSD OS (eg. OpenBSD) do not allow binding to a
       single-socket for both IPv4 and IPv6 per RFC 3493.  This will
       allow one or the other based on _ip. */
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_flags = AI_PASSIVE;
#endif
    hints.ai_protocol = IPPROTO_UDP;
	hints.ai_socktype = SOCK_DGRAM;

	/* 解析本地地址 */
	struct addrinfo *result;
	struct addrinfo *rp;
	auto ret = getaddrinfo(_ip.c_str(), _port.c_str(), &hints, &result);
	if (ret != 0) {
		error(_module.c_str(), "getaddrinfo failed.");
		return -1;
	}

	/* bind一个socket */
	int sock = -1;
	for (rp = result; rp != NULL; rp = rp->ai_next) {
		sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (sock == -1) {
			error(_module.c_str(), "create net server socket failed: '%d:%s'", errno, strerror(errno));
			continue;
		}

		if (bind(sock, rp->ai_addr, rp->ai_addrlen) == 0) {
			break;                  /* Success */
		}
		else {
			error(_module.c_str(), "bind net server socket failed: '%d:%s'", errno, strerror(errno));
			close(sock);
		}
	}
	freeaddrinfo(result);           /* No longer needed */

	if (rp == nullptr) {
		error(_module.c_str(), "No address can use for: '%s:%s'", _ip.c_str(), _port.c_str());
		return -1;
	}
	_fd = sock;

	info(_module.c_str(), "Success bind socket: %d", _fd);
	return 0;
}

int TcpServer::bind_server(const int max_msg) {
	struct addrinfo hints;

	/* 构造地址类型 */
	memset(&hints, 0, sizeof(struct addrinfo));
#ifdef AI_V4MAPPED
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 and IPv6 */
    hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG | AI_V4MAPPED;
#else
    /* Certain *BSD OS (eg. OpenBSD) do not allow binding to a
       single-socket for both IPv4 and IPv6 per RFC 3493.  This will
       allow one or the other based on _ip. */
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_flags = AI_PASSIVE;
#endif
    hints.ai_protocol = IPPROTO_TCP;
	hints.ai_socktype = SOCK_STREAM;

	/* 解析本地地址 */
	struct addrinfo *result;
	struct addrinfo *rp;
	auto ret = getaddrinfo(_ip.c_str(), _port.c_str(), &hints, &result);
	if (ret != 0) {
		error(_module.c_str(), "getaddrinfo failed.");
		return -1;
	}

	/* bind一个socket */
	int sock = -1;
	for (rp = result; rp != NULL; rp = rp->ai_next) {
		sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (sock == -1) {
			error(_module.c_str(), "create net server socket failed: '%d:%s'", errno, strerror(errno));
			continue;
		}

		int flag = 1;
		if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&flag, sizeof(flag)) < 0) {
			error(_module.c_str(), "set reuse addr for tcp failed: '%d:%s'", errno, strerror(errno));
			freeaddrinfo(result);           /* No longer needed */
			close(sock);
			return -1;
		}

		if (bind(sock, rp->ai_addr, rp->ai_addrlen) == 0) {
			break;                  /* Success */
		}
		else {
			error(_module.c_str(), "bind net server socket failed: '%d:%s'", errno, strerror(errno));
			close(sock);
		}
	}
	freeaddrinfo(result);           /* No longer needed */

	if (rp == nullptr) {
		error(_module.c_str(), "No address can use for: '%s:%s'", _ip.c_str(), _port.c_str());
		return -1;
	}
	_fd = sock;

	info(_module.c_str(), "Success bind socket: %d", _fd);
	return 0;
}

int TcpServer::to_listen() {
	if (listen(_fd, 1024) < 0) {
		error(_module.c_str(), "listen socket Failed.");
		close_fd();
		return -1;
	}

	return 0;
}

/**
 * Accept a TCP connection
 */
std::shared_ptr<RemoteClient> TcpServer::to_accept() {
	struct sockaddr_storage cli_addr;
	unsigned int addr_len = sizeof(cli_addr);
	memset(&cli_addr, 0, sizeof(cli_addr));

	auto client_sock = accept(_fd, (struct sockaddr *) &cli_addr, &addr_len);
	if (client_sock < 0) {
		error(_module.c_str(), "Accept socket Failed.");
		return nullptr;
	}

	auto src_ip = sockaddr_to_str((struct sockaddr *) &cli_addr);
	if (src_ip.empty()) {
		warn(_module.c_str(), "parse client Address Failed.");
	}

	return std::make_shared<RemoteClient>(src_ip, client_sock, cli_addr);
}

}

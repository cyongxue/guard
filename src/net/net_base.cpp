/*
 *  net_base.cpp, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年3月21日
 *      Author: yongxue@cyongxue@163.com
 */

#include "net_base.h"

namespace net {

void Net::close_fd() {
	if (_fd != -1) {
#ifdef WIN
		closesocket(_fd);
#else
		close(_fd);
#endif	/* WIN */
		_fd = -1;
	}
	return;
}

int Net::get_sendbuf_size() {
	int len = 0;
	socklen_t optlen = sizeof(len);

	if (getsockopt(_fd, SOL_SOCKET, SO_SNDBUF, &len, &optlen) == -1) {
		error(_module.c_str(), "getsockopt sendbuf failed: '%d:%s'", errno, strerror(errno));
		return -1;
	}

	return len;
}

int Net::get_recvbuf_size() {
	int len = 0;
	socklen_t optlen = sizeof(len);

	if (getsockopt(_fd, SOL_SOCKET, SO_RCVBUF, &len, &optlen) == -1) {
		error(_module.c_str(), "getsockopt recvbuf failed: '%d:%s'", errno, strerror(errno));
		return -1;
	}

	return len;
}

int UnixDomain::recv_unix_msg(char *buf, int buf_len) {
	ssize_t recvd;

	socklen_t unix_addr_len = sizeof(_unix_addr);
	if ((recvd = recvfrom(_fd, buf, buf_len - 1, 0, (struct sockaddr *)&_unix_addr, &unix_addr_len)) < 0) {
		error(_module.c_str(), "recvfrom error.");
		return 0;
	}
	buf[recvd] = '\0';

	return recvd;
}

int UnixDomain::send_unix_msg(const std::string& msg) {
	if (send(_fd, msg.c_str(), msg.length(), 0) < 0) {
		if (errno == ENOBUFS) {
			error(_module.c_str(), "send msg failed: %s.", strerror(errno));
			return Net::ERR_SOCKBUSY;
		}

		error(_module.c_str(), "send msg failed: %s.", strerror(errno));
		return Net::ERR_SOCKERR;
	}

	return 0;
}

int UnixDomain::set_max_sendbuf_size(int max_size) {

	_max_msg_size = max_size;

	int buf_size;
	unsigned int opt_len = sizeof(buf_size);
	if (getsockopt(_fd, SOL_SOCKET, SO_SNDBUF, &buf_size, &opt_len) == -1) {
		error(_module.c_str(), "getsockopt sendbuf failed: '%d:%s'", errno, strerror(errno));
		return -1;
	}

	if (buf_size < _max_msg_size) {
		buf_size = _max_msg_size;
		if (setsockopt(_fd, SOL_SOCKET, SO_SNDBUF, &buf_size, opt_len) < 0) {
			error(_module.c_str(), "setsockopt sendbuf failed: '%d:%s'", errno, strerror(errno));
			return -1;
		}
	}
	info(_module.c_str(), "setsockopt sendbuf: %d", buf_size);
	return 0;
}

int UnixDomain::set_max_recvbuf_size(int max_size) {

	_max_msg_size = max_size;

	int buf_size;
	unsigned int opt_len = sizeof(buf_size);
	if (getsockopt(_fd, SOL_SOCKET, SO_RCVBUF, &buf_size, &opt_len) == -1) {
		error(_module.c_str(), "getsockopt recvbuf failed: '%d:%s'", errno, strerror(errno));
		return -1;
	}

	if (buf_size < _max_msg_size) {
		buf_size = _max_msg_size;
		if (setsockopt(_fd, SOL_SOCKET, SO_RCVBUF, &buf_size, opt_len) < 0) {
			error(_module.c_str(), "setsockopt recvbuf failed: '%d:%s'", errno, strerror(errno));
			return -1;
		}
	}
	info(_module.c_str(), "setsockopt recvbuf: %d", buf_size);
	return 0;
}

/**
 * get addr info
 */
std::string NetSocket::get_host(const std::string& host, unsigned int attempts) {
	std::string ip = "";

	if (host.empty()) {
		return ip;
	}

	struct addrinfo *result;
	for (auto i = 0; i <= attempts; i++) {
		if (getaddrinfo(host.c_str(), nullptr, nullptr, &result) != 0) {
			sleep(i++);
			continue;
		}

		ip = sockaddr_to_str(result->ai_addr);

		freeaddrinfo(result);
		break;
	}

	return ip;
}

/**
 * Receive a TCP packet (from an open socket)
 */
int NetSocket::recv_tcp(char* buf, int buf_len) {
	int recv_b = recv(_fd, buf, buf_len - 1, 0);
	if (recv_b > 0) {
		buf[recv_b] = '\0';
		return 0;
	}
	return -1;
}

/**
 * receives a message from a connected udp socket
 */
int NetSocket::recv_udp(char* buf, int buf_len) {
	int recv_b = recv(_fd, buf, buf_len, 0);
	if (recv_b < 0) {
		return 0;
	}
	buf[recv_b] = '\0';

	return recv_b;
}

/* send a udp packet */
int NetSocket::send_udp(const std::string& msg) {
	unsigned int i = 0;
	/* Maximum attempts is 5 */
	while (send(_fd, msg.c_str(), msg.length(), 0) < 0) {
		if ((errno != ENOBUFS) || (i >= 5)) {
			return Net::ERR_SOCKERR;
		}

		i++;
		error(_module.c_str(), "Remote socket busy, waiting %ds", i);
		sleep(i);
	}

	return 0;
}

/* send a tcp packet (through an open socket) */
int NetSocket::send_by_tcp(const std::string& msg) {
	if (send(_fd, msg.c_str(), msg.length() , 0) <= 0) {
		return Net::ERR_SOCKERR;
	}

	return 0;
}

int NetSocket::send_tcp_with_size(const std::string& msg, int size) {
	if (send(_fd, msg.c_str(), msg.length(), 0) < size) {
		return Net::ERR_SOCKERR;
	}

	return 0;
}

/**
 * convert a sockaddr to a printable address
 */
std::string NetSocket::sockaddr_to_str(struct sockaddr *src_addr) {
	sa_family_t af;
	struct sockaddr_in *sa4;
	struct sockaddr_in6 *sa6;

	char buf[256] = {'\0'};
#ifdef WIN
	int new_length = sizeof(buf);
#endif
	af = src_addr->sa_family;
	switch (af) {
		case AF_INET:
			sa4 = (struct sockaddr_in*)src_addr;
#ifdef WIN
			WSAAddressToString((LPSOCKADDR) sa4, sizeof(struct sockaddr_in), NULL, buf, (LPDWORD) &newlength);
#else
			inet_ntop(af, (const void *)&(sa4->sin_addr), buf, sizeof(buf));
#endif
			break;
		case AF_INET6:
			sa6 = (struct sockaddr_in6 *)src_addr;
#ifdef WIN3
			WSAAddressToString((LPSOCKADDR) sa6, sizeof(struct sockaddr_in6), NULL, buf, (LPDWORD) &newlength);
#else
			inet_ntop(af, (const void *) &(sa6->sin6_addr), buf, sizeof(buf));
#endif
        	if (IN6_IS_ADDR_V4MAPPED(&(sa6->sin6_addr))) {  /* extract the embedded IPv4 address */
				memmove(buf, buf+7, sizeof(buf) - 7);
			}
        	break;
		default:
			buf[0] = '\0';
	}

	return std::string(buf);
}

int Epoll::create_epoll() {
	int fd = -1;
#ifdef Darwin
	/* mac */
	fd  = kqueue();
#else
	/* linux */
	fd = epoll_create(FD_SIZE);
#endif
	if (fd < 0) {
		return -1;
	}
	_ep_fd = fd;
	return fd;
}

int Epoll::add_event(int fd, int events) {
	int ret = 0;
#ifdef Darwin
	/* kqueue中，对于event需要分开注册 */
	struct kevent ev[2];					/* 仅考虑：EVFILT_READ和EVFILT_WRITE事件 */
	int n = 0;
	if (events & EVFILT_READ) {
		EV_SET(&ev[n++], fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, (void *)(intptr_t)fd);
	}
	if (events & EVFILT_WRITE) {
		EV_SET(&ev[n++], fd, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, (void *)(intptr_t)fd);
	}
	ret = kevent(_ep_fd, ev, n, nullptr, 0, nullptr);
#else
	/* todo: linux */
#endif
	if (ret != 0) {
		return -1;
	}
	return 0;
}

int Epoll::delete_event(int fd, int events) {
	int ret = 0;
#ifdef Darwin
	/* kqueue中，对于event需要分开注册 */
	struct kevent ev[2];					/* 仅考虑：EVFILT_READ和EVFILT_WRITE事件 */
	int n = 0;
	if (events & EVFILT_READ) {
		EV_SET(&ev[n++], fd, EVFILT_READ, EV_DELETE, 0, 0, (void *)(intptr_t)fd);
	}
	if (events & EVFILT_WRITE) {
		EV_SET(&ev[n++], fd, EVFILT_WRITE, EV_DELETE, 0, 0, (void *)(intptr_t)fd);
	}
	ret = kevent(_ep_fd, ev, n, nullptr, 0, nullptr);
#else
	/* todo: linux */
#endif
	if (ret != 0) {
		return -1;
	}
	return 0;
}

int Epoll::modify_event(int fd, int events) {
#ifdef Darwin
	/* nothing to do */
#else
	// todo:
#endif
	return 0;
}

}

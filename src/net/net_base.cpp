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
		close(_fd);
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

}

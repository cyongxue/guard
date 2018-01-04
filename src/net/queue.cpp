/*
 *  queue.cpp, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年6月3日
 *      Author: yongxue@cyongxue@163.com
 */

#include "queue.h"

namespace net {

/**
 * unix domain client connect server
 */
int UnixDomainQueueIn::connect_server(const int max_msg) {

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

int UnixDomainQueueOut::bind_server(const int max_msg) {
	unlink(_path.c_str());

	_fd = socket(PF_UNIX, SOCK_DGRAM, 0);
	if (_fd < 0) {
		error(_module.c_str(), "create server socket failed: '%d: %s'", errno, strerror(errno));
		return -1;
	}

	if (bind(_fd, (struct sockaddr *)&_unix_addr, SUN_LEN(&_unix_addr)) < 0) {
		error(_module.c_str(), "bind server socket failed: '%d: %s'", errno, strerror(errno));
		close_fd();
		return -1;
	}

	if (chmod(_path.c_str(), _mode) < 0) {
		error(_module.c_str(), "chmod '%s' failed: '%d: %s'", _path.c_str(), errno, strerror(errno));
		close_fd();
		return -1;
	}

	if (set_max_recvbuf_size(max_msg) < 0) {
		error(_module.c_str(), "set_max_recvbuf_size failed.");
		close_fd();
		return -1;
	}

	return 0;
}

}

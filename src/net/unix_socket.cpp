/*
 *  unix_socket.cpp, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年6月3日
 *      Author: yongxue@cyongxue@163.com
 */

#include "unix_socket.h"

namespace net {

/**
 * unix domain client connect server
 */
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

///**
// * unix domain client send msg，send a message to the queue
// * input params:
// * 		mq_msg: 消息位置
// * 		mq_type: 消息类型
// */
//int UnixDomainClient::send_msg(const std::string& msg, const std::string& mq_msg, char mq_type) {
//	/* check for global locks */
//	auto lock = LockWithFile::instance();
//	lock->wait();
//
//	if (mq_type == MQ_SECURE) {			// secure模式
//
//	}
//	else {
//
//	}
//
//	/* queue not available */
//	if (_fd < 0) {
//		error(NET, "unix queue fd < 0");
//		return -1;
//	}
//
//	/**
//	 * we attempt 5 times to send the message if the receiver socket is busy.
//	 * after the first error, we wait 1 second;
//	 * after the second error, we wait 3 seconds;
//	 * after the third error, we wait 5 seconds;
//	 * after the fourth error, we wait 10 seconds;
//	 * if we failed again, the message is not going to be delivered and an error is sent back.
//	 */
//	// todo: send msg to unix queue
//
//	return 0;
//}

//std::shared_ptr<UnixDomainClient> write_msg_queue(const std::string& queue_path) {
//	if (util::file_mod_time(queue_path) < 0) {
//		sleep(1);
//		if (util::file_mod_time(queue_path) < 0) {
//			sleep(5);
//			if (util::file_mod_time(queue_path) < 0) {
//				sleep(15);
//				if (util::file_mod_time(queue_path) < 0) {
//					error(NET, "Queue '%s' not accessible: '%s'.", queue_path.c_str(), "Queue not found.");
//					return nullptr;
//				}
//			}
//		}
//	}
//
//	auto queue = std::make_shared<net::UnixDomainClient>(queue_path);
//	if (queue == nullptr) {
//		return nullptr;
//	}
//	if (queue->connect_server(OS_SIZE_6144 + 256) < 0) {
//		sleep(1);
//		if (queue->connect_server(OS_SIZE_6144 + 256) < 0) {
//			sleep(2);
//			if (queue->connect_server(OS_SIZE_6144 + 256) < 0) {
//				error(NET, "Queue '%s' not accessible.", queue_path.c_str());
//				return nullptr;
//			}
//		}
//	}
//
//	debug(NET, "(unix_domain) Maximum send buffer set to: '%d'.", queue->get_sendbuf_size());
//	return queue;
//}

int UnixDomainServer::bind_server(const int max_msg) {
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

int UnixDomainServer::to_listen() {
	if (listen(_fd, 1024) < 0) {
		error(_module.c_str(), "listen socket Failed.");
		close_fd();
		return -1;
	}

	return 0;
}

std::shared_ptr<UnixDomainServer::UnixDomainRemoteClient> UnixDomainServer::to_accept() {
	auto cli_info = std::make_shared<UnixDomainServer::UnixDomainRemoteClient>();

	struct sockaddr_un tmp_un;
	int un_len = sizeof(tmp_un);
	int client_fd = accept(_fd, (struct sockaddr *)&(tmp_un), (unsigned int*)&un_len);
	if (client_fd < 0) {
		return nullptr;
	}

	cli_info->set_client_fd(client_fd);
	cli_info->set_client_add(tmp_un);

	return cli_info;
}

/**
 * 支持：1. 内部loop循环；2. 内部单次有外部循环
 */
int UnixDomainServer::run_loop(int wait_ms, bool is_forever) {
	for (;;) {
		auto ret = loop_once(wait_ms);
		if (ret < 0) {
			// todo: loop once error
		}

		/* continue */
		if (is_forever == false) {
			break;
		}
	}

	return 0;
}

/**
 * 单次循环
 */
int UnixDomainServer::loop_once(int wait_ms) {
	auto ep = epoll();
#ifdef Darwin
	struct timespec timeout;
	timeout.tv_sec = wait_ms / 1000;
	timeout.tv_nsec = (wait_ms % 1000) * 1000 * 1000;

	struct kevent active_evs[Epoll::EV_LIST_NUMBER];
	int ret = kevent(ep->ep_fd(), nullptr, 0, active_evs, Epoll::EV_LIST_NUMBER, &timeout);
	if (ret < 0) {
		return -1;
	}
	for (int i = 0; i < ret; i++) {
		if (active_evs[i].flags & EV_EOF) {
			// todo: 该fd出错处理
		}
		else if (active_evs[i].ident == _fd && active_evs[i].filter == EVFILT_READ) {
			// todo: accept, 建立新的连接
		}
		else if (active_evs[i].filter == EVFILT_READ) {
			// todo: read msg
		}
		else if (active_evs[i].filter == EVFILT_WRITE) {
			// todo: write msg
		}
		else {
			return -1;
		}
	}
#else
	/* linux */
	struct epoll_event events[Epoll::EPOLL_EVENT];
	int ret = epoll_wait(ep->ep_fd(), events, Epoll::EPOLL_EVENT, wait_ms);
	if (ret < 0) {
		return -1;
	}
	for (auto i = 0; i < ret; i++) {
		/* 暂时仅考虑in、out的事件 */
		if (events[i].data.fd == _fd) {
			// todo: accept
		}
		else if (events[i].events & EPOLLIN) {
			// todo: read msg
		}
		else if (events[i].events & EPOLLOUT) {
			// todo: send msg
		}
		else {
			// todo:
		}
	}
#endif
	return 0;
}

//std::shared_ptr<UnixDomainServer> read_msg_queue(const std::string& queue_path) {
//	auto queue = std::make_shared<UnixDomainServer>(queue_path, 0660);
//	if (queue == nullptr) {
//		return nullptr;
//	}
//
//	if (queue->bind_server(OS_SIZE_6144 + 512) < 0) {
//		error(NET, "UnixDomainServer bind server error.");
//		return nullptr;
//	}
//
//	return queue;
//}

}



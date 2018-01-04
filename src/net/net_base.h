/*
 *  net_base.h, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年3月21日
 *      Author: yongxue@cyongxue@163.com
 */

#ifndef SRC_NET_NET_BASE_H_
#define SRC_NET_NET_BASE_H_

#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "defs.h"
#include "log.h"

#ifdef Darwin
#include <sys/event.h>
#else
#include <sys/epoll.h>
#endif

namespace net {

#define NET		"Net"
#define OS_SIZE_6144    6144

/* Default queues */
#define MQ_LOCALFILE	'1'
#define MQ_SYSLOG		'2'
#define MQ_HOSTINFO		'3'
#define MQ_SECURE		'4'
#define MQ_SYSCHECK		'8'
#define MQ_ROOTCHECK	'9'

/**
 * socket交互基础类
 */
class Net {
	protected:
		int 		_fd;

		std::string	_module;

	public:
		const int ERR_SOCKBUSY = -2;
		const int ERR_SOCKERR = -3;

	public:
		Net(): _fd(-1), _module(NET) {
			info(_module.c_str(), "Net struct ...");
		};
		virtual ~Net() {
			info(_module.c_str(), "Net destruct ...");
			if (_fd != -1) {
#ifdef WIN
				closesocket(_fd);
#else
				close(_fd);
#endif	/* WIN */
			}
		};

		int get_sendbuf_size();
		int get_recvbuf_size();

	protected:
		void close_fd();
};

/**
 * unix domain(unix域套接字)方式
 */
class UnixDomain: public Net {
	protected:
		std::string				_path;
		struct sockaddr_un		_unix_addr;

		int						_max_msg_size;

	public:
		UnixDomain(): _max_msg_size(0) {};
		UnixDomain(const std::string& path): _path(path), _max_msg_size(0) {
			info(_module.c_str(), "UnixDomain struct ...");

			memset(&_unix_addr, 0, sizeof(_unix_addr));
			_unix_addr.sun_family = AF_UNIX;
			strncpy(_unix_addr.sun_path, path.c_str(), sizeof(_unix_addr.sun_path) - 1);
		}
		virtual ~UnixDomain() {
			info(_module.c_str(), "UnixDomain destruct ...");
		}

	public:
		/* send a message using a unix socket. */
		virtual int send_unix_msg(const std::string& msg);
		/* receive a message from a unix socket */
		virtual int recv_unix_msg(char *buf, int buf_len);

	protected:
		/* 设置option buf size */
		int set_max_sendbuf_size(int max_size);
		int set_max_recvbuf_size(int max_size);
};

/**
 * net socket方式
 */
class NetSocket: public Net {
	protected:
		std::string 		_ip;
		std::string			_port;
		unsigned int 		_proto;

		struct addrinfo		_addr;

	public:
		NetSocket(const std::string& ip, unsigned int port, unsigned int proto):
			_ip(ip), _port(std::to_string(port)), _proto(proto) {}
		virtual ~NetSocket() {};

		/* get addrinfo by host */
		std::string get_host(const std::string& host, unsigned int attempts);
		/* convert a socketaddr to a printable address */
		std::string sockaddr_to_str(struct sockaddr *src_addr);

		/* udp */
		/* receives a message from a connected udp socket */
		int recv_udp(char* buf, int buf_len);
		/* send a udp packet */
		int send_udp(const std::string& msg);

		/* tcp */
		/* receive a tcp packet (from an open socket) */
		int recv_tcp(char* buf, int buf_len);
		/* send a tcp packet (through an open socket) */
		int send_by_tcp(const std::string& msg);
		int send_tcp_with_size(const std::string& msg, int size);
};

class Epoll {
	private:
		int 	_ep_fd;

	public:
#ifdef Darwin
		static const int EV_LIST_NUMBER = 32;
#else
		static const int FD_SIZE = 1024;
		static const int EPOLL_EVENT = 32;
#endif
		/* 参考：1. epoll：http://man7.org/linux/man-pages/man2/epoll_ctl.2.html
		 * 2. http://eradman.com/posts/kqueue-tcp.html
		 * */
		const int EV_READ_IN = 0x0001;
		const int EV_WRITE_OUT = 0x0002;
		const int EV_EPOLL_RDHUP = 0x0004;
		const int EV_EPOLL_ERR = 0x0010;
		const int EV_EPOLL_HUP = 0x0020;

	public:
		Epoll(): _ep_fd(-1) {

		}

		int create_epoll();
		int ep_fd() const { return _ep_fd; }

		int add_event(int fd, int events);
		int delete_event(int fd, int events);
		int modify_event(int fd, int events);
};

/**
 * net client的基类
 */
class Client {
	public:
		virtual ~Client() = default;

		virtual int connect_server(const int max_msg) = 0;
};

/**
 * remote client infomation
 */
class RemoteClient {
	private:
		std::string 				_ip;
		int							_fd;
		struct sockaddr_storage 	_addr;

	public:
		RemoteClient(const std::string& ip, int fd, const struct sockaddr_storage& addr):_ip(ip), _fd(fd), _addr(addr) {};
};

/**
 * base class, server
 */
class Server {
	public:
		virtual ~Server() = default;
};

}

#endif /* SRC_NET_NET_BASE_H_ */

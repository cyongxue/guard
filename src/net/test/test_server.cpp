/*
 *  test_server.cpp, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年6月2日
 *      Author: yongxue@cyongxue@163.com
 */

#include <iostream>

#include "unix_socket.h"
#include "queue.h"

using namespace std;

int queue_read();

int main() {
	/* start tcp server */

	/* start udp server */

	/* start unix domain server */

	/* start queue read */
	(void)queue_read();

	return 0;
}

int queue_read() {
	auto out = net::UnixDomainQueueOut("./unix_queue", 0660);
	out.bind_server(1024);

	while (1) {
		char buf[1024] = {'\0'};
		auto ret = out.recv_unix_msg(buf, sizeof(buf));
		if (ret > 0) {
			std::cout << buf << std::endl;
		}
		else {
			std::cout << "recv failed." << std::endl;
		}
	}

	return 0;
}

int unix_domain_server() {
	auto unix_domain_server = net::UnixDomainServer("./unix_queue", 0660);
	unix_domain_server.bind_server(10240);

	char buf[1024] = {'\0'};
	auto ret = unix_domain_server.recv_unix_msg(buf, sizeof(buf));
	if (ret > 0) {
		std::cout << buf << std::endl;
		if (unix_domain_server.send_unix_msg(buf) != 0) {
			std::cout << "send failed." << std::endl;
		}
	}
	else {
		std::cout << "recv failed." << std::endl;
	}

	while (1) {
		sleep(5);
	}

	return 0;
}


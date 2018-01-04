/*
 *  test_client.cpp, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年6月2日
 *      Author: yongxue@cyongxue@163.com
 */
#include <iostream>

#include "unix_socket.h"
#include "queue.h"

using namespace std;

int queue_write();

int main() {
	/* start tcp client */

	/* start udp client */

	/* start unix domain client */

	/* start queue write */
	(void)queue_write();

	return 0;
}

int queue_write() {
	auto in = net::UnixDomainQueueIn("./unix_queue");
	if (in.connect_server(1024) == 0) {
		for (auto i = 0; i < 10; i++) {
			std::string msg = std::to_string(i) + ":1234567890";
			std::cout << msg << std::endl;
			if (in.send_unix_msg(msg) != 0) {
				std::cout << "send failed." << std::endl;
			}
//			sleep(1);
		}
	}
	else {
		std::cout << "connect server failed." << std::endl;
	}

	return 0;
}

int unix_domain_client() {
	auto unix_domain_client = net::UnixDomainClient("./unix_queue");
	if (unix_domain_client.connect_server(1024) == 0) {
		unix_domain_client.send_unix_msg("xkxkxkkxkxkxkxkk");

		char buf[1024] = {'\0'};
		unix_domain_client.recv_unix_msg(buf, sizeof(buf));
		std::cout << buf << std::endl;
	}
	else {
		std::cout << "connect server failed." << std::endl;
	}

	return 0;
}

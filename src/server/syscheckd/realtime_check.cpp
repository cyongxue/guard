/*
 *  realtime_check.cpp, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年3月30日
 *      Author: yongxue@cyongxue@163.com
 */

#include "log.h"
#include "realtime_check.h"

/* linux环境支持inotify情况 */
#ifdef INOTIFY_ENABLED

#include <sys/inotify.h>

/* 析构函数，涉及到资源的释放
 * */
RtFim::~RtFim() {
	if (_fd != -1) {

		// 先移除watch
		for (auto one: _dir_tb) {
			inotify_rm_watch(_fd, one.first);
		}

		// 关闭监控
		close(_fd);
		_fd = -1;
	}
}

int RtFim::start() {
	debug(_module.c_str(), "Initializing real time file monitoring...");

	/* 创建inotify实例，得到一个fd值 */
	_fd = inotify_init();
	if (_fd < 0) {
		error(_module.c_str(), "Unable to initialize inotify.");
		return -1;
	}

	return 0;
}

/**
 * 注意：仅监控目录dir，如此：后面process才有dir + "/" + event->name的操作方式
 * 过滤nfs放在上层业务中
 */
int RtFim::add_dir(const std::string& dir) {
	if (_fd < 0) {
		if (start() == -1) {
			return -1;
		}
	}

	int wd = inotify_add_watch(_fd, dir.c_str(), REALTIME_MONITOR_FLAGS);
	if (wd < 0) {
		error(_module.c_str(), "Unable to add directory to real time monitoring: '%s', %d", dir.c_str(), wd);
	}
	else {
		auto result = _dir_tb.find(wd);
		if (result == _dir_tb.end()) {
			_dir_tb.insert(std::make_pair(wd, dir));
			info(_module.c_str(), "Directory added for real time monitoring: '%s'..", dir.c_str());
		}
	}
	return 0;
}

int RtFim::process() {
	char buf[REALTIME_EVENT_BUFFER + 1];
	buf[REALTIME_EVENT_BUFFER] = '\0';

	auto len = read(_fd, buf, REALTIME_EVENT_BUFFER);
	if (len < 0) {
		error(_module.c_str(), "Unable to read from real time buffer.");
	}
	else if (len > 0) {
		int i = 0;
		struct inotify_event * event;
		while (i < len) {
			event = (struct inotify_event *)(void *)&buf[i];
			if (event->len) {
				auto result = _dir_tb.find(event->wd);
				if (result != _dir_tb.end()) {
					std::string file_name = result->second + "/" + event->name;
					/* Need a sleep here to avoid triggering on vim edits */
					sleep(1);
					debug(_module.c_str(), "File has change '%s'", file_name.c_str());
					(void)_file_db->realtime_checksum_file(file_name);
				}
				else {
					error(_module.c_str(), "No find the dir for '%s'", event->name);
				}
			}

			i += REALTIME_EVENT_SIZE + event->len;
		}
	}

	return 0;
}

/* windows环境的支持 */
#elif defined(WIN)

/* 非windows和非inotify支持 */
#else

#endif



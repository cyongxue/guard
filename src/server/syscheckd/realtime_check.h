/*
 *  realtime_check.h, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年3月30日
 *      Author: yongxue@cyongxue@163.com
 */

#ifndef SRC_SERVER_SYSCHECKD_REALTIME_CHECK_H_
#define SRC_SERVER_SYSCHECKD_REALTIME_CHECK_H_

// test 使用
#define INOTIFY_ENABLED

/* linux环境支持inotify情况
 * 检查系统是否支持inotify： grep INOTIFY_USER /boot/config-$(uname -r)
 * 				     或者： 查看/proc/sys/fs/inotify目录
 * */
#ifdef INOTIFY_ENABLED

#include <sys/inotify.h>

#define REALTIME_MONITOR_FLAGS  IN_MODIFY|IN_ATTRIB|IN_MOVED_FROM|IN_MOVED_TO|IN_CREATE|IN_DELETE|IN_DELETE_SELF
#define REALTIME_EVENT_SIZE     (sizeof (struct inotify_event))
#define REALTIME_EVENT_BUFFER   (2048 * (REALTIME_EVENT_SIZE + 16))

/* windows环境的支持 */
#elif defined(WIN)

/* 非windows和非inotify支持 */
#else

#endif

/*
 * 用于文件实时监控；关注linux平台的inotify支持和windows系统的支持
 * */
class RtFim {
	private:
		std::string	_module;
		std::shared_ptr<FileDb>				_file_db;

		int _fd;
#ifdef WIN
		// todo:
#endif
		std::unordered_map<int, std::string> _dir_tb;

	public:
		RtFim(const std::shared_ptr<FileDb>& file_db) {
			_file_db = file_db;
			_module = _file_db->serv_name();

			_fd = -1;
#ifdef WIN
			// todo:
#endif
		}

		~RtFim();

		int fd() const { return _fd; }
		const std::unordered_map<std::string, std::string>& dir_tb() const { return _dir_tb; }

		int start();
		int add_dir(const std::string& dir);
		int process();
};

#endif /* SRC_SERVER_SYSCHECKD_REALTIME_CHECK_H_ */

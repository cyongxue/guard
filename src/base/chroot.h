/*
 *  util.h, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年3月2日
 *      Author: yongxue@cyongxue@163.com
 */

#ifndef SRC_BASE_UTIL_H_
#define SRC_BASE_UTIL_H_

class Chroot {
	private:
		bool 			_is_chroot;

		std::string		_server_name;
		std::string		_base_dir;

		static Chroot*  _instance;

	private:
		Chroot(const std::string& server_name, const std::string& base_dir):
			_server_name(server_name), _base_dir(base_dir) {
			_is_chroot = false;
		}

	public:
		static Chroot* instance(const std::string& server_name = "",
				const std::string& base_dir = "");

		std::string full_file_path(const std::string& file_name);

		int do_chroot();
};


#endif /* SRC_BASE_UTIL_H_ */

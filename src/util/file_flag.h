/*
 *  file_flag.h, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年3月21日
 *      Author: yongxue@cyongxue@163.com
 */

#ifndef SRC_UTIL_FILE_FLAG_H_
#define SRC_UTIL_FILE_FLAG_H_

namespace util {

class FileCheckOpt {
	public:
		enum FileOpt {
			CHECK_MD5SUM  		= 0000001,
			CHECK_PERM			= 0000002,
			CHECK_SIZE			= 0000004,
			CHECK_OWNER			= 0000010,
			CHECK_GROUP			= 0000020,
			CHECK_SHA1SUM		= 0000040,
			CHECK_REALTIME		= 0000100,
			CHECK_SEECHANGES 	= 0000200,
		};

	private:
		uint32_t	_opts;

	public:
		void add_opt(const uint32_t opts) {
			_opts |= opts;
		}
		void del_opt(const uint32_t opts) {
			_opts &= ~ opts;
		}

		bool is_opt(FileOpt opt) const {
			return (_opts & opt);
		}

		uint32_t opts() const { return _opts; }
		std::string opts_str() const;

		static std::string get_str_type(int type);
};

/**
 * 文件系统的类型
 */
struct file_system_type {
	const char* 		_name;
	const uint32_t		_type;
	const int			_flag;
	file_system_type(const char* name, const uint32_t type, const int flag):
		_name(name), _type(type), _flag(flag) {}
};

int is_nfs(const std::string& file_name);
int skip_fs(const std::string& file_name);

}

#endif /* SRC_UTIL_FILE_FLAG_H_ */

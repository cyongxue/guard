/*
 *  log.cpp, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年3月2日
 *      Author: yongxue@cyongxue@163.com
 */
#include <sys/stat.h>
#include <unistd.h>

#include "log.h"

/**
 * 日志格式：
 * 2017/02/22 13:19:28 claw-remoted: INFO: [msg].[file:file_line]
 */

Log* Log::_instance = nullptr;

Log* Log::instance(std::string path, std::string name) {
	if (nullptr == _instance) {
		_instance = new Log(path, name);
	}
	return _instance;
}

int Log::is_dir(const char * file) {
	struct stat file_status;
	if (stat(file, &file_status) < 0) {
		return (-1);
	}
	if (S_ISDIR(file_status.st_mode)) {
		return (0);
	}
	return (-1);
}

int Log::open() {

	if (_is_daemon) {
		if (is_dir(_log_path.c_str()) == -1) {
			if (mkdir(_log_path.c_str(), 0770) == -1) {
				return -1;
			}
		}

		time_t tm = time(nullptr);
		struct tm *p = localtime(&tm);
		char file_name[LOG_DATA_FORMAT_LEN] = {0};
		strftime(file_name, sizeof(file_name), "%Y-%m-%d", p);

		char log_file[256] = {0};
		snprintf(log_file, sizeof(log_file), "%s/%s.log", _log_path.c_str(), file_name);

		_fp = fopen(log_file, "a");
		if (_fp == nullptr) {
			return -1;
		}

		std::string link_name = _log_path + "/" + _link_name;
		unlink(link_name.c_str());

		if (link(log_file, link_name.c_str()) == -1) {
			fclose(_fp);
			return -1;
		}
	}
	return 0;
}

void Log::close() {
	fclose(_fp);
	_fp = nullptr;
	return;
}

int Log::reopen() {
	close();
	return open();
}

void Log::set_daemon(int flag) {
	_is_daemon = flag;
	return;
}

void Log::set_log_level(LogLevel level) {
	_log_level = level;
	return;
}

void Log::log(LogLevel log_level, const char* module, const char *fmt, ...) {

	std::string str_log_level = log_level_str[(int)log_level];

	if (_is_daemon) {
		if (log_level > _log_level) {
			return;
		}

		int ret = 0;
		if (!_fp) {
			ret = open();
		}

		time_t tm = time(nullptr);
		struct tm *p = localtime(&tm);
		if (p->tm_mday != _curr_day) {
			// 重新创建文件，并建立link
			if (_file_lock.try_lock()) {
				ret = reopen();
				_file_lock.unlock();
			}
			else {
				return;
			}
		}

		if (0 == ret) {
			(void)fprintf(_fp, "%d/%02d/%02d %02d:%02d:%02d ",
							  p->tm_year + 1900, p->tm_mon + 1,
							  p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);
			(void)fprintf(_fp, "%s: %s: ", module, str_log_level.c_str());

			// 打印内容
			va_list args;
			va_start(args, fmt);
			(void)vfprintf(_fp, fmt, args);
			va_end(args);

			fprintf(_fp, ".[%s:%d]", __FILE__, __LINE__);
			fprintf(_fp, "\n");

			fflush(_fp);
		}
	}
	else {
		time_t tm = time(nullptr);
		struct tm *p = localtime(&tm);

		(void)fprintf(stderr, "%d/%02d/%02d %02d:%02d:%02d ",
							  p->tm_year + 1900, p->tm_mon + 1,
							  p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);
		(void)fprintf(stderr, "%s: %s: ", module, str_log_level.c_str());

		// 打印内容
		va_list args;
		va_start(args, fmt);
		(void)vfprintf(stderr, fmt, args);
		va_end(args);

		fprintf(stderr, ".[%s:%d]", __FILE__, __LINE__);
		fprintf(stderr, "\n");
	}

	return;
}

//#define LOG_TEST
#ifdef LOG_TEST
int main() {
//	Log::instance()->set_daemon(1);
////	Log::instance()->set_log_level(LogLevel::INFO);
////	Log::instance()->log(LogLevel::DEBUG, "Test", "%s", "hello world");
////	Log::instance()->log(LogLevel::INFO, "Daemon", "%s", "==========Daemon Daemon");
	debug("Test", "%s", "hello world");
//	fatal("Fatal", "%s", "hello world");
	info("Daemon", "%s", "==========Daemon Daemon");
	return 0;
}
#endif


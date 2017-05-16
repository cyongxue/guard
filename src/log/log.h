/*
 *  log.h, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年3月2日
 *      Author: yongxue@cyongxue@163.com
 */

#ifndef SRC_LOG_LOG_H_
#define SRC_LOG_LOG_H_

#include <stdio.h>
#include <stdlib.h>
#include <string>

#include <mutex>

#define LOG_DATA_FORMAT_LEN				sizeof("2015-02-03")
#define LOG_DATA_FORMAT_SIZE			(LOGDATE_FORMAT_LEN + 1)

enum class LogLevel:uint8_t {
	NO = 0,
	FATAL,
	ERROR,
	WARN,
	INFO,
	DEBUG,
	ALL,
};

static const char * log_level_str[] = {
		"No", "FATAL", "ERROR", "WARN", "INFO", "DEBUG", "ALL",
};

class Log {
	private:
		std::mutex	_file_lock;

		FILE *			_fp;
		std::string		_log_path;
		std::string		_link_name;

		int			_is_daemon;			// 是否是后台
		LogLevel	_log_level;
		int			_curr_day;

		static Log* _instance;

	private:
		Log(std::string path, std::string name):_log_path(path), _link_name(name) {
			_fp = nullptr;
			_log_level = LogLevel::DEBUG;

			_is_daemon = 0;

			time_t tm = time(nullptr);
			struct tm *p = localtime(&tm);
			_curr_day = p->tm_mday;
		};

		int is_dir(const char * file);

		int reopen();
		int open();
		void close();

	public:
		~Log() {
			fclose(_fp);
		};

		void set_daemon(int flag);
		void set_log_level(LogLevel level);

		/* 对外提供日志打印处理 */
		void log(LogLevel log_level, const std::string module, const std::string file_name, int file_line, const char *fmt, ...);

		static Log* instance(std::string path = "./logs", std::string name = "guard.log");
};

//#define PRINT_TEST
#ifndef PRINT_TEST
// debug级别日志
#define debug(module, fmt, args...)		Log::instance()->log(LogLevel::DEBUG, module, __FILE__, __LINE__, fmt, ##args)
// info级别日志
#define info(module, fmt, args...)		Log::instance()->log(LogLevel::INFO, module, __FILE__, __LINE__, fmt, ##args)
// warn级别日志
#define warn(module, fmt, args...)		Log::instance()->log(LogLevel::WARN, module, __FILE__, __LINE__, fmt, ##args)
// err级别日志
#define error(module, fmt, args...)		Log::instance()->log(LogLevel::ERROR, module, __FILE__, __LINE__, fmt, ##args)
// fatal级别日志，并exit(1)退出进程
#define fatal(module, fmt, args...)		{Log::instance()->log(LogLevel::FATAL, module, __FILE__, __LINE__, fmt, ##args); exit(1);}
#else
#define debug(module, fmt, args...)		printf(fmt, ##args)
#define info(module, fmt, args...)		printf(fmt, ##args)
#define warn(module, fmt, args...)		printf(fmt, ##args)
#define error(module, fmt, args...)		printf(fmt, ##args)
#define fatal(module, fmt, args...)		printf(fmt, ##args)
#endif

#endif /* SRC_LOG_LOG_H_ */

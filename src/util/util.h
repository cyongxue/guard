/*
 *  util.h, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年3月15日
 *      Author: yongxue@cyongxue@163.com
 */

#ifndef SRC_UTIL_UTIL_H_
#define SRC_UTIL_UTIL_H_

#include <sstream>
#include <string>
#include <vector>

#include <regex.h>
#include <sys/stat.h>

namespace util {

#define	INET6_ADDRSTRLEN	46
#define IPSIZE          INET6_ADDRSTRLEN /* IP Address size             */

/* 根据指定delim分割string */
std::vector<std::string> split_str(const std::string& src, const char delim);
/* 移除string的head和tail的指定flag字符 */
std::string rm_end_head_char(const std::string& src, const char flag);
/* 判断指定string是否为数字 */
bool is_number(const std::string& src);
/* parse boolean */
int eval_bool(const std::string& src);

/* 获取文件的modify时间，也可以判断文件存在性 */
time_t file_mod_time(const std::string& file);

/* 获取os的uname */
std::string get_uname();

/* satop(struct sockaddr *sa, char *dst, socklen_t size)
 * Convert a sockaddr to a printable address.
 */
int sockaddr_to_str(struct sockaddr *sa);

bool is_dir(const char * file);
bool is_dir(const std::string& file);

/*
 * check if the file is present using several methods to void being tricked by syscall hiding
 * */
bool is_file(const std::string& name);
bool is_file_on_dir(const std::string& file_name, const std::string& dir_name);

/* 移除首尾的space和tab */
char* rm_end_head_space_tab(char * str);

/* compile a POSIX regex
 */
bool posix_regex(const char * str, const char * regex);
}

#endif /* SRC_UTIL_UTIL_H_ */

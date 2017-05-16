/*
 *  util.cpp, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年3月15日
 *      Author: yongxue@cyongxue@163.com
 */

#include <algorithm>
#include <cctype>
#include <sys/utsname.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "defs.h"
#include "util.h"

namespace util {

std::vector<std::string> split_str(const std::string& src, const char delim) {

	std::vector<std::string> dest;

	std::string token;
	std::istringstream ss(src);

	while (std::getline(ss, token, delim)) {
		dest.push_back(token);
	}

	return dest;
}

std::string rm_end_head_char(const std::string& src, const char flag) {

	size_t len = 0;

	auto begin = src.find_first_not_of(flag);
	if (begin == std::string::npos) {
		begin = 0;
	}

	auto end = src.find_last_not_of(flag);
	if (end == std::string::npos) {
		len = src.length() - begin;
	}
	else {
		len = src.length() - (src.length() - end) - begin + 1;
	}

	auto dest = src.substr(begin, len);

	return dest;
}

bool is_number(const std::string& src) {
	return std::all_of(src.begin(), src.end(), [] (char c) { return (std::isdigit((int)c) != 0); });
}

int eval_bool(const std::string& src) {
	if (src.empty()) {
		return -1;
	}

	if (src == "yes") {
		return 1;
	}
	else if (src == "no") {
		return 0;
	}
	else {
		return -1;
	}
}


time_t file_mod_time(const std::string& file) {
    struct stat file_status;

    if (stat(file.c_str(), &file_status) < 0) {
        return (-1);
    }

    return (file_status.st_mtime);				// 得到文件修改时间
}

std::string get_uname() {
	struct utsname uts_buf;
	std::string ret;

	if (uname(&uts_buf) >= 0) {
		char tmp_buf[256];
		memset(tmp_buf, 0, sizeof(tmp_buf));
		snprintf(tmp_buf, 255, "%s %s %s %s %s - %s %s",
					 uts_buf.sysname,
					 uts_buf.nodename,
					 uts_buf.release,
					 uts_buf.version,
					 uts_buf.machine,
					 __guard_name, __version);
		ret = tmp_buf;
	}
	else {
		char tmp_buf[256];
		snprintf(tmp_buf, 255, "No system info available -  %s %s",
				__guard_name, __version);
		ret = tmp_buf;
	}
	return ret;
}

std::string sockaddr_to_str(struct sockaddr *sa) {
	std::string ret;
	char tmp[IPSIZE];
	memset(tmp, 0, sizeof(tmp));

	sa_family_t af;
	struct sockaddr_in *sa4;
	struct sockaddr_in6 *sa6;
#ifdef WIN
	int newlength;
#endif

	af = sa->sa_family;

	switch (af)
	{
	case AF_INET:
		sa4 = (struct sockaddr_in *) sa;
#ifdef WIN32
		newlength = size;
		WSAAddressToString((LPSOCKADDR) sa4, sizeof(struct sockaddr_in),
						   NULL, dst, (LPDWORD) &newlength);
#else
		inet_ntop(af, (const void *) &(sa4->sin_addr), tmp, sizeof(tmp));
#endif
		ret = tmp;
		return ret;
	case AF_INET6:
		sa6 = (struct sockaddr_in6 *) sa;
#ifdef WIN32
		newlength = size;
		WSAAddressToString((LPSOCKADDR) sa6, sizeof(struct sockaddr_in6),
						   NULL, dst, (LPDWORD) &newlength);
#else
		inet_ntop(af, (const void *) &(sa6->sin6_addr), tmp, sizeof(tmp));
#endif
		if (IN6_IS_ADDR_V4MAPPED(&(sa6->sin6_addr)))
		{  /* extract the embedded IPv4 address */
			memmove(tmp, tmp+7, sizeof(tmp)-7);
		}
		ret = tmp;
		return ret;
	default:
		*tmp = '\0';
		return ret;
	}
}

bool is_dir(const char * file) {
	struct stat file_status;
	if (stat(file, &file_status) < 0) {
		return false;
	}
	if (S_ISDIR(file_status.st_mode)) {
		return true;
	}
	return false;
}

bool is_dir(const std::string& file) {
	struct stat file_status;
	if (stat(file.c_str(), &file_status) < 0) {
		return false;
	}
	if (S_ISDIR(file_status.st_mode)) {
		return true;
	}
	return false;
}

/**
 * check if the file is present using several methods to void being tricked by syscall hiding
 * 通过多种方式，防止文件被syscall隐藏
 */
bool is_file(const std::string& origin_name) {

	if (origin_name.empty()) {
		return false;
	}
	char * file_name = calloc(origin_name.length() + 1, 1);
	if (file_name == nullptr) {
		return false;
	}
	strncpy(file_name, origin_name.c_str(), origin_name.length());

	int ret = 0;
#ifndef WIN
	/* 记录当前目录 */
	char curr_dir[OS_SIZE_1024];
	memset(curr_dir, 0, sizeof(curr_dir));
	if (nullptr == getcwd(curr_dir, OS_SIZE_1024 - 2)) {
		free(file_name);
		return false;
	}
	/**
	 * file_name: /xxxx					则base_name为/xxxx, 和file_name相等
	 * 	     	  /aaa/bbb/ccc/ddd		则base_name为/ddd，和file_name是不等的
	 */
	char *base_name = strrchr(file_name, '/');
	if (base_name == nullptr) {
		free(file_name);
		return false;
	}
	if (base_name != file_name) {
		*base_name = '\0';
		base_name++;
		char *dir_name = file_name;		// dir_name: "/aaa/bbb/ccc"

		if (chdir(dir_name) == 0) {
			if (chdir(base_name) == 0) {
				ret = 1;
			}
			else if (errno == ENOTDIR) {
				ret = 1;
			}

			auto dp = opendir(base_name);
			if (dp) {
				closedir(dp);
				ret = 1;
			}
			else if (errno == ENOTDIR) {
				ret = 1;
			}

			/* 回到之前保存的current dir中 */
			if (chdir(curr_dir) == -1) {
				free(file_name);
				return false;
			}
		}
		base_name--;
		*base_name = '/';
	}
	else {
		if (chdir(file_name) == 0) {
			ret = 1;
			/* 回到工作的current dir，如果回去失败，则return false */
			if (chdir(curr_dir) == -1) {
				free(file_name);
				return false;
			}
		}
		else if (errno == ENOTDIR) {
			ret = 1;
		}
	}
#else
	auto dp = opendir(file_name);
	if (dp) {
		closedir(dp);
		ret = 1;
	}
#endif
	struct stat statbuf;
	FILE *fp = NULL;
    if ( (stat(file_name, &statbuf) < 0) &&
#ifndef WIN
            (access(file_name, F_OK) < 0) &&
#endif
            ((fp = fopen(file_name, "r")) == NULL)) {
    	free(file_name);
        return (bool)ret;
    }

    free(file_name);
    if (fp) {
    	fclose(fp);
    }

    return true;
}

/**
 * check if 'file' is present on 'dir' using readdir
 */
bool is_file_on_dir(const std::string& file_name, const std::string& dir_name) {
	DIR *dp = opendir(dir_name.c_str());
	if (!dp) {
		return false;
	}

	struct dirent * entry;
	while ((entry = readdir(dp)) != nullptr) {
		if (strcmp(entry->d_name, file_name.c_str()) == 0) {
			closedir(dp);
			return true;
		}
	}

	closedir(dp);
	return 0;
}

char* rm_end_head_space_tab(char * str) {
	size_t str_sz = strlen(str);
	/* Return zero-length str as is */
	if (str_sz == 0) {
		return str;
	} else {
		str_sz--;
	}

	/* Remove trailing spaces */
	while (str[str_sz] == ' ' || str[str_sz] == '\t') {
		if (str_sz == 0) {
			break;
		}

		str[str_sz--] = '\0';
	}
	/* ignore leading spaces */
	while (*str != '\0') {
		if (*str == ' ' || *str == '\t') {
			str++;
		} else {
			break;
		}
	}

	return (str);
}

/* Compile a POSIX regex, returning NULL on error
 * Returns 1 if matches, 0 if not
 */
bool posix_regex(const char * str, const char * regex) {

	if (!str || !regex) {
		return false;
	}

	regex_t preg;
	if (regcomp(&preg, regex, REG_EXTENDED | REG_NOSUB) != 0) {
		return false;
	}

	if (regexec(&preg, str, strlen(str), nullptr, 0) != 0) {
		regfree(&preg);
		return false;
	}

	regfree(&preg);
	return true;
}


}


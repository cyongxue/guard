/*
 *  check_trojans.cpp, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年4月7日
 *      Author: yongxue@cyongxue@163.com
 */

#include "check_trojans.h"


/**
 * Get next character from wherever
 */
int Strings::getch() {
	++_offset;

	if (_head_len) {
		if (_hcnt < _head_len) {
			return (int)_buf[_hcnt++];
		}
		_head_len = 0;
	}

	if ((_read_len == -1) || (_read_len-- > 0)) {
		return fgetc(_fp);
	}

	return EOF;
}

/**
 * file为：executable file
 */
bool Strings::match() {
	if (_file_name.empty() || _regex.empty()) {
		return false;
	}

	// open file
	if (!_fp) {
		_fp = fopen(_file_name.c_str(), "r");
		if (_fp == nullptr) {
			return false;
		}
	}

	// read file head
	Exec *head = (Exec*)_buf;
	_head_len = read(fileno(_fp), head, sizeof(Exec));
	if ((_head_len == sizeof(Exec)) && (!N_BADMAG(*head))) {
		_offset = N_TXTOFF(*head);
		if (fseek(stdin, _offset, SEEK_SET) == -1) {		// 偏移到offset，为啥不是fd，而是stdin
			_read_len = -1;
		}
		else {
			_read_len = head->a_text + head->a_data;
		}
		_head_len = 0;
	}
	else if (_head_len == -1) {
		_head_len = 0;
		_read_len = -1;
	}
	else {
		_hcnt = 0;
	}

	/* Read the file and perform the regex comparison: 读取文件并正则匹配 */
	char line[OS_SIZE_1024 + 1];
	char *buf;
	int cnt, ch;
	unsigned char* tmp_ptr;
	unsigned char bfr[STR_MINLEN + 2];
	memset(bfr, 0, sizeof(bfr));
	for (cnt = 0, tmp_ptr = bfr; (ch = getch()) != EOF; ) {
		if (IS_STR(ch)) {
			if (!cnt) {
				tmp_ptr = bfr;
			}
			*tmp_ptr++ = ch;
			if (++cnt < STR_MINLEN) {
				continue;
			}

			strncpy(line, (char *)bfr, STR_MINLEN + 1);
			buf = line;
			buf += strlen(line);
			while (((ch = getch()) != EOF) && IS_STR(ch)) {
				if (cnt < OS_SIZE_1024) {
					*buf = (char)ch;
					buf++;
				}
				else {
					*buf = '\0';
					break;
				}
				cnt++;
			}

			*buf = '\0';
			if (util::posix_regex(line, _regex.c_str())) {
				// 正则匹配，则符合，return true
				return true;
			}
		}
		cnt = 0;
	}

	return false;
}


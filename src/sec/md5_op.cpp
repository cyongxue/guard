/*
 *  md5_op.cpp, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年3月24日
 *      Author: yongxue@cyongxue@163.com
 */

#include <stdio.h>
#include <string.h>

#include "md5_op.h"
#include "md5.h"

namespace sec {

/**
 * 计算一个文件的md5值
 */
int md5_file(const std::string& file_name, int mode, std::string& dest) {
	if (file_name.empty()) {
		return -1;
	}

    MD5_CTX ctx;
    unsigned char buf[1024 + 1];
    unsigned char digest[16];
    size_t n;
    buf[1024] = '\0';

    FILE *fp = fopen(file_name.c_str(), mode == SecFileType::BINARY ? "rb" : "r");
    if (!fp) {
        return (-1);
    }

    MD5Init(&ctx);
    while ((n = fread(buf, 1, sizeof(buf) - 1, fp)) > 0) {
        buf[n] = '\0';
        MD5Update(&ctx, buf, (unsigned)n);
    }

    MD5Final(digest, &ctx);

    char tmp_buf[33];
    memset(tmp_buf, 0, sizeof(tmp_buf));
    for (n = 0; n < 16; n++) {
        snprintf(tmp_buf, 3, "%02x", digest[n]);
        tmp_buf += 2;
    }

    fclose(fp);

    dest = tmp_buf;

    return (0);
}

int md5_str(const std::string& src, std::string& dest)
{
    unsigned char digest[16];

    int n;

    MD5_CTX ctx;
    MD5Init(&ctx);
    MD5Update(&ctx, (const unsigned char *)src.c_str(), src.length());
    MD5Final(digest, &ctx);

    char tmp_buf[33];
    memset(tmp_buf, 0, sizeof(tmp_buf));
    for (n = 0; n < 16; n++) {
        snprintf(tmp_buf, 3, "%02x", digest[n]);
        tmp_buf += 2;
    }

    dest = tmp_buf;

    return (0);
}

int md5_cmd(const std::string& cmd, std::string& dest) {
	if (cmd.empty()) {
		return -1;
	}

	MD5_CTX ctx;
	unsigned char buf[1024 + 1];
	unsigned char digest[16];
	size_t n;
	buf[1024] = '\0';

	FILE *fp = popen(cmd.c_str(), "r");
	if (!fp) {
		return (-1);
	}

	MD5Init(&ctx);
	while ((n = fread(buf, 1, sizeof(buf) - 1, fp)) > 0) {
		buf[n] = '\0';
		MD5Update(&ctx, buf, (unsigned)n);
	}

	MD5Final(digest, &ctx);

	char tmp_buf[33];
	memset(tmp_buf, 0, sizeof(tmp_buf));
	for (n = 0; n < 16; n++) {
		snprintf(tmp_buf, 3, "%02x", digest[n]);
		tmp_buf += 2;
	}

	fclose(fp);

	dest = tmp_buf;

	return (0);
}

}


/*
 *  sha1_op.cpp, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年3月24日
 *      Author: yongxue@cyongxue@163.com
 */

#include <stdio.h>
#include <string.h>

#include "sha1_op.h"
#include "sha_locl.h"

namespace sec {

/**
 * output： string形式的输出结果
 */
int sha1_file(const std::string& file_name, int mode, std::string& sha1) {

	if (file_name.empty()) {
		return -1;
	}

    SHA_CTX c;
    unsigned char buf[2048 + 2];
    unsigned char md[SHA_DIGEST_LENGTH];
    size_t n;
    buf[2049] = '\0';

    FILE *fp = fopen(file_name.c_str(), mode == BINARY ? "rb" : "r");
    if (!fp) {
        return (-1);
    }

    SHA1_Init(&c);
    while ((n = fread(buf, 1, 2048, fp)) > 0) {
        buf[n] = '\0';
        SHA1_Update(&c, buf, n);
    }

    SHA1_Final(&(md[0]), &c);

    char tmp_buf[65];
	memset(tmp_buf, 0, 65);
    for (n = 0; n < SHA_DIGEST_LENGTH; n++) {
        snprintf(tmp_buf, 3, "%02x", md[n]);
        tmp_buf += 2;
    }

    fclose(fp);
    sha1 = tmp_buf;

    return (0);
}

int sha1_cmd(const std::string& cmd, std::string& sha1) {

	if (cmd.empty()) {
		return -1;
	}

    SHA_CTX c;
    unsigned char buf[2048 + 2];
    unsigned char md[SHA_DIGEST_LENGTH];
    size_t n;
    buf[2049] = '\0';

    FILE *fp = popen(cmd.c_str(), "r");
    if (!fp) {
        return (-1);
    }

    SHA1_Init(&c);
    while ((n = fread(buf, 1, 2048, fp)) > 0) {
        buf[n] = '\0';
        SHA1_Update(&c, buf, n);
    }

    SHA1_Final(&(md[0]), &c);

    char tmp_buf[65];
	memset(tmp_buf, 0, 65);
    for (n = 0; n < SHA_DIGEST_LENGTH; n++) {
        snprintf(tmp_buf, 3, "%02x", md[n]);
        tmp_buf += 2;
    }

    fclose(fp);
    sha1 = tmp_buf;

    return (0);
}

}


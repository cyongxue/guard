/*
 *  agent_info.cpp, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年3月18日
 *      Author: yongxue@cyongxue@163.com
 */

#include <stdio.h>

#include "chroot.h"
#include "log.h"

std::string AgentInfo::agent_guard_id() const {
	auto file = Chroot::instance()->full_file_path(_file_name);

	auto fp = fopen(file.c_str(), "r");
	if (!fp) {
		sleep(1);
		fp = fopen(file.c_str(), "r");
	}

	if (!fp) {
		error("AgentInfo", "Could not open file '%s' due to [(%d)-(%s)].", file.c_str(), errno, strerror(errno));
		return "";
	}

	char buf[1024] = '\0';
	if (fgets(buf, sizeof(buf), fp)) {
		auto len  = strlen(buf) - 1;
		while (len > 0 && ((buf[len] == '\n') || (buf[len] == '\r'))) {
			buf[len--] = '\0';
		}

		std::string ret(buf);
		fclose(fp);

		debug("AgentInfo", "agent guard id: '%s'", buf);
		return ret;
	}

	fclose(fp);
	return "";
}

std::string AgentInfo::agent_profile() const {
	auto file = Chroot::instance()->full_file_path(_file_name);

	auto fp = fopen(file.c_str(), "r");
	if (!fp) {
		sleep(1);
		fp = fopen(file.c_str(), "r");
	}

	if (!fp) {
		error("AgentInfo", "Could not open file '%s' due to [(%d)-(%s)].", file.c_str(), errno, strerror(errno));
		return "";
	}

	// 文件第二行就是
	char buf[1024] = '\0';
	if (fgets(buf, 1024, fp) && fgets(buf, 1024, fp)) {
		auto len  = strlen(buf) - 1;
		while (len > 0 && ((buf[len] == '\n') || (buf[len] == '\r'))) {
			buf[len--] = '\0';
		}

		std::string ret(buf);
		fclose(fp);

		debug("AgentInfo", "agent profile: '%s'", buf);
		return ret;
	}

	fclose(fp);
	return "";
}

int AgentInfo::write_agent_info(const std::string& guard_id, const std::string& profile) {
	if (guard_id.empty()) {
		error("AgentInfo", "guard_id id is empty.");
		return -1;
	}

	auto file = Chroot::instance()->full_file_path(_file_name);
	auto fp = fopen(file.c_str(), "w");
	if (!fp) {
		error("AgentInfo", "Could not open file '%s' due to [(%d)-(%s)].", file.c_str(), errno, strerror(errno));
		return -1;
	}

	fprintf(fp, "%s\n%s\n", guard_id.c_str(), (profile.empty()) ? "-": profile.c_str());
	fclose(fp);
	return 0;
}

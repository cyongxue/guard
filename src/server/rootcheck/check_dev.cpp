/*
 *  check_dev.cpp, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年5月13日
 *      Author: yongxue@cyongxue@163.com
 */

#include "log.h"
#include "check_dev.h"

void CheckDev::check_dev() {
	auto config = std::static_pointer_cast<RkConfig>(_config);
	auto server_name = config->server_name();

	debug(server_name.c_str(), "Starting on check dev.");

	std::string file_path = config->config_data()->basedir() + "/dev";
	(void)read_dir(file_path);

	if (_errors == 0) {
		std::string msg = "No problem detected on the /dev directory. Analyzed " +
				_total + " files.";
		// todo: notify_rk(ALERT_OK, op_msg);
	}

	return;
}


int CheckDev::read_dir(const std::string& dir_name) {
	auto config = std::static_pointer_cast<RkConfig>(_config);
	auto server_name = config->server_name();

	const std::string ignore_dev[] = {
		"MAKEDEV", "README.MAKEDEV", "MAKEDEV.README", ".udevdb", ".udev.tdb", ".initramfs-tools",
		"MAKEDEV.local", ".udev", ".initramfs", "oprofile", "fd", "cgroup",
#ifdef SOLARIS
		".devfsadm_dev.lock", ".devlink_db_lock", ".devlink_db", ".devfsadm_daemon.lock",
		".devfsadm_deamon.lock", ".devfsadm_synch_door", ".zone_reg_door",
#endif
	};
	const std::string ignore_dev_full_path[] = {
		"/dev/shm/sysconfig", "/dev/bus/usb/.usbfs", "/dev/shm", "/dev/gpmctl"
	};

	if (dir_name.empty()) {
		error(server_name.c_str(), "Invalid directory given.");
		return -1;
	}

	DIR *dp = opendir(dir_name.c_str());
	if (!dp) {
		error(server_name.c_str(), "open dir failed, dir: %s", dir_name.c_str());
		return -1;
	}

	struct dirent *entry;
	while ((entry = readdir(dp)) != nullptr) {
		if ((strcmp(entry->d_name, ".") == 0) || ((entry->d_name, "..") == 0)) {
			continue;
		}
		_total++;

		bool ignore_match = false;
		for (const auto& one: ignore_dev) {
			if (strcmp(one.c_str(), entry->d_name) == 0) {
				ignore_match = true;
				break;
			}
		}
		if (ignore_match == true) {
			debug(server_name.c_str(), "match ignore dir entry: %s", entry->d_name);
			continue;
		}

		std::string file_path = dir_name + "/" + entry->d_name;
		ignore_match = false;
		for (const auto& one: ignore_dev_full_path) {
			if (file_path == one) {
				ignore_match = true;
				break;
			}
		}
		if (ignore_match == true) {
			debug(server_name.c_str(), "match ignore full path: %s", file_path.c_str());
			continue;
		}

		/* found a non-ignored entry in the dir, so process it */
		(void)read_file(file_path);
	}
	closedir(dp);
	return 0;
}


int CheckDev::read_file(const std::string& file_name) {
	auto config = std::static_pointer_cast<RkConfig>(_config);
	auto server_name = config->server_name();

	struct stat statbuf;
	if (lstat(file_name.c_str(), &statbuf) < 0) {
		error(server_name.c_str(), "file[%s] stat failed[%d: %s].", file_name.c_str(),
				errno, strerror(errno));
		return -1;
	}

	if (S_ISDIR(statbuf.st_mode)) {
		debug(server_name.c_str(), "Reading dir: %s", file_name.c_str());
		return read_dir(file_name);
	}
	else if (S_ISREG(statbuf.st_mode)) {
		std::string msg = "File '" + file_name.c_str() + "'present on /dev. Possible hidden file.";
		// todo: notify_rk(ALERT_SYSTEM_CRIT, op_msg);
		_errors++;
	}

	return 0;
}

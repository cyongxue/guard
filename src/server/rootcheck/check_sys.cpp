/*
 *  check_sys.cpp, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年5月9日
 *      Author: yongxue@cyongxue@163.com
 */

#include "check_sys.h"

CheckSys::~CheckSys() {
	if (_rw) {
		if (ftell(_rw) == 0) {
			unlink(CheckSysRwFile);
		}
		fclose(_rw);
	}
	if (_rwx) {
		if (ftell(_rwx) == 0) {
			unlink(CheckSysRwxFile);
		}
		fclose(_rwx);
	}
	if (_suid) {
		if (ftell(_suid) == 0) {
			unlink(CheckSysSuidFile);
		}
		fclose(_suid);
	}
}

/**
 * 分析system中的文件file
 * 分析内容：
 * 	1. readdir可以读到，但是stat读不到；说明文件file被hidden；
 * 	2. read文件的length，stat得到的size，要一致；不一致，则异常；（避免误报，存在一些技巧）
 * 	3. 文件other有writable，值得关注，需要记录error；
 * 	4. 文件other有writable，且可以执行exec，则是alert；
 * 	5. 文件other有writable，且owner为root，则是危险，需要alert；
 * 	6. suid的文件也值得关注；
 * 其中，regular file就是正常的文件。如果是directory，则继续进去遍历；
 *
 * return		-1			分析失败
 * 				0			分析完成
 */
int CheckSys::read_sys_file(const std::string& file_name, int do_read) {
	auto config = std::static_pointer_cast<RkConfig>(_config);
	auto server_name = config->server_name();

	_total++;

#ifdef WIN
	// todo:
#endif
	struct stat statbuf;
	if (lstat(file_name.c_str(), &statbuf) < 0) {
#ifndef WIN
		/* readdir可以读到该文件，但是lstat却读不到该文件 */
		std::string msg = "Anomaly detected in file '" + file_name + "'. Hidden from stats, but showing up on readdir."
				" Possible kernel level rootkit.";
		// todo: notify_rk(ALERT_ROOTKIT_FOUND, op_msg);
		_errors++;
#endif
		warn(server_name.c_str(), "check_sys file[%s] stat failed, but opendir have this entry.", file_name.c_str());
		return -1;
	}
	else if (S_ISDIR(statbuf.st_mode)) {
		if (file_name == "/dev/fd") {
			return 0;
		}
		/* ignore the /proc directory(it has size 0) */
		if (statbuf.st_size == 0) {
			return 0;
		}
		// 目录，继续下一步处理
		debug(server_name.c_str(), "check_sys dir[%s] going read sys dir.", file_name.c_str());
		return read_sys_dir(file_name, do_read);
	}

	/* check if the size from stats is the same as when we read the file */
	/* 普通文件，且read，比较stat获取文件长度和read得到的长度是否一致 */
	if (S_ISREG(statbuf.st_mode) && do_read) {
		int fd = open(file_name.c_str(), O_RDONLY, 0);
		/* it may not necessarily open */
		long int total = 0;
		ssize_t nr;
		if (fd >= 0) {
			char buf[OS_SIZE_1024];
			while ((nr = read(fd, buf, sizeof(buf))) > 0) {
				total += nr;
			}
			close(fd);

			if (file_name == "/dev/bus/usb/.usbfs/devices") {
				/* ignore .usbfs/devices */
			}
			else if (total != statbuf.st_size) {
				struct stat statbuf2;
				if ((lstat(file_name.c_str(), &statbuf2) == 0) && (total != statbuf2.st_size)
						&& (statbuf.st_size == statbuf2.st_size)) {
					std::string msg = "Anomaly detected in file '" + file_name + "'. "
							"File size doesn't match what we found. Possible kernel level rootkit.";
					// todo: notify_rk(ALERT_ROOTKIT_FOUND, op_msg);
					_errors++;
				}
			}
		}
		debug(server_name.c_str(), "check_sys file[%s], read length=%d, st_size=%d.",
				file_name.c_str(), total, statbuf.st_size);
	}

	/* if has OTHER write and exec permission, alert */
#ifndef WIN
	if ((statbuf.st_mode & S_IWOTH) == S_IWOTH && S_ISREG(statbuf.st_mode)) {		// 普通文件，被other writable
		if ((statbuf.st_mode & S_IXUSR) == S_IXUSR) {
			// other可以写，然后具备exec，则有alert
			if (_rwx) {
				fprintf(_rwx, "%s\n", file_name.c_str());
			}
			_errors++;
		}
		else {
			// 可读、可写
			if (_rw) {
				fprintf(_rw, "%s\n", file_name.c_str());
			}
		}

		if (statbuf.st_uid == 0) {
			std::string msg = "File '" + file_name + "' is owned by root and has write permissions to anyone.";
			// todo: notify_rk(ALERT_SYSTEM_CRIT, op_msg);
		}
		// other writable，则算一个error
		_errors++;
	}
	else if ((statbuf.st_mode & S_ISUID) == S_ISUID) {
		// 设置uid值
		if (_suid) {
			fprintf(_suid, "%s\n", file_name.c_str());
		}
	}
#endif

	return 0;
}

int CheckSys::scan_dir(const std::string& dir_name, int do_read) {
	auto config = std::static_pointer_cast<RkConfig>(_config);
	auto server_name = config->server_name();

#ifndef WIN
	/* 需要执行read操作的dir */
	std::string dirs_to_doread[] = {
			"/bin", "/sbin", "/usr/bin", "/usr/sbin", "/dev", "/etc", "/boot"
	};
	for (const auto& one: dirs_to_doread) {
		if (dir_name == one) {
			do_read = 1;
			break;
		}
	}
#else
	do_read = 0;
#endif

	/* open the directory, read every entry in the directory */
	DIR *dp = opendir(dir_name.c_str());
	if (!dp) {
		error(server_name.c_str(), "check_sys dir[%s] opendir failed.", dir_name.c_str());
		if (!(dir_name.empty() && (dp = opendir("/")))) {
			error(server_name.c_str(), "check_sys dir[/] opendir failed, dir_name is empty");
			return -1;
		}
	}

	auto run_data = config->run_data();
	unsigned int entry_count = 0;

	struct dirent *entry;
	while ((entry = readdir(dp)) != nullptr) {
		/* Ignore . and .. */
		if ((strcmp(entry->d_name, ".") == 0) || (strcmp(entry->d_name, "..") == 0)) {
			entry_count++;
			continue;
		}

		std::string file_name;
		if (dir_name == "/") {
			file_name = dir_name + entry->d_name;
		}
		else {
			file_name = dir_name + "/" + entry->d_name;
		}
		struct stat statbuf_local;
		if (lstat(file_name.c_str(), &statbuf_local) == 0) {
#ifndef Darwin
            if (S_ISDIR(statbuf_local.st_mode))
#else
            if (S_ISDIR(statbuf_local.st_mode) ||
                    S_ISREG(statbuf_local.st_mode) ||
                    S_ISLNK(statbuf_local.st_mode))
#endif
            {
            	entry_count++;
            }
		}

		for (const auto& one: run_data->rk_files()) {
			if (strcmp(one._rk_file.c_str(), entry->d_name) == 0) {
				warn(server_name.c_str(), "check_sys file[%s/%s] in rk_files.", dir_name.c_str(), entry->d_name);
				_errors++;

				std::string msg = "Rootkit '" + one._rk_name + "' detected by the presence of file'";
				msg += dir_name + "/" + one._rk_file + "'.";
				// todo: notify_rk(ALERT_ROOTKIT_FOUND, op_msg);
			}
		}

		/* ignore the /proc and /sys file systems */
		if (file_name == "/proc" || file_name == "/sys") {
			debug(server_name.c_str(), "check_sys file[%s] in ignore\"/proc, /sys\".", file_name.c_str());
			continue;
		}

		debug(server_name.c_str(), "check_sys file[%s]", file_name.c_str());
		(void)read_sys_file(file_name, do_read);
	}
	closedir(dp);

	return entry_count;
}

/**
 * 检查directory
 */
int CheckSys::read_sys_dir(const std::string& dir_name, int do_read) {
	auto config = std::static_pointer_cast<RkConfig>(_config);
	auto server_name = config->server_name();
	/* ignore user-supplied list */
	for (const auto& one: config->config_data()->ignore()) {
		if (one == dir_name) {
			debug(server_name.c_str(), "check_sys dir[%s] in ignore list.", dir_name.c_str());
			return 1;
		}
	}

	/* should we check for NFS */
	if (config->config_data()->skip_nfs()) {
		// todo: is_nfs()
		// todo: debug(server_name.c_str(), "check_sys file[%s] in nfs file and skip_nfs enable.", dir_name.c_str());
	}

	/* getting the number of nodes, the total number on opendir must be the same. */
	struct stat statbuf;
	if (lstat(dir_name.c_str(), &statbuf) < 0) {
		error(server_name.c_str(), "check_sys dir[%s] lstat failed.", dir_name.c_str());
		return -1;
	}

	/* current device id */
	int did_changed = 0;
	if (_did != statbuf.st_dev) {
		if (_did != 0) {
			did_changed = 1;
		}
		_did = statbuf.st_dev;
	}
	/* 进入该函数必须是dir */
	if (!S_ISDIR(statbuf.st_mode)) {
		error(server_name.c_str(), "check_sys dir[%s] is no dir.", dir_name.c_str());
		return -1;
	}

	int entry_count = scan_dir(dir_name, do_read);
	if (entry_count == -1) {
		error(server_name.c_str(), "check_sys dir[%s] scan entry failed.", dir_name.c_str());
		return -1;
	}

	/* skip further test because the fs cant deliver the stats */
	// todo:

	/* entry count for directory different than the actual link count from stats: 比对文件数 */
	if ((entry_count != (unsigned int)statbuf.st_nlink) &&
			((did_changed == 0) || ((entry_count + 1) != (unsigned)statbuf.st_nlink))) {
		debug(server_name.c_str(), "check_sys dir[%s], entry_count=%d, nlink=%d", entry_count, statbuf.st_nlink);
#ifndef WIN
		/* get stat again */
		struct stat statbuf2;
		if ((lstat(dir_name.c_str(), &statbuf2) == 0) &&
				(statbuf2.st_nlink != entry_count)) {
			debug(server_name.c_str(), "check_sys dir[%s], entry_count=%d, nlink=%d", entry_count, statbuf2.st_nlink);

			std::string msg = "Files hidden inside  directory '" + dir_name +
					"'. Link count does not match number of files (" + entry_count + "," + statbuf.st_nlink + ").";
#ifdef SOLARIS
			if (strncmp(dir_name.c_str(), "/boot", strlen("/boot")) != 0) {
				// todo: notify_rk(ALERT_ROOTKIT_FOUND, op_msg);
				_errors++;
			}
#elif defined(Darwin) || defined(FreeBSD)
			if (strncmp(dir_name.c_str(), "/dev", strlen("/dev")) != 0) {
				// todo: notify_rk(ALERT_ROOTKIT_FOUND, op_msg);
				_errors++;
			}
#else
			// todo: notify_rk(ALERT_ROOTKIT_FOUND, op_msg);
			_errors++;
#endif
		}
#endif	/* WIN*/
	}

	return 0;
}

/* check目录
 * */
void CheckSys::check_sys() {
	auto config = std::static_pointer_cast<RkConfig>(_config);
	auto server_name = config->server_name();
	auto base_dir = config->config_data()->basedir();

	debug(server_name.c_str(), "Starting on check_sys");

	if (config->config_data()->notify() != BaseServer::NotifyType::QUEUE) {
		debug(server_name.c_str(), "check_sys notify type not 'queue', so open sys file.");
		_rw = fopen(CheckSysRwFile, "w");
		_rwx = fopen(CheckSysRwxFile, "w");
		_suid = fopen(CheckSysSuidFile, "w");
	}

	if (config->config_data()->scan_all()) {
		/* scan the whole file system, maybe slow. */
#ifndef WIN
		std::string file_path = "/";
#else
		std::string file_path = base_dir;
#endif
		debug(server_name.c_str(), "check_sys scan all, file_path: [%s], read_all=[%d]", file_path.c_str(), config->config_data()->read_all());
		(void)read_sys_dir(file_path, config->config_data()->read_all());
	}
	else {
		/* scan only specific directories */
#ifndef WIN
		std::string dirs_to_scan[] = {
				"/bin", "/sbin", "/usr/bin", "/usr/sbin",
				"/dev", "/lib", "/etc", "/root",
				"/var/log", "/var/mail", "/var/lib", "/var/www", "/var/tmp",
				"/usr/lib", "/usr/include", "/tmp",
				"/boot", "/usr/local", "/sys"
		};
#else
		std::string dirs_to_scan[] = {
				"C:\\WINDOWS", "C:\\Program Files"
		}
#endif
		for (const auto &one: dirs_to_scan) {
#ifndef WIN
			std::string file_path = base_dir + one;
#else
			std::string file_path = one;
#endif
			debug(server_name.c_str(), "check_sys no scan all, file_path: [%s], read_all=[%d]",
					file_path.c_str(), config->config_data()->read_all());
			(void)read_sys_dir(file_path, config->config_data()->read_all());
		}
	}

	// 得出结论信息
	info(server_name.c_str(), "check_sys have error: %d", _errors);
	if (_errors == 0) {
		std::string msg = "No problem found on the system. Analyzed " + _total + " files.";
		// todo: notify_rk(ALERT_OK, op_msg);
	}
	else if (_rw && _rwx && _suid) {
		std::string msg = "Check the following files for more information:\n";
		msg += (ftell(_rw) == 0)? "": "       "CheckSysRwFile" (list of world writable files)\n";
		msg += (ftell(_rwx) == 0)? "": "       "CheckSysRwxFile" (list of world writable/executable files)\n";
		msg += (ftell(_suid) == 0)? "": "       "CheckSysSuidFile" (list of suid files)";
		// todo: notify_rk(ALERT_SYSTEM_ERR, op_msg);
	}

	// 资源销毁，放到析构中完成
	return;
}

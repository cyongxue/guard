/*
 *  file_db.cpp, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年3月22日
 *      Author: yongxue@cyongxue@163.com
 */

#include <dirent.h>
#include <string.h>
#include <stdexcept>

#include <unistd.h>
#include <sys/stat.h>

#include "md5_op.h"
#include "sha1_op.h"

#include "file_flag.h"
#include "file_db.h"


/**
 * return 0		不是
 * 		  -1	出错
 * 		  1		是
 */
int FileDb::is_nfs(const std::string& dir_name) {
	auto ret = util::is_nfs(dir_name);
	if (ret == -1) {
		error(_serv_name.c_str(), "file '%s' is_nfs failed.", dir_name.c_str());
	}
	return ret;
}

int FileDb::skip_nfs(const std::string& dir_name) {
	auto ret = util::skip_fs(dir_name);
	if (ret == -1) {
		error(_serv_name.c_str(), "file '%s' skip_fs failed.", dir_name.c_str());
	}
	return ret;
}


int FileDb::create_db_data() {
	auto sys_config_data = _serv_config->config_data();

	if (sys_config_data->dirs_infos().empty()) {
		error(_serv_name.c_str(), "No directories to check.");
		return -1;
	}
	info(_serv_name.c_str(), "Starting syscheck database (pre-scan).");

	for (auto one: sys_config_data->dirs_infos()) {
		// todo: read_dir
		auto ret = read_dir(one._dir, one._opts, one._file_restrict);
		if (ret == 1) {
			warn(_serv_name.c_str(), "Dir ignored: '%s'", one._dir.c_str());
		}
		else if (ret == -1) {
			error(_serv_name.c_str(), "Dir loaded failed: '%s'", one._dir.c_str());
		}
		else {
			debug(_serv_name.c_str(), "Directory loaded from syscheck db: '%s'", one._dir.c_str());
		}
	}

#ifdef INOTIFY_ENABLED
	if (_serv_config->run_data()->realtime() && _serv_config->run_data()->realtime()->fd() >= 0) {
		info(_serv_name.c_str(), "Real time file monitoring started");
	}
#endif
	info(_serv_name.c_str(), "Finished creating syscheck database (pre-scan completed).");
	return 0;
}

/**
 * return 	0   正常结束
 * 			-1	失败
 * 			1	忽略
 */
int FileDb::read_dir(const std::string& dir_name, const util::FileCheckOpt& opts, const std::shared_ptr<std::regex> file_restrict) {
	if (dir_name.empty()) {
		error(_serv_name.c_str(), "Attempted to use null string dir. ");
		return -1;
	}

	auto sys_config_data = _serv_config->config_data();
	if (sys_config_data->skip_nfs()) {
		auto ret = is_nfs(dir_name);
		if (ret != 0) {
			return ret;
		}
	}

	struct dirent * entry;
	DIR* dp = opendir(dir_name.c_str());
	if (!dp) {
		if (errno == ENOTDIR) {
			if (read_file(dir_name, opts, file_restrict) == 0) {
				return 0;
			}
		}
#ifdef WIN
		// todo: windows的提示信息
#else
		error(_serv_name.c_str(), "Error opening directory: '%s': %s.", dir_name.c_str(), strerror(errno));
#endif
		return -1;
	}

	/* check for realtime需要依赖inotify */
	if (opts.is_opt(util::FileCheckOpt::FileOpt::CHECK_REALTIME)) {
#ifdef INOTIFY_ENABLED
		if (_serv_config->run_data()->realtime()) {
			(void)_serv_config->run_data()->realtime()->add_dir(dir_name);
		}
#else
		error(_serv_name.c_str(), "realtime monitoring request on unsupported system for '%s'", dir_info._dir.c_str());
#endif
	}

	while ((entry = readdir(dp)) != nullptr) {
		/* Ignore . and ..  */
		if ((strcmp(entry->d_name, ".") == 0) ||
				(strcmp(entry->d_name, "..") == 0)) {
			continue;
		}

		std::string new_path = dir_name;
		if (new_path.find_last_of('/') != (new_path.length() - 1)) {
			new_path += "/";
		}
		new_path += entry->d_name;

		(void)read_file(new_path, opts, file_restrict);
	}
	closedir(dp);

	return 0;
}


int FileDb::read_file(const std::string& file_name, const util::FileCheckOpt& opts, const std::shared_ptr<std::regex> file_restrict) {
	auto sys_config_data = _serv_config->config_data();

	/* check ignore */
	for (auto one: sys_config_data->ignores()) {
		if (strncasecmp(one.c_str(), file_name.c_str(), one.length()) == 0) {
			info(_serv_name.c_str(), "Match ignore, file: '%s'", file_name.c_str());
			return 0;
		}
	}

	/* check regex ignore */
	for (auto one: sys_config_data->ignores_regexs()) {
		if (std::regex_match(file_name, one)) {
			info(_serv_name.c_str(), "Match ignore regex, file: '%s'", file_name.c_str());
			return 0;
		}
	}

	/* 处理文件 */
	struct stat statbuf;
	if (lstat(file_name.c_str(), &statbuf) < 0) {
		if (errno == ENOTDIR) {
			std::string alert_msg = "-1 " + file_name;
			// todo: send_syscheck_msg
			return 0;
		}
		else {
			error(_serv_name.c_str(), "Error accessing '%s': '%s'", file_name.c_str(), strerror(errno));
			return -1;
		}
	}
	/* 如果目录，继续read dir */
	if (S_ISDIR(statbuf.st_mode)) {
		debug(_serv_name.c_str(), "Reading dir: '%s'", file_name.c_str());
#ifdef WIN
		// todo: windows
#endif
		return read_dir(file_name, opts, file_restrict);
	}

	return deal_file(statbuf, file_name, opts, file_restrict);
}


std::string FileDb::create_send_alert_msg(const util::FileCheckOpt& opts, const struct stat& statbuf,
		const std::string& md5, const std::string& sha1, const std::string& file_name) {
	std::string ret;
	ret += opts.is_opt(util::FileCheckOpt::CHECK_SIZE) ? (long)statbuf.st_size : 0 + ":";
	ret += opts.is_opt(util::FileCheckOpt::CHECK_PERM) ? (int)statbuf.st_mode : 0 + ":";
	ret += opts.is_opt(util::FileCheckOpt::CHECK_OWNER) ? (int)statbuf.st_uid : 0 + ":";
	ret += opts.is_opt(util::FileCheckOpt::CHECK_GROUP) ? (int)statbuf.st_gid : 0 + ":";
	ret += opts.is_opt(util::FileCheckOpt::CHECK_MD5SUM) ? md5 : "xxx" + ":";
	ret += opts.is_opt(util::FileCheckOpt::CHECK_SHA1SUM) ? sha1 : "xxx" + " ";
	ret += file_name;
	return ret;
}

int FileDb::create_sum_by_alertmsg(const std::string& file_name, const std::string& alertmsg, std::string& sum) {
	struct stat statbuf;
#ifdef WIN
	// todo:
#else
	if (lstat(file_name.c_str(), &statbuf) < 0)
#endif
	{
		error(_serv_name.c_str(), "file stat error: '%s':%d, %s", file_name.c_str(), errno, strerror(errno));
		std::string send_alert_msg = "-1 " + file_name;
		// todo: send_syscheck_msg
		return -1;
	}

	int size = alertmsg[0] == '+' ? 1 : 0;
	int perm = alertmsg[1] == '+' ? 1 : 0;
	int owner = alertmsg[2] == '+' ? 1 : 0;
	int group = alertmsg[3] == '+' ? 1 : 0;
	int md5sum = alertmsg[4] == '+' ? 1 : 0;
	/* sha1 sum */
	int sha1sum = 0;
	if (alertmsg[5] == '+') {
		sha1sum = 1;
	} else if (alertmsg[5] == 's') {
		sha1sum = 1;
	} else if (alertmsg[5] == 'n') {
		sha1sum = 0;
	}

	std::string md5 = "xxx";
	std::string sha1 = "xxx";
	if (S_ISREG(statbuf.st_mode)) {
		if (md5sum || sha1sum) {
			auto prefilter_cmd = _serv_config->config_data()->prefilter_cmd();
			if (md5_sha1_file(file_name, prefilter_cmd, sec::SecFileType::BINARY, md5, sha1) < 0) {
				warn(_serv_name.c_str(), "create md5 and sha1 failed: '%s'", file_name.c_str());
			}
		}
	}
#ifndef WIN
	else if (S_ISLNK(statbuf.st_mode)) {
		struct stat statbuf_lnk;
		if (stat(file_name.c_str(), &statbuf_lnk) == 0) {
			if (S_ISREG(statbuf_lnk.st_mode)) {
				if (md5sum || sha1sum) {
					auto prefilter_cmd = _serv_config->config_data()->prefilter_cmd();
					if (md5_sha1_file(file_name, prefilter_cmd, sec::SecFileType::BINARY, md5, sha1) < 0) {
						warn(_serv_name.c_str(), "create md5 and sha1 failed: '%s'", file_name.c_str());
					}
				}
			}
		}
	}
#endif

	sum += size == 0 ? 0 : (long)statbuf.st_size + ":";
	sum += perm == 0 ? 0 : (int)statbuf.st_mode + ":";
	sum += owner == 0 ? 0 : (int)statbuf.st_uid + ":";
	sum += group == 0 ? 0 : (int)statbuf.st_gid + ":";
	sum += md5sum == 0 ? "xxx" : md5;
	sum += sha1sum == 0 ? "xxx" : sha1;

	return 0;
}


std::string FileDb::create_save_alert_msg(const util::FileCheckOpt& opts, const struct stat& statbuf,
		const std::string& md5, const std::string& sha1, char sha1s) {
	std::string ret;
	ret += opts.is_opt(util::FileCheckOpt::CHECK_SIZE) ? '+' : '-';
	ret += opts.is_opt(util::FileCheckOpt::CHECK_PERM) ? '+' : '-';
	ret += opts.is_opt(util::FileCheckOpt::CHECK_OWNER) ? '+' : '-';
	ret += opts.is_opt(util::FileCheckOpt::CHECK_GROUP) ? '+' : '-';
	ret += opts.is_opt(util::FileCheckOpt::CHECK_MD5SUM) ? '+' : '-';
	ret += sha1s;
	ret += opts.is_opt(util::FileCheckOpt::CHECK_SIZE) ? (long)statbuf.st_size : 0 + ":";
	ret += opts.is_opt(util::FileCheckOpt::CHECK_PERM) ? (int)statbuf.st_mode : 0 + ":";
	ret += opts.is_opt(util::FileCheckOpt::CHECK_OWNER) ? (int)statbuf.st_uid : 0 + ":";
	ret += opts.is_opt(util::FileCheckOpt::CHECK_GROUP) ? (int)statbuf.st_gid : 0 + ":";
	ret += opts.is_opt(util::FileCheckOpt::CHECK_MD5SUM) ? md5 : "xxx" + ":";
	ret += opts.is_opt(util::FileCheckOpt::CHECK_SHA1SUM) ? sha1 : "xxx";

	return ret;
}

/**
 * 文件处理
 */
int FileDb::deal_file(const struct stat& statbuf, const std::string& file_name,
		const util::FileCheckOpt& opts, const std::shared_ptr<std::regex> file_restrict) {
	/* 先检查file restrict是否匹配 */
	if (file_restrict) {
		if (!std::regex_match(file_name, *file_restrict.get())) {
			return 0;
		}
	}

	/* 普通文件处理 */
	if (S_ISREG(statbuf.st_mode) || S_ISLNK(statbuf.st_mode)) {

		/* 配合DB操作 */
		auto result = _db.find(file_name);
		if (result == _db.end()) {
			// 第一次加入到db中
			std::string file_md5 = "xxx";
			std::string file_sum = "xxx";
			char sha1s = '+';

			/* Generate checksums */
			if (opts.is_opt(util::FileCheckOpt::CHECK_MD5SUM) || opts.is_opt(util::FileCheckOpt::CHECK_SHA1SUM)) {
				if (S_ISLNK(statbuf.st_mode)) {
					struct stat statbuf_lnk;
					if (stat(file_name.c_str(), &statbuf_lnk) == 0) {
						if (S_ISREG(statbuf_lnk.st_mode)) {
							auto prefilter_cmd = _serv_config->config_data()->prefilter_cmd();
							if (md5_sha1_file(file_name, prefilter_cmd, sec::SecFileType::BINARY, file_md5, file_sum) < 0) {
								warn(_serv_name.c_str(), "create md5 and sha1 failed: '%s'", file_name.c_str());
							}
						}
					}
				}
				else {
					auto prefilter_cmd = _serv_config->config_data()->prefilter_cmd();
					if (md5_sha1_file(file_name, prefilter_cmd, sec::SecFileType::BINARY, file_md5, file_sum) < 0) {
						warn(_serv_name.c_str(), "create md5 and sha1 failed: '%s'", file_name.c_str());
					}
				}

				if (opts.is_opt(util::FileCheckOpt::CHECK_SEECHANGES)) {
					sha1s = 's';
				}
			}
			else {
				if (opts.is_opt(util::FileCheckOpt::CHECK_SEECHANGES)) {
					sha1s = 'n';
				}
				else {
					sha1s = '-';
				}
			}
#ifndef WIN
			if (opts.is_opt(util::FileCheckOpt::CHECK_SEECHANGES)) {
				auto alert_dump = seechanges_file_changes(file_name);
			}
#endif
			auto save_alert_msg = create_save_alert_msg(opts, statbuf, file_md5, file_sum, sha1s);
			_db.insert(std::make_pair(file_name, save_alert_msg));

			auto send_alert_msg = create_send_alert_msg(opts, statbuf, file_md5, file_sum, file_name);
			// todo: send_syscheck_msg
		}
		else {
			// 和db中的进行对比，得出是否发生变更
			std::string new_sum;
			if (create_sum_by_alertmsg(file_name, result->second, new_sum) < 0) {
				return 0;
			}
			if (strcmp(new_sum.c_str(), result->second.c_str() + 6) != 0) {
				// 说明发生了变更
				std::string send_alert_msg;
#ifdef WIN
				send_alert_msg = new_sum + " " + file_name;
#else
				const std::string& alertmsg = result->second;
				if (alertmsg[5] == 's' || alertmsg[5] == 'n') {
					auto alert_dump = seechanges_file_changes(file_name);
					if (!alert_dump.empty()) {
						send_alert_msg = new_sum + " " + file_name + "\n" + alert_dump;
					}
					else {
						send_alert_msg = new_sum + " " + file_name;
					}
				}
				else {
					send_alert_msg = new_sum + " " + file_name;
				}
#endif
				// todo: send_syscheck_msg;
			}
		}

		/* 避免扫描对于系统资源的占用，一定次数之后，进行适当的休眠 */
		if (_count >= _serv_config->config_data()->sleep_after()) {
			sleep(_serv_config->config_data()->tsleep());
			_count = 0;
		}
		_count++;
		debug(_serv_name.c_str(), "file: '%s'", file_name.c_str());
	}
	else {
		debug(_serv_name.c_str(), "*** IRREG file: '%s'", file_name.c_str());
	}

	return 0;
}

/**
 * 依次创建dir，最终创建完整的路径
 * /Users/yongxue/work/eclipse-work/mytest/rootkit/backdoor.c
 */
int FileDb::seechanges_create_path(const std::string& file_name) {

	char *buffer = strdup(file_name.c_str());
	char *new_dir = buffer;

	char *tmpstr = strchr(buffer + 1, '/');
	if (!tmpstr) {
		error(_serv_name.c_str(), "Invalid path name: '%s'", file_name.c_str());
		free(buffer);
		return (0);
	}
	*tmpstr = '\0';
	tmpstr++;

	while (1) {
		if (util::is_dir(new_dir) == false) {
#ifndef WIN
			if (mkdir(new_dir, 0770) == -1)
#else
			if (mkdir(new_dir) == -1)
#endif
			{
				error(_serv_name.c_str(), "Could not create directory '%s' due to [(%d)-(%s)].", new_dir, errno, strerror(errno));
				free(buffer);
				return (0);
			}
		}

		if (*tmpstr == '\0') {
			break;
		}

		tmpstr[-1] = '/';					// 修改为'\0'的地方，重新设置为'/'
		tmpstr = strchr(tmpstr, '/');
		if (!tmpstr) {
			break;
		}
		*tmpstr = '\0';
		tmpstr++;
	}

	free(buffer);
	return (1);
}

/**
 * 文件内容复制过去
 */
bool FileDb::seechanges_dup_file(const std::string& old, const std::string& current) {
	FILE *fpr = fopen(old.c_str(), "r");
	if (!fpr) {
		return false;
	}
	auto fpw = fopen(current.c_str(), "w");
	if (!fpw) {
		fclose(fpr);
		return false;
	}

	unsigned char buf[2048 + 1];
	buf[2048] = '\0';
	size_t n = fread(buf, 1, 2048, fpr);
	do {
		buf[n] = '\0';
		fwrite(buf, n, 1, fpw);
	} while ((n = fread(buf, 1, 2048, fpr)) > 0);

	fclose(fpr);
	fclose(fpw);

	return true;
}

/**
 * 检查是否在不关注diff列表中
 */
bool FileDb::is_nodiff(const std::string& file_name) {
	auto nodiffs = _serv_config->config_data()->nodiffs();
	if (!nodiffs.empty()) {
		for (const auto& one: nodiffs) {
			if (one == file_name) {
				return true;
			}
		}
	}

	auto nodiffs_regex = _serv_config->config_data()->nodiffs_regexs();
	if (!nodiffs_regex.empty()) {
		for (const auto& one: nodiffs_regex) {
			if (std::regex_match(file_name, one)) {
				return true;
			}
		}
	}

	return false;
}

/**
 * 生成文件变更的alert信息
 */
std::string FileDb::gen_diff_alert(const std::string& diff_location, time_t alert_diff_time) {
	FILE * fp = fopen(diff_location.c_str(), "r");
	if (!fp) {
		error(_serv_name.c_str(), "Unable to generate diff alert.");
		return "";
	}

	char * tmp_str = nullptr;
	char buf[6144 + 1];
	buf[6144] = '\0';
	auto n = fread(buf, 1, 4096 - 1, fp);
	if (n <= 0) {
		error(_serv_name.c_str(), "Unable to generate diff alert.(fread)");
		fclose(fp);
		return "";
	}
	else if (n >= 4000) {
		buf[n] = '\0';
		tmp_str = strrchr(buf, '\n');
		if (tmp_str) {
			*tmp_str = '\0';
		}
		else {
			buf[256] = '\0';
		}
	}
	else {
		buf[n] = '\0';
	}

	n = 0;
	/* Get up to 20 line changes, 即仅保留20行不同 */
	tmp_str = buf;

	while (tmp_str && (*tmp_str != '\0')) {
		tmp_str = strchr(tmp_str, '\n');
		if (!tmp_str) {
			break;
		} else if (n >= 19) {
			*tmp_str = '\0';
			break;
		}
		n++;
		tmp_str++;
	}

	std::string ret;
	if (n >= 19) {
		ret = buf + "\nMore changes..";
	}
	else {
		ret = buf;
	}
	return ret;
}

/* check if the files has changed */
std::string FileDb::seechanges_file_changes(const std::string& file_name) {
	std::string ret;

	auto diff_dir = Chroot::instance()->full_file_path(DIFF_DIR);

	// old file info
	auto old_location = diff_dir + "/local" + file_name + "/" + DIFF_LAST_FILE;
	std::string old_md5;
	if (sec::md5_file(old_location, sec::SecFileType::BINARY, old_md5) != 0) {
		/* If the file is not there, rename new location to last location */
		if (seechanges_create_path(old_location) != 1) {
			error(_serv_name.c_str(), "create file location failed: '%s'", old_location.c_str());
		} else if (seechanges_dup_file(file_name, old_location) == false){
			error(_serv_name.c_str(), "dup file failed: '%s'", old_location.c_str());
		}
		return ret;
	}

	// new file info
	std::string new_md5;
	if (sec::md5_file(file_name, sec::SecFileType::BINARY, new_md5) != 0) {
		return ret;
	}

	if (old_md5 == new_md5) {
		/* if they match, keep the old file and remove the new */
		return ret;
	}

	/* md5值发生变化，需要处理 */
	// backup old location
	auto old_date_of_change = util::file_mod_time(old_location);
	std::string tmp_location = diff_dir + "/local" + file_name + "/state." + (int)old_date_of_change;
	rename(old_location.c_str(), tmp_location.c_str());

	// rename new old location
	if (seechanges_dup_file(file_name, old_location) == false) {
		error(_serv_name.c_str(), "Unable to create snapshot for '%s'", file_name.c_str());
		return ret;
	}

	auto new_date_of_change = util::file_mod_time(old_location);
	auto base_dir = _serv_config->server()->base_dir();
	std::string old_tmp = base_dir + TMP_DIR + "/syscheck-changes-" + old_md5 + "-" + old_date_of_change;
	std::string new_tmp = base_dir + TMP_DIR + "/syscheck-changes-" + new_md5 + "-" + new_date_of_change;
	std::string diff_tmp = base_dir + TMP_DIR + "/syscheck-changes-" + old_md5 + "-"
			+ old_date_of_change + "-" + new_md5 + "-" + new_date_of_change;

	std::string diff_location = diff_dir + "/local" + file_name + "/diff." + new_date_of_change;

	/* Create symlinks */
	if (symlink(old_location.c_str(), old_tmp.c_str()) == -1) {			// 最新的file name的
		error(_serv_name.c_str(), "Unable to link from '%s' to '%s' due to [(%d)-(%s)].",
				old_location.c_str(), old_tmp.c_str(), errno, strerror(errno));
		goto cleanup;
	}
	if (symlink(tmp_location.c_str(), new_tmp.c_str()) == -1) {			// 上一次file name对应的
		error(_serv_name.c_str(), "Unable to link from '%s' to '%s' due to [(%d)-(%s)].",
				tmp_location.c_str(), new_tmp.c_str(), errno, strerror(errno));
		goto cleanup;
	}
	if (symlink(diff_location.c_str(), diff_tmp.c_str()) == -1) {			// 上一次file name对应的
		error(_serv_name.c_str(), "Unable to link from '%s' to '%s' due to [(%d)-(%s)].",
				diff_location.c_str(), diff_tmp.c_str(), errno, strerror(errno));
		goto cleanup;
	}

	int status = -1;
	if (is_nodiff(file_name)) {
		std::string nodiff_msg = "<Diff truncated because nodiff option>";
		FILE* fdiff = fopen(diff_location.c_str(), "w");
		if (!fdiff) {
			error(_serv_name.c_str(), "Unable to open file for writing `%s`", diff_location.c_str());
			goto cleanup;
		}
		fwrite(nodiff_msg.c_str(), nodiff_msg.length() + 1, 1, fdiff);
		fclose(fdiff);

		status = 0;
	}
	else {
		/* 利用diff命令得到两个文件的不同，写入到diff文件中 */
		std::string diff_cmd = "diff \"" + new_tmp + "\" \"" + old_tmp + "\" > \"" + diff_tmp + "\" 2> /dev/null";
		if (system(diff_cmd.c_str()) != 256) {
			error(_serv_name.c_str(), "Unable to run `%s`", diff_cmd.c_str());
			goto cleanup;
		}

		status = 0;
	}

cleanup:
	unlink(old_tmp.c_str());
	unlink(new_tmp.c_str());
	unlink(diff_tmp.c_str());

	if (status == -1) {
		return ret;
	}

	return gen_diff_alert(diff_location, new_date_of_change);			// 生成alert信息
}

int FileDb::md5_sha1_file(const std::string& file_name, const std::string& prefilter_cmd, int mode,
				std::string& md5, std::string& sha1) {

	if (prefilter_cmd.empty()) {
		if ((sec::sha1_file(file_name, mode, sha1) == 0)
				&& (sec::md5_file(file_name, mode, md5) == 0)) {
			return 0;
		}
		else {
			return -1;
		}
	}
	else {
		std::string cmd = prefilter_cmd + " " + file_name;
		if ((sec::sha1_cmd(cmd, sha1) == 0)
				&& (sec::md5_cmd(cmd, md5) == 0)) {
			return 0;
		}
		else {
			return -1;
		}
	}

	return 0;
}


int FileDb::realtime_checksum_file(const std::string& file_name) {
	auto result = _db.find(file_name);
	if (result != _db.end()) {
		auto save_alert_msg = result->second;

		std::string new_sum;
		if (create_sum_by_alertmsg(file_name, save_alert_msg, new_sum) < 0) {
			return 0;
		}

		if (strcmp(new_sum.c_str(), result->second.c_str() + 6) != 0) {
			// 说明发生了变更
			std::string send_alert_msg;
#ifdef WIN
			send_alert_msg = new_sum + " " + file_name;
#else
			const std::string& alertmsg = result->second;
			if (alertmsg[5] == 's' || alertmsg[5] == 'n') {
				auto alert_dump = seechanges_file_changes(file_name);
				if (!alert_dump.empty()) {
					send_alert_msg = new_sum + " " + file_name + "\n" + alert_dump;
				}
				else {
					send_alert_msg = new_sum + " " + file_name;
				}
			}
			else {
				send_alert_msg = new_sum + " " + file_name;
			}
#endif
			// todo: send_syscheck_msg;

			return 1;
		}
	}

	return 0;
}

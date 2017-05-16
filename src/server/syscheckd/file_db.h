/*
 *  file_db.h, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年3月22日
 *      Author: yongxue@cyongxue@163.com
 */

#ifndef SRC_SERVER_SYSCHECKD_FILE_DB_H_
#define SRC_SERVER_SYSCHECKD_FILE_DB_H_

#include "syscheck_config.h"

/**
 * 单例模式
 * 创建file db时，尝试创建realtime对象
 */
class FileDb {
	private:
		std::unordered_map<std::string, std::string>	_db;
		int												_count;

		std::shared_ptr<SyscheckConfig>					_serv_config;
		std::string										_serv_name;

	protected:
		int is_nfs(const std::string& dir_name);
		int skip_nfs(const std::string& dir_name);

		int deal_file(const struct stat& statbuf, const std::string& file_name,
				const util::FileCheckOpt& opts, const std::shared_ptr<std::regex> file_restrict);
		int md5_sha1_file(const std::string& file_name, const std::string& prefilter_cmd, int mode,
				std::string& md5, std::string& sha1);

		/* 检查是否在不关注diff列表中 */
		bool is_nodiff(const std::string& file_name);
		std::string gen_diff_alert(const std::string& diff_location, time_t alert_diff_time);
		/* check if the files has changed */
		std::string seechanges_file_changes(const std::string& file_name);
		bool seechanges_dup_file(const std::string& old, const std::string& current);
		int seechanges_create_path(const std::string& file_name);

		// 文件处理
		int deal_file(const struct stat& statbuf, const std::string& file_name,
				const util::FileCheckOpt& opts, const std::shared_ptr<std::regex> file_restrict);
		std::string create_send_alert_msg(const util::FileCheckOpt& opts, const struct stat& statbuf,
				const std::string& md5, const std::string& sha1, const std::string& file_name);
		std::string create_save_alert_msg(const util::FileCheckOpt& opts, const struct stat& statbuf,
				const std::string& md5, const std::string& sha1, char sha1s);
		int create_sum_by_alertmsg(const std::string& file_name, const std::string& alertmsg, std::string& sum);


	public:
		FileDb(std::shared_ptr<SyscheckConfig> serv_config): _serv_config(serv_config) {
			_serv_name = serv_config->server_name();
			_count = 0;
		}

		void set_serv_name(const std::string& server_name);
		const std::string& serv_name() const { return _serv_name; }

		int create_db_data();
		int read_dir(const std::string& dir_name, const util::FileCheckOpt& opts, const std::shared_ptr<std::regex> file_restrict);
		int read_file(const std::string& file_name, const util::FileCheckOpt& opts, const std::shared_ptr<std::regex> file_restrict);

		/* 由Realtime触发的文件check */
		int realtime_checksum_file(const std::string& file_name);
};



#endif /* SRC_SERVER_SYSCHECKD_FILE_DB_H_ */

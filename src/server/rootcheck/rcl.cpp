/*
 *  rcl.cpp, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年4月11日
 *      Author: yongxue@cyongxue@163.com
 */

#include "rcl.h"

#include "defs.h"
#include "log.h"

/**
 * 经过调整后，全部采用正则表达式方式处理
 * input params：
 * 		dir： 待check的目录
 * 		file：文件匹配的正则
 */
bool RclCheckFile::check_dir(const std::string& dir, const std::string& reg_file, const std::string& pattern) {
	DIR *dp = opendir(dir.c_str());
	if (!dp) {
		return false;
	}

	bool ret = false;
	struct dirent *entry;
	struct stat statbuf_local;

	while ((entry = readdir(dp)) != nullptr) {
		/* Ignore . and ..  */
		if ((strcmp(entry->d_name, ".") == 0) || (strcmp(entry->d_name, "..") == 0)) {
			continue;
		}

		std::string file_path = dir + "/" + entry->d_name;

		/* check if the read entry matches the provided file name */
		std::regex file_pattern(reg_file);
		if (std::regex_match(entry->d_name, file_pattern)) {
			if (check_file(file_path, pattern)) {
				ret = true;
			}
		}

		/* check if file is a directory */
		if (lstat(file_path.c_str(), &statbuf_local) == 0) {
			if (S_ISDIR(statbuf_local.st_mode)) {
				if (check_dir(file_path, reg_file, pattern)) {
					ret = true;
				}
			}
		}
	}

	closedir(dp);
	return ret;
}

/**
 * 根据regex匹配file中的内容
 */
bool RclCheckFile::check_file(const std::string& file, const std::string& pattern) {
	if (file.empty()) {
		return 0;
	}

	auto run_data = std::static_pointer_cast<RkRunData>(_run_data);
	auto alert_msgs = run_data->alert_msg();

	auto split_file = util::split_str(file, ',');
	for (const auto& one: split_file) {
		/* if we don't have a pattern, just check if the file/dir is there */
		if (pattern.empty()) {
			if (util::is_file(one)) {
				std::string msg = "File: " + one + ".";
				if (alert_msgs->is_have(msg)) {
					return true;
				}

				(void)alert_msgs->add_end(msg);
				return true;
			}
		}
		else {
			bool full_negate = _rcl->is_all_negate_pattern(pattern);

			debug(_rcl->server_name().c_str(), "checking file: %s.", file.c_str());
			FILE *fp = fopen(file.c_str(), "r");
			if (fp) {
				debug(_rcl->server_name().c_str(), "starting check file: %s", file.c_str());
				char buf[OS_SIZE_2048] = '\0';
				while (fgets(buf, sizeof(buf), fp) != nullptr) {
					char *nbuf;
					nbuf = strchr(buf, '\n');
#ifdef WIN
					nbuf = strchr(buf, '\r');
#endif
					if (nbuf) {
						*nbuf = '\0';
					}

					/* matched */
					bool pt_match = _rcl->pattern_match(buf, pattern);
					debug(_rcl->server_name().c_str(), "buf=\"%s\"; Pattern=\"%s\".", buf, pattern.c_str());
					debug(_rcl->server_name().c_str(), "pt_match=%d; full_negate=%d.", pt_match, full_negate);
					if ((pt_match == true) && (full_negate == false)) {
						debug(_rcl->server_name().c_str(), "alerting file %s on line %s.", file.c_str(), buf);
						fclose(fp);

						std::string msg = " File: " + file + ".";
						if (alert_msgs->is_have(msg)) {
							return true;
						}
						(void)alert_msgs->add_end(msg);
						return true;
					}
					else if ((pt_match == false) && (full_negate == true)){
						debug(_rcl->server_name().c_str(), "found a complete match for full_negate.");
						full_negate = false;
						break;
					}
				}
				fclose(fp);

				if (full_negate == true) {
					debug(_rcl->server_name().c_str(), "full_negate alerting - file %s.", file.c_str());
					std::string msg = " File: " + file + ".";
					if (alert_msgs->is_have(msg)) {
						return true;
					}
					(void)alert_msgs->add_end(msg);
					return true;
				}
			}
		}
	}

	return false;
}


int Rcl::NameInfo::set_condition(const std::string& condition) {
	if (condition == "all") {
		_condition |= RclCondition::COND_ALL;
	}
	else if (condition == "any") {
		_condition |= RclCondition::COND_ANY;
	}
	else if (condition == "any required") {
		_condition |= RclCondition::COND_ANY;
		_condition |= RclCondition::COND_REQ;
	}
	else if (condition == "all required") {
		_condition |= RclCondition::COND_ALL;
		_condition |= RclCondition::COND_REQ;
	}
	else {
		_condition |= RclCondition::COND_INV;
		return false;
	}

	return true;
}

char * Rcl::get_valid_line(char *buf, unsigned int size) {
	while (fgets(buf, size, _fp) != nullptr) {
		char * nbuf = strchr(buf, '\n');
		if (nbuf) {
			*nbuf = '\0';
		}

		nbuf = buf;
		while (*nbuf != '\0') {
			if (*nbuf == ' ' || *nbuf == '\t') {
				nbuf++;
				continue;
			}
			else if (*nbuf == '#') {
				*nbuf = '\0';
				continue;
			}
			else {
				break;
			}
		}

		if (*nbuf == '\0') {
			continue;
		}

		return nbuf;
	}
	return nullptr;
}


bool Rcl::is_name(const char * buf) {
	if ((*buf == '[') && (buf[strlen(buf) - 1] == ']')) {
		return true;
	}
	return false;
}

/**
 * name的格式如下：
 * [CIS - Testing against the CIS Red Hat Enterprise Linux 5 Benchmark v2.1.0] [any required] [http://www.claw.net/]
 */
std::shared_ptr<Rcl::NameInfo> Rcl::get_name(char* buf) {

	/* Check if name is valid */
	if (!is_name(buf)) {
		return nullptr;
	}

	/* name */
	buf++;
	char *tmp_location = strchr(buf, ']');
	if (!tmp_location) {
		return nullptr;
	}
	*tmp_location = '\0';

	auto name_info = std::make_shared<Rcl::NameInfo>();
	/* codition */
	tmp_location++;
	if ((*tmp_location != ' ') && (tmp_location[1] != '[')) {
		return nullptr;
	}
	tmp_location += 2;
	char *tmp_location2 = strchr(tmp_location, ']');
	if (!tmp_location2) {
		return nullptr;
	}
	*tmp_location2 = '\0';
	if (!name_info->set_condition(tmp_location)) {
		return nullptr;
	}

	/* reference */
	tmp_location2++;
	if ((*tmp_location2 != ' ') && (tmp_location2[1] != '[')) {
		return nullptr;
	}
	tmp_location2 += 2;
	tmp_location = strchr(tmp_location2, ']');
	if (!tmp_location) {
		return nullptr;
	}
	*tmp_location = '\0';

	name_info->_name = buf;
	name_info->_reference = tmp_location2;

	return name_info;
}

/**
 * 格式如下：
 * # RC scripts location
 * $rc_dirs=/etc/rc.d/rc2.d,/etc/rc.d/rc3.d,/etc/rc.d/rc4.d,/etc/rc.d/rc5.d;
 * 		'$': 开头标记；
 * 		';': 截止标记
 * 	rc_dirs					变量名，name
 * 	/etc/rc.d/rc2.d等		变量值，value
 */
int Rcl::get_var(char *nbuf) {
	if (*nbuf != '$') {
		return 0;
	}

	char *tmp = strchr(nbuf, ';');
	if (tmp) {
		*tmp = '\0';
	}
	else {
		return -1;
	}

	tmp = strchr(nbuf, '=');
	if (tmp) {
		*tmp = '\0';
		tmp++;
	}
	else {
		return -1;
	}

	std::string name = nbuf;
	std::string value = tmp;
	(void)add_var_value(name, value);
	return 1;
}

int Rcl::add_var_value(const std::string& key, const std::string& value) {
	if (key.empty() || value.empty()) {
		return 0;
	}

	auto found = _var_values.find(key);
	if (found != _var_values.end()) {
		/* Duplicate entry */
		return 1;
	}

	_var_values.insert(std::pair<std::string, std::string>(key, value));
	return 1;
}

bool Rcl::ValueInfo::set_type(const std::string& type) {
	if (type == "f") {
		_type = RclType::FILE_TYPE;
	}
	else if (type == "r") {
		_type = RclType::REGISTRY_TYPE;
	}
	else if (type == "p") {
		_type = RclType::PROCESS_TYPE;
	}
	else if (type == "d") {
		_type = RclType::DIR_TYPE;
	}
	else {
		return false;
	}
	return true;
}

/**
 * 格式如下：
 * 		f:$php.ini -> r:^register_globals = On;
 * 		f											类型type
 * 		$php.ini -> r:^register_globals = On;		值，value
 */
std::shared_ptr<Rcl::ValueInfo> Rcl::get_value(char *buf) {

	char *value = strchr(buf, ':');
	if (value == nullptr) {
		return nullptr;
	}
	*value = '\0';

	auto value_info = std::make_shared<Rcl::ValueInfo>();

	/* set type */
	value++;
	char* tmp_str = strchr(value, ';');
	if (tmp_str == nullptr) {
		return nullptr;
	}
	*tmp_str = '\0';
	if (!value_info->set_type(buf)) {
		return nullptr;
	}

	/* set value */
	if (value[0] == '!') {
		value_info->set_negate(true);
		value++;
	}
	value_info->set_value(value);

	return value_info;
}

/**
 * value的格式如下：
 * $web_dirs -> .php$ -> r:eval\(base64_decode\(\paWYo;
 */
std::string Rcl::get_pattern(const std::string& value) {
	auto pos = value.find(" -> ");
	if (pos == std::string::npos) {
		return "";
	}
	return value.substr(pos + 4);
}

bool Rcl::rcl_open() {
	auto fp = fopen(_rcl_file.c_str(), "r");
	if (!fp) {
		error(_server_name.c_str(), "No rcl file: '%s'", _rcl_file.c_str());
		return false;
	}

	_fp = fp;

	return true;
}

/**
 * 类似：f:$php.ini -> r:^register_globals = On;  的配置中，
 * ^register_globals = On   就是regex形式的pattern，但是可能是利用" && "表示多个pattern的存在
 * eg： ^register_globals = On && ^register_globals = Off
 * 也可以在前面"!"表示任何一个的取反
 */
bool Rcl::is_all_negate_pattern(const std::string& pattern) {
	if (pattern.empty()) {
		warn(_server_name.c_str(), "The pattern is empty.");
		return false;
	}

	std::string tmp_pattern = pattern;
	auto pos = tmp_pattern.find(" && ");
	do {
		if (tmp_pattern[0] != '!') {
			debug(_server_name.c_str(), "No negate: %s", tmp_pattern.c_str());
			return false;
		}

		if (pos != std::string::npos) {
			auto next_pos = pos + 4;

			tmp_pattern = tmp_pattern.substr(next_pos);
			pos = tmp_pattern.find(" && ");
		}
		else {
			break;
		}
	} while (!tmp_pattern.empty());

	debug(_server_name.c_str(), "all negate: %s, %d", tmp_pattern.c_str(), pos);
	return true;
}

/* Checks if the specific pattern is present on str.
 * A pattern can be preceeded by:
 *                                =: (for equal) - default - strcasecmp
 *                                r: (for guard regexes)
 *                                >: (for strcmp greater)
 *                                <: (for strcmp  lower)
 *
 * Multiple patterns can be specified by using " && " between them.
 * All of them must match for it to return true.
 *
 * eg：r:^\.wp_version && >:$wp_version = '3.2.1'
 * eg: r:^register_globals = On
 */
bool Rcl::pattern_match(const std::string& str, const std::string& pattern) {
	if (str.empty() || pattern.empty()) {
		debug(_server_name.c_str(), "str or pattern is emtpy");
		return false;
	}

	bool is_match = false;

	size_t begin_pos = 0;
	size_t pos = pattern.find(" && ", begin_pos);
	do {
		bool neg = false;
		/* parse negate flag */
		auto one_pattern = pattern.substr(begin_pos, pos);
		if(one_pattern[0] == '!') {
			neg = true;
			one_pattern = one_pattern.substr(1);
		}

		/* deal pattern match */
		if (strncasecmp(one_pattern.c_str(), "=:", 2) == 0) {
			if (one_pattern.substr(2) == str) {
				is_match = true;
			}
		}
		else if (strncasecmp(one_pattern.c_str(), "r:", 2) == 0) {
			std::regex reg(one_pattern.substr(2));
			if (std::regex_match(str, reg)) {
				is_match = true;
			}
		}
		else if (strncasecmp(one_pattern.c_str(), "<:", 2) == 0) {
			if (one_pattern.substr(2) < str) {
				is_match = true;
			}
		}
		else if (strncasecmp(one_pattern.c_str(), ">:", 2) == 0) {
			if (one_pattern.substr(2) > str) {
				is_match = true;
			}
		}
		else {
#ifdef WIN
			// todo:
#else
			if (one_pattern == str) {
				is_match = true;
			}
#endif
		}

		// deal negate
		if (neg) {
			if (is_match) {						// !，发生匹配，立即结束
				is_match = false;
				break;
			}
		}
		else {
			if (!is_match) {					// 有一项没有匹配，则要结束
				is_match = false;
				break;
			}
		}

		is_match = true;				// 能走下来，说明匹配过
		if (pos != std::string::npos) {
			begin_pos = 4 + pos;
			pos = pattern.find(" && ", begin_pos);
		}
		else {
			break;
		}
	} while(begin_pos < pattern.length());

	return is_match;
}

/**
 * read all variables first, so they must be defined at the top of file
 *
$php.ini=/etc/php.ini,/var/www/conf/php.ini,/etc/php5/apache2/php.ini;
$web_dirs=/var/www,/var/htdocs,/home/httpd,/usr/local/apache,/usr/local/apache2,/usr/local/www;

# PHP checks
[PHP - Register globals are enabled] [any] [http://www.claw.net/wiki]
f:$php.ini -> r:^register_globals = On;

 */
int Rcl::parse_file_vars(char *out_buf, int buf_size) {
	if (_fp == nullptr) {
		if (!rcl_open) {
			return -1;
		}
	}

	char *nbuf = nullptr;
	char buf[OS_SIZE_1024 + 1];
	while (1) {
		nbuf = get_valid_line(buf, sizeof(buf));
		if (nbuf == nullptr) {
			return -1;					// ？？？？这里真的要使用-1吗？？？
		}

		auto rc_code = get_var(nbuf);
		if (rc_code == 0) {
			break;		// 没有解析到结果，结束解析
		}
		else if (rc_code == -1) {
			error(_server_name.c_str(), "Invalid rk variable: '%s'.", nbuf);
			return -1;
		}
	}

	strncpy(out_buf, nbuf, strlen(nbuf));
	return 0;
}

/**
 * rcl_check_dir
 * rcl的规则为：				d:$web_dirs -> ^application_top.php$ -> r:'osCommerce 2.2-;
 * 经过解析之后的value为：		$web_dirs -> ^application_top.php$ -> r:'osCommerce 2.2-;
 *
 * var对应的配置为：
 * 		$web_dirs=/var/www,/var/htdocs,/home/httpd,/usr/local/apache,/usr/local/apache2,/usr/local/www;
 *
 * return -1			failed
 * 		  0				不匹配
 * 		  1				匹配
 */
int Rcl::check_dir(std::shared_ptr<Rcl::ValueInfo> value) {
	std::string file = get_pattern(value->value());
	if (file.empty()) {
		error(_server_name.c_str(), "Invalid rk variable: '%s'.", value->value().c_str());
		return -1;
	}
	std::string pattern = get_pattern(file);

	std::string f_value;
	if (value->value()[0] == '$') {
		auto found = _var_values.find(value->value());
		if (found != _var_values.end()) {
			f_value = found->second;
		}
		else {
			error(_server_name.c_str(), "Invalid rk variable: '%s'.", value->value().c_str());
			return -1;
		}
	}
	else {
		f_value = value->value();
	}

	auto config = std::static_pointer_cast<RkConfig>(_config);
	auto config_data = config->config_data();

	auto dirs = util::split_str(f_value, ',');
	for (auto one: dirs) {
		debug(_server_name.c_str(), "checking dir: %s; file: %s, pattern: %s", one.c_str(), file.c_str(), pattern.c_str());
		auto is_nfs = util::is_nfs(one);
		if ((is_nfs == 1) && config_data->skip_nfs()) {
			debug(_server_name.c_str(), "rootcheck.skip_nfs enabled and %s is flagged as NFS..", one.c_str());
		}
		else {
			debug(_server_name.c_str(), "%s => is_nfs=%d, skip_nfs=%d.", one.c_str(), is_nfs, config_data->skip_nfs());

			RclCheckFile rk_file(config->run_data(), this);
			if (rk_file.check_dir(one, file, pattern)) {
				debug(_server_name.c_str(), "Found dir. dir: %s, file: %s, pattern: %s", one.c_str(), file.c_str(), pattern.c_str());
				return 1;
			}
		}
	}

	return 0;
}

/**
 * return -1			failed
 * 		  0				不匹配
 * 		  1				匹配
 */
int Rcl::check_file(std::shared_ptr<Rcl::ValueInfo> value) {
	auto config = std::static_pointer_cast<RkConfig>(_config);

	std::string pattern = get_pattern(value->value());

	auto f_value = value->value();
	if (f_value[0] == '$') {
		auto found = _var_values.find(value->value());
		if (found != _var_values.end()) {
			f_value = found->second;
		}
		else {
			error(_server_name.c_str(), "Invalid rk variable: '%s'.", value->value().c_str());
			return -1;
		}
	}
#ifdef WIN
	else if () {
		// todo
	}
#endif

	debug(_server_name.c_str(), "checking file: '%s', pattern: '%s'", f_value.c_str(), pattern.c_str());
	RclCheckFile rk_file(config->run_data(), this);
	if (rk_file.check_file(f_value, pattern)) {
		debug(_server_name.c_str(), "found file..");
		return 1;
	}

	return 0;
}

/**
 * 对于rcl进行处理，
 * 其中，process仅涉及process检查时涉及，仅跟p规则相关
 */
int Rcl::get_entry(const ProcessList& processes) {
	auto config = std::static_pointer_cast<RkConfig>(_config);
	auto run_data = config->run_data();

#ifdef WIN
	// todo:
#endif
	/* read all variables */
	char buf[OS_SIZE_1024 + 1];
	memset(buf, 0, sizeof(buf));
	if (parse_file_vars(buf, sizeof(buf)) == -1) {
		error(_server_name.c_str(), "parse variables failed.");
		return -1;
	}

	/* get first name */
	auto name_info = get_name(buf);
	if (name_info == nullptr) {
		error(_server_name.c_str(), "Invalid rk configuration name: '%s'.", buf);
		return -1;
	}

	/* Get the real entries */
	char *nbuf = nullptr;
	do {
		debug(_server_name.c_str(), "Checking entry: '%s'..", name_info->_name.c_str());
		/* get each value */
		int global_match = 0;
		std::shared_ptr<Rcl::ValueInfo> value_info;
		do {
			nbuf = get_valid_line(buf, sizeof(buf));
			if (nbuf == nullptr) {
				break;
			}
			if (is_name(nbuf)) {
				break;
			}

			/* get value to look for */
			value_info = get_value(nbuf);
			if (value_info == nullptr) {
				error(_server_name.c_str(), "Invalid rk configuration value: '%s'.", nbuf);
				return -1;
			}

			/* check for a file */
			int match = 0;
			if (value_info->type() == RclType::FILE_TYPE) {
				auto ret = check_file(value_info);
				if (ret == -1) {
					continue;
				}
				else if (ret == 1) {
					match = 1;
				}
			}
			else if (value_info->type() == RclType::REGISTRY_TYPE) {
#ifdef WIN
				// todo:
#endif
			}
			else if (value_info->type() == RclType::DIR_TYPE) {
				auto ret = check_dir(value_info);
				if (ret == -1) {
					continue;
				}
				else if (ret == 1) {
					match = 1;
				}
			}
			else if (value_info->type() == RclType::PROCESS_TYPE) {
				debug(_server_name.c_str(), "Checking process: '%s'..", value_info->_value.c_str());
				if (processes.is_process(value_info->value())) {
					debug(_server_name.c_str(), "found process...");
					match = 1;
				}
			}

			/* '!' negate */
			if (value_info->_negate) {
				if (match) {
					match = 0;
				}
				else {
					match = 1;
				}
			}

			/* 设置匹配结论, check the conditions */
			if (name_info->_condition & RclCondition::COND_ANY) {
				debug(_server_name.c_str(), "Condition ANY..");
				if (match) {
					global_match = 1;
				}
			}
			else {
				debug(_server_name.c_str(), "Condition ALL..");
				if (match && (global_match != -1)) {
					global_match = 1;
				}
				else {
					global_match = -1;
				}
			}
		} while(value_info != nullptr);		// 看起来应该是这样

		/* alert msg */
		//  todo:
		if (global_match == 1) {
			int j = 0;
			auto one_alert_msg = run_data->alert_msg()->pop_head();
			while (!one_alert_msg.empty()) {
				std::string op_msg;
				if (!name_info->_reference.empty()) {
					op_msg = _msg + " " + name_info->_name + "." + one_alert_msg + " Reference: " + name_info->_reference + " .";
				}
				else {
					op_msg = _msg + " " + name_info->_name + "." + one_alert_msg;
				}

				if ((value_info->type() == RclType::DIR_TYPE) || (j == 0)) {
					// todo: notify_rk(ALERT_POLICY_VIOLATION, op_msg);
				}
				one_alert_msg = run_data->alert_msg()->pop_head();
			}
		}
		else {
			run_data->alert_msg()->clear_queue();

			if (name_info->_condition & RclCondition::COND_REQ) {
				return 1;
			}
		}

		/* End if we don't have anything else */
		if (!nbuf) {
			return 1;
		}

		/* Get name already read */
		name_info = get_name(nbuf);
		if (!name_info) {
			return 1;
		}
	} while(nbuf != nullptr);		// 这里值得在考虑考虑

	return 1;
}

/*
 *  rcl.h, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年4月11日
 *      Author: yongxue@cyongxue@163.com
 */

#ifndef SRC_SERVER_ROOTCHECK_RCL_H_
#define SRC_SERVER_ROOTCHECK_RCL_H_

#include <stdio.h>
#include <map>

class RclCheckFile {
	private:
		std::shared_ptr<RunData>	_run_data;

		Rcl*		_rcl;

	public:

		RclCheckFile(std::shared_ptr<RunData> run_data, Rcl* rcl): _run_data(run_data), _rcl(rcl) {}

		bool check_dir(const std::string& dir, const std::string& reg_file, const std::string& pattern);
		bool check_file(const std::string& file, const std::string& pattern);
};

class Rcl {
	public:
		class KeyComp {
			public:
				bool operator() (const std::string& lstr, const std::string& rstr) const {
					return lstr < rstr;
				}
		};

		enum RclType {
			INVALID_TYPE = 0,
			FILE_TYPE = 1,
			REGISTRY_TYPE = 2,
			PROCESS_TYPE = 3,
			DIR_TYPE = 4
		};
		class ValueInfo {
			private:
				RclType		_type;
				std::string _value;
				bool 		_negate;				// 取反

			public:
				ValueInfo() {_type = INVALID_TYPE; _negate = false;}

				bool set_type(const std::string& type);
				RclType type() const { return _type; }

				const std::string& value() const { return _value; }
				void set_value(const std::string& value) { _value = value; }

				bool negate() const { return _negate; }
				void set_negate(bool negate) { _negate = negate; }
		};

		enum RclCondition {
			COND_ALL = 0x001,
			COND_ANY = 0x002,
			COND_REQ = 0x004,
			COND_INV = 0x010
		};
		class NameInfo {
			public:
				std::string 	_name;
				std::string		_reference;

				RclCondition	_condition;

			public:
				int set_condition(const std::string& condition);
		};
	private:
		std::shared_ptr<BaseConfig>		_config;

		std::string 		_server_name;
		std::string 		_rcl_file;
		FILE*				_fp;

		std::string			_msg;
		std::map<std::string, std::string, KeyComp>		_var_values;

	public:
		~Rcl() {
			if (_fp) {
				fclose(_fp);
				_fp = nullptr;
			}
		}
		Rcl(const std::string& server_name, const std::string& rcl_file, const std::string& msg) {
			_server_name = server_name;
			_rcl_file = rcl_file;
			_msg = msg;
			_fp = nullptr;
		}

		const std::string& server_name() const { return _server_name; }

		bool rcl_open();

		bool is_all_negate_pattern(const std::string& pattern);
		/* Checks if the specific pattern is present on str. */
		bool pattern_match(const std::string& str, const std::string& pattern);

		int get_entry(const ProcessList& processes);

	private:
		bool is_name(const char * buf);
		char * get_valid_line(char *buf, unsigned int size);

		int get_var(char *nbuf);
		int add_var_value(const std::string& key, const std::string& value);
		std::shared_ptr<Rcl::NameInfo> get_name(char* buf);
		std::shared_ptr<Rcl::ValueInfo> get_value(char *buf);

		std::string get_pattern(const std::string& value);

		int parse_file_vars(char *out_buf, int buf_size);

		int check_dir(std::shared_ptr<Rcl::ValueInfo> value);
		int check_file(std::shared_ptr<Rcl::ValueInfo> value);
};


#endif /* SRC_SERVER_ROOTCHECK_RCL_H_ */

/*
 *  file_flag.cpp, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年3月21日
 *      Author: yongxue@cyongxue@163.com
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/param.h>
#include <sys/mount.h>

#include "file_flag.h"

namespace util {
static std::string FileCheckOpt::get_str_type(int type) {
	std::string ret;

	switch (type) {
		case CHECK_PERM:
			ret = "perm";
			break;
		case CHECK_SIZE:
			ret = "size";
			break;
		case CHECK_OWNER:
			ret = "owner";
			break;
		case CHECK_GROUP:
			ret = "group";
			break;
		case CHECK_MD5SUM:
			ret = "md5sum";
			break;
		case CHECK_SHA1SUM:
			ret = "sha1sum";
			break;
		case CHECK_REALTIME:
			ret = "realtime";
			break;
		case CHECK_SEECHANGES:
			ret = "report_changes";
			break;
		default:
			break;
	}

	return ret;
}


std::string FileCheckOpt::opts_str() const {
	std::string ret;

	if (_opts & CHECK_PERM) {
		ret += "perm";
		ret += "|";
	}

	if (_opts & CHECK_SIZE) {
		ret += "size";
		ret += "|";
	}

	if (_opts & CHECK_OWNER) {
		ret += "owner";
		ret += "|";
	}

	if (_opts & CHECK_GROUP) {
		ret += "group";
		ret += "|";
	}

	if (_opts & CHECK_MD5SUM) {
		ret += "md5sum";
		ret += "|";
	}

	if (_opts & CHECK_SHA1SUM) {
		ret += "sha1sum";
		ret += "|";
	}

	if (_opts & CHECK_REALTIME) {
		ret += "realtime";
		ret += "|";
	}

	if (_opts & CHECK_SEECHANGES) {
		ret += "report_changes";
	}

	return rm_end_head_char(ret, '|');
}

/****************/
const struct file_system_type network_file_systems[] = {
		file_system_type("NFS", 0x6969, 1),
		file_system_type("CIFS", 0xFF534D42, 1),

    /*  The last entry must be name=NULL */
		file_system_type(nullptr, 0, 0)
};

/* List of filesystem to skip the link count test */
const struct file_system_type skip_file_systems[] = {
		file_system_type("BTRFS", 0x9123683E, 1),

	    /*  The last entry must be name=NULL */
		file_system_type(nullptr, 0, 0)
};


/**
 * return  1		是
 * 		   0		不是
 * 		   -1		错误
 */
int is_nfs(const std::string& file_name) {
	struct statfs stfs;

	/* ignore NFS (0x6969) or CIFS (0xFF534D42) mounts */
	if ( !statfs(file_name.c_str(), &stfs) ) {
		int i;
		for ( i = 0; network_file_systems[i]._name != nullptr; i++ ) {
			if(network_file_systems[i]._type == stfs.f_type ) {
				return network_file_systems[i]._flag;
			}
		}
		return(0);
	}
	else {
		/* If the file exists, throw an error and retreat! If the file does not exist, there
		 * is no reason to spam the log with these errors. */
		return(-1);
	}
}

/**
 * return 		1		是
 * 			    0		不是
 * 			    -1		失败
 */
int skip_fs(const std::string& file_name) {
	struct statfs stfs;

	if ( !statfs(file_name.c_str(), &stfs) )
	{
		int i;
		for ( i=0; skip_file_systems[i]._name != NULL; i++ ) {
			if(skip_file_systems[i]._type == stfs.f_type ) {
				return skip_file_systems[i]._flag;
			}
		}
		return(0);
	}
	else
	{
		/* If the file exists, throw an error and retreat! If the file does not exist, there
		 * is no reason to spam the log with these errors. */
		return(-1);
	}
}

}


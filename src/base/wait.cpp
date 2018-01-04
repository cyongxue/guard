/*
 *  wait.cpp, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年5月18日
 *      Author: yongxue@cyongxue@163.com
 */

#include "chroot.h"
#include "log.h"

#include "wait.h"

LockWithFile* LockWithFile::_instance = nullptr;

static LockWithFile* LockWithFile::instance() {
	if (_instance == nullptr) {
		_instance = new LockWithFile();
	}

	return _instance;
}

void LockWithFile::set_wait() {
	_wait_lock = true;

	/* when lock, so create file. */
	std::string wait_file_path = Chroot::instance()->full_file_path(LOCK_WAIT_FILE);
	FILE *fp = fopen(wait_file_path.c_str(), "w");
	if (fp) {
		fprintf(fp, "l");
		fclose(fp);
	}

	return;
}

void LockWithFile::del_wait() {
	_wait_lock = false;

	/* free lock, and delete file */
	std::string wait_file_path = Chroot::instance()->full_file_path(LOCK_WAIT_FILE);
	unlink(wait_file_path.c_str());

	return;
}

#ifdef WIN
void LockWithFile::wait() {
	if (!_wait_lock) {
		return;
	}

	info(_module.c_str(), "Process locked. Waiting for permission...");
	while (1) {
		if (!_wait_lock) {
			break;
		}

		sleep(LockWithFile::LOCK_LOOP);
	}
	info(_module.c_str(), "Lock free. Continuing...");

	return;
}
#else
void LockWithFile::wait() {
	std::string wait_file_path = Chroot::instance()->full_file_path(LOCK_WAIT_FILE);

	/* lock file no exist, so lock no lock. */
	struct stat file_status;
	if (stat(wait_file_path.c_str(), &file_status) == -1) {
		error(_module.c_str(), "no lock, wait file[%s] stat failed.", wait_file_path.c_str());
		return;
	}

	info(_module.c_str(), "Process locked. Waiting for permission...");
	while (1) {
		if (stat(wait_file_path.c_str(), &file_status) == -1) {
			break;
		}

		/* Sleep LOCK_LOOP seconds and check if lock is gone */
		sleep(LockWithFile::LOCK_LOOP);
	}
	info(_module.c_str(), "Lock free. Continuing...");

	return;
}
#endif 			/* !WIN */

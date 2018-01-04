/*
 *  wait.h, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年5月18日
 *      Author: yongxue@cyongxue@163.com
 */

#ifndef SRC_BASE_WAIT_H_
#define SRC_BASE_WAIT_H_

/**
 * 单例模式
 */
class LockWithFile {
	public:
		const int LOCK_LOOP = 5;
#ifndef WIN
#define LOCK_WAIT_FILE				"/queue/guard/.wait"
#else
#define LOCK_WAIT_FILE				".wait"
#endif

	private:
		std::string 	_module;
		bool			_wait_lock;

		static LockWithFile* _instance;

		LockWithFile() {
			_module = "LockWithFile";
			_wait_lock = false;
		}

	public:
		static LockWithFile* instance();

		/* create global lock */
		void set_wait();
		/* remove global lock */
		void del_wait();
		/* check for the wait file. if present, wait.
		 * works as a simple inter process lock (only the main process is allowed to lock) */
		void wait();
};

#endif /* SRC_BASE_WAIT_H_ */

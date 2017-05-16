/*
 *  defs.h, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年3月2日
 *      Author: yongxue@cyongxue@163.com
 */

#ifndef SRC_HEADERS_DEFS_H_
#define SRC_HEADERS_DEFS_H_

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>

#include "test_defs.h"

#define OS_INVALID		-1

/* Some global names */
#define __guard_name    "GUARD HIDS"
#define __version       "v2.9.0"
#define __author        "Trend Micro Inc."
#define __contact       "contact@guard.net"
#define __site          "http://www.guard.net"
#define __license       "\
This program is free software; you can redistribute it and/or modify\n\
it under the terms of the GNU General Public License (version 2) as \n\
published by the Free Software Foundation. For more details, go to \n\
http://www.guard.net/main/license/\n"

#define GUARDCONF       "/etc/guard.conf"
#define GUARD_DEFINES   "/etc/internal_options.conf"
#define AGENT_CONF		"/ect/share/agent.conf"

#define OS_PIDFILE		"/var/run"

#define GUARD_INFO_FILE		"/queue/guard/.agent_info"
#define UNIX_QUEUE			"/queue/guard/queue"

#define OS_SIZE_1024	1024
#define OS_SIZE_2048	2048

/* Default to 10 hours */
#define ROOTCHECK_WAIT          72000

/* Diff queue */
#define DIFF_DIR        "/queue/diff"
#define DIFF_NEW_FILE  "new-entry"
#define DIFF_LAST_FILE "last-entry"

#define TMP_DIR			"/tmp"

#endif /* SRC_HEADERS_DEFS_H_ */

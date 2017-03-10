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

/* Some global names */
#define __claw_name    "CLAW HIDS"
#define __version       "v2.9.0"
#define __author        "Trend Micro Inc."
#define __contact       "contact@claw.net"
#define __site          "http://www.claw.net"
#define __license       "\
This program is free software; you can redistribute it and/or modify\n\
it under the terms of the GNU General Public License (version 2) as \n\
published by the Free Software Foundation. For more details, go to \n\
http://www.claw.net/main/license/\n"

#define CLAWCONF       "/etc/claw.conf"
#define CLAW_DEFINES   "/etc/internal_options.conf"

#define OS_PIDFILE		"/var/run"

#define OS_SIZE_1024	1024

#endif /* SRC_HEADERS_DEFS_H_ */

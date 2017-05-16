/*
 *  sha1_op.h, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年3月24日
 *      Author: yongxue@cyongxue@163.com
 */

#ifndef SRC_SEC_SHA1_OP_H_
#define SRC_SEC_SHA1_OP_H_

namespace sec {

int sha1_file(const std::string& file_name, int mode, std::string& sha1);
int sha1_cmd(const std::string& cmd, std::string& sha1);

}

#endif /* SRC_SEC_SHA1_OP_H_ */

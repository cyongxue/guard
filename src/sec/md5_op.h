/*
 *  md5_op.h, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年3月24日
 *      Author: yongxue@cyongxue@163.com
 */

#ifndef SRC_SEC_MD5_OP_H_
#define SRC_SEC_MD5_OP_H_

namespace sec {

int md5_str(const std::string& src, std::string& dest);
int md5_file(const std::string& file_name, int mode, std::string& dest);

int md5_cmd(const std::string& cmd, std::string& dest);

}

#endif /* SRC_SEC_MD5_OP_H_ */

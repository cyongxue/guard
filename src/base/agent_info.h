/*
 *  agent_info.h, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年3月18日
 *      Author: yongxue@cyongxue@163.com
 */

#ifndef SRC_BASE_AGENT_INFO_H_
#define SRC_BASE_AGENT_INFO_H_

class AgentInfo {
	private:
		std::string 	_file_name;

	public:
		AgentInfo(std::string file_name): _file_name(file_name) {}
		~AgentInfo() = default;

		std::string agent_guard_id() const;
		std::string agent_profile() const;

		/* Write the agent info inside the queue, for the other processes to read
		 * Returns 1 		on success or
		 *         <= 0 	on failure
		 */
		int write_agent_info(const std::string& wol_id, const std::string& profile);
};



#endif /* SRC_BASE_AGENT_INFO_H_ */

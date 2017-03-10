/*
 *  test.cpp, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年3月9日
 *      Author: yongxue@cyongxue@163.com
 */

#include "xml.h"

using namespace xml;

int main(int argc, char **argv) {
	if (argc < 2) {
		std::cout << "Usage: " << argv[0] << "file" << std::endl;
		return -1;
	}

	xml::Xml xml_instance(argv[1]);
	if (xml_instance.read_xml() < 2) {
		error(xml::module.c_str(), "OS_ReadXML error: %s, line :%d", xml_instance.err().c_str(), xml_instance.err_line());
		return 1;
	}

	if (xml_instance.apply_variables() != 0) {
		error(xml::module.c_str(), "OS_ReadXML error: Applying variables: %s\n", xml_instance.err().c_str());
		return 1;
	}

	xml::Node xml_node;
	auto nodes = xml_instance.get_element_by_node(xml_node);
	if (nodes.size() == 0) {
		error(xml::module.c_str(), "OS_GetElementsbyNode error: %s, line: %d\n", xml_instance.err().c_str(), xml_instance.err_line());
		return 1;
	}

	for (auto it = nodes.begin(); it != nodes.end(); ) {
		auto cnodes = xml_instance.get_element_by_node(*it);
		if (cnodes.size() == 0) {
			it++;
			continue;
		}

		for (auto it2 = cnodes.begin(); it2 != cnodes.end();) {
			info(xml::module.c_str(), "Element: %s -> %s", it2->_element.c_str(), it2->_content.c_str());
			if ((it2->_attributes.size() != 0) && (it2->_values.size() != 0)) {
				int k = 0;
				while (k < it2->_attributes.size()) {
					info(xml::module.c_str(), "attr %s:%s", it2->_attributes[k].c_str(), it2->_values[k].c_str());
					k++;
				}
			}
			it2++;
		}
		it++;
	}

	return 0;
}



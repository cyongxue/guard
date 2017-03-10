/*
 *  xml.h, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年3月6日
 *      Author: yongxue@cyongxue@163.com
 */

#ifndef SRC_XML_XML_H_
#define SRC_XML_XML_H_

#include <vector>

#include "log.h"

namespace xml {

const std::string module = "XML";

enum class XmlType {
	XML_ATTR,
	XML_ELEM,
	XML_VARIABLE_BEGIN = '$'
};

#define XML_CONFS    '<'
#define XML_CONFE    '>'
#define XML_COM      '!'

#define XML_VAR              "var"
#define XML_VAR_ATTRIBUTE    "name"

struct Node {
	unsigned int _key;
	std::string  _element;
	std::string  _content;
	std::vector<std::string>  _attributes;
	std::vector<std::string>  _values;
};

/* xml节点 */
class XmlNode {
	public:
		uint32_t	_relation;
		XmlType		_xml_type;
		std::string _name;
		std::string	_contents;

		int32_t		_ck;
		uint32_t	_line;
};

/* 整个xml的内容 */
class Xml {
	private:
		uint32_t		_curr;		/* Current position (and last after reading) */
		int32_t			_foll;		/* Current position for the xml_access */
		std::vector<XmlNode>	_xml_nodes;

		uint32_t 				_line;					// 记录处理的line值
		uint32_t 	  			_err_line;
		std::string				_err;

		std::string 	_file_name;
		FILE* 			_fp;
		bool			_is_open;

	private:
		int xml_fgetc();
		int is_comment();

		int write_element(const std::string elem, XmlType type, unsigned int parent);
		int write_content(const std::string content, unsigned int parent);
		int get_attributes(unsigned int parent);

		int read_element(unsigned int parent);

		std::vector<std::string> get_elements_internal(std::vector<std::string> element_names, XmlType type) const;
		std::vector<std::string> get_content_internal(std::vector<std::string> element_names, std::string attr) const;

	public:
		Xml(std::string file_name);
		~Xml();

		std::string err() const {
			return _err;
		}
		uint32_t err_line() const {
			return _err_line;
		}

		bool is_open() const;
		int read_xml();

		/* 从解析的xml中读取结果 */
		uint32_t element_exist(std::vector<std::string> element_names) const;
		uint32_t root_element_exist(std::string element_name) const;

		std::string get_attribute_content(std::vector<std::string> element_names, std::string attribute_name) const;
		std::vector<std::string> get_contents(std::vector<std::string> element_names) const;
		std::vector<std::string> get_element_contents(std::vector<std::string> element_names) const;
		std::string get_one_content_for_element(std::vector<std::string> element_names) const;

		std::vector<std::string> get_elements(std::vector<std::string> element_names) const;
		std::vector<std::string> get_attributes_for_elements(std::vector<std::string> element_names) const;

		/* Get the elements by node */
		std::vector<Node> get_element_by_node(const Node& node) const;

		int apply_variables();
};
}



#endif /* SRC_XML_XML_H_ */

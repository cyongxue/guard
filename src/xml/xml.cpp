/*
 *  xml.cpp, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年3月6日
 *      Author: yongxue@cyongxue@163.com
 */

#include "xml.h"

namespace xml {

/* 构造 */
Xml::Xml(std::string file_name) {
	_file_name = file_name;
	_fp = fopen(_file_name.c_str(), "r");
	if (_fp) {
		_is_open = true;
	}
	else {
		error(module.c_str(), "File '%s' not found.", file_name.c_str());
		_is_open = false;
	}

	_curr = 0;
	_foll = 0;

	_err_line = 0;
	_line = 0;
}

/* 析构 */
Xml::~Xml() {
	if (_fp) {
		fclose(_fp);
	}
}

bool Xml::is_open() const {
	return _is_open;
}

int Xml::read_xml() {

	/* 检查文件打开 */
	if (!is_open()) {
		error(module.c_str(), "File '%s' not found.", _file_name.c_str());
		return -2;
	}

	/* 递归遍历元素 */
	int r;
	if ((r = read_element(0)) < 0) {
		if (r != -2) {
			error(module.c_str(), "Read element error.");
			return -1;
		}
	}

	/* 处理 */
	for (auto i = 0; i < _curr; i++) {
		if (_xml_nodes[i]._ck == 0) {
			error(module.c_str(), "Element '%s' not closed.", _xml_nodes[i]._name.c_str());
			return -1;
		}
	}

	return 0;
}

/* 写element到vector中 */
int Xml::write_element(const std::string elem, XmlType type, unsigned int parent) {

	XmlNode new_node;
	new_node._name = elem;
	new_node._xml_type = type;
	new_node._relation = parent;
	new_node._ck = 0;
	new_node._line = _line;

	if (type == XmlType::XML_ATTR) {
		new_node._ck = 1;
	}

	if (elem == XML_VAR) {
		new_node._xml_type = XmlType::XML_VARIABLE_BEGIN;
	}

	_xml_nodes.push_back(new_node);
	_curr++;
	return 0;
}

/* 写content到vector中
 * return 0			成功
 * 		  -1		失败
 * */
int Xml::write_content(const std::string content, unsigned int parent) {
	if (_xml_nodes.max_size() <= parent) {
		error(module.c_str(), "parent > xml_nodes index.");
		return -1;
	}
	_xml_nodes[parent]._content = content;
	return 0;
}

int Xml::get_attributes(unsigned int parent) {
	int c;
	int location = 0;
	unsigned int count = 0;
	int c_to_match = 0;

	std::string attr;
	std::string value;

	while ((c = xml_fgetc()) != EOF) {
		if ((c == XML_CONFE) || ((location == 0) && (c == '/'))) {
			if (location == 1) {
				error(module.c_str(), "Attribute '%s' not closed.", attr.c_str());
				return (-1);
			} else if ((location == 0) && (count > 0)) {
				error(module.c_str(), "Attribute '%s' has no value.", attr.c_str());
				return (-1);
			} else if (c == '/') {
				return (c);
			} else {
				return (0);
			}
		} else if ((location == 0) && (c == '=')) {
			/* Check for existing attribute with same name */
			unsigned int i = _curr - 1;
			/* Search attributes backwards in same parent */
			while (_xml_nodes[i]._relation == parent && _xml_nodes[i]._xml_type == XmlType::XML_ATTR) {
				if (_xml_nodes[i]._name == attr) {
					error(module.c_str(), "Attribute '%s' already defined.", attr.c_str());
					return (-1);
				}

				/* Continue with previous element */
				if (i == 0) {
					break;
				}
				i--;
			}

			c = xml_fgetc();
			if ((c != '"') && (c != '\'')) {
				unsigned short int _err = 1;
				if (isspace(c)) {
					while ((c = xml_fgetc()) != EOF) {
						if (isspace(c)) {
							continue;
						} else if ((c == '"') || (c == '\'')) {
							_err = 0;
							break;
						} else {
							break;
						}
					}
				}
				if (_err != 0) {
					error(module.c_str(), "Attribute '%s' not followed by a \" or \'.", attr.c_str());
					return (-1);
				}
			}

			c_to_match = c;
			location = 1;
			count = 0;
		} else if ((location == 0) && (isspace(c))) {
			if (count == 0) {
				continue;
			} else {
				error(module.c_str(), "Attribute '%s' has no value.", attr.c_str());
				return (-1);
			}
		} else if ((location == 1) && (c == c_to_match)) {
			if (write_element(attr, XmlType::XML_ATTR, parent) < 0) {
				return (-1);
			}
			if (write_content(value, _curr - 1) < 0) {
				return (-1);
			}
			c = xml_fgetc();
			if (isspace(c)) {
				return (get_attributes(parent));
			} else if (c == XML_CONFE) {
				return (0);
			} else if (c == '/') {
				return (c);
			}

			error(module.c_str(), "Bad attribute closing for '%s'='%s'.", attr.c_str(), value.c_str());
			return (-1);
		} else if (location == 0) {
			attr.push_back((char)c);
		} else if (location == 1) {
			value.push_back((char)c);
		}
	}

	error(module.c_str(), "End of file while reading an attribute.");
	return -1;
}

/**
 * return -2		文件结尾
 *        -1		解析错误
 */
int Xml::read_element(unsigned int parent) {

	int c;
	int prevv = 0;

	unsigned int currentlycont = 0;
	unsigned int count = 0;
	short int location = -1;

	std::string elem;
	std::string content;
	std::string closed_elem;

	while ((c = xml_fgetc()) != EOF) {
		if (c == '\\') {
			prevv = c;
		}
		else if (prevv == '\\') {
			if (c != XML_CONFS) {
				prevv = 0;
			}
		}

		/* check for comments，检查注释 */
		if (c == XML_CONFS) {
			auto r = is_comment();
			if (r < 0) {
				error(module.c_str(), "Comment not closed.");
				return -1;
			}
			else if (r == 1) {
				continue;		// 注释就不处理
			}
		}

		/* real checking */
		if ((location == -1) && (prevv == 0)) {
			if (c == XML_CONFS) {
				if ((c = fgetc(_fp)) == '/') {
					error(module.c_str(), "Element not opened.");
					return -1;
				}
				else {
					ungetc(c, _fp);
				}
				location = 0;
			}
			else {
				continue;
			}
		}
		else if ((location == 0) && ((c == XML_CONFE) || isspace(c))) {
			int _ge = 0;
			int _ga = 0;

			/* Remove the / at the end of the element name */
			if (count > 0 && elem[count - 1] == '/') {
				_ge = '/';
				elem.pop_back();
			}

			if (write_element(elem, XmlType::XML_ELEM, parent) < 0) {
				return (-1);
			}
			currentlycont = _curr - 1;			// 记录当前层级
			if (isspace(c)) {
				if ((_ga = get_attributes(parent)) < 0) {				// 取Attributes
					return (-1);
				}
			}

			/* If the element is closed already (finished in />) */
			if ((_ge == '/') || (_ga == '/')) {
				if (write_content("\0", currentlycont) < 0) {
					return (-1);
				}
				_xml_nodes[currentlycont]._ck = 1;						// 解析到一个

				elem = "";
				closed_elem = "";
				content = "";
				currentlycont = 0;
				count = 0;
				location = -1;
				if (parent > 0) {
					return (0);
				}
			} else {
				count = 0;
				location = 1;
			}
		}
		else if ((location == 2) && (c == XML_CONFE)) {
			if (closed_elem != elem) {
				error(module.c_str(), "Element '%s' not closed.", elem.c_str());
				return (-1);
			}
			if (write_content(content, currentlycont) < 0) {
				return (-1);
			}
			_xml_nodes[currentlycont]._ck = 1;					// 解析得到一个结果

			elem = "";
			closed_elem = "";
			content = "";
			currentlycont = 0;
			count = 0;
			location = -1;
			if (parent > 0) {
				return (0);
			}
		}
		else if ((location == 1) && (c == XML_CONFS) && (prevv == 0)) {
			if ((c = fgetc(_fp)) == '/') {
				count = 0;
				location = 2;
			} else {
				ungetc(c, _fp);
				ungetc(XML_CONFS, _fp);

				if (read_element(parent + 1) < 0) {
					return (-1);
				}
				count = 0;
			}
		}
		else {
			if (location == 0) {
				elem.push_back((char)c);
			} else if (location == 1) {
				content.push_back((char) c);
			} else if (location == 2) {
				closed_elem.push_back((char) c);
			}

			if ((XML_CONFS == c) && (prevv != 0)) {
				prevv = 0;
			}
		}
	}

	if (location == -1) {
		return -2;
	}

	error(module.c_str(), "End of file and some elements were not closed.");
	return -1;
}

int Xml::xml_fgetc() {
	auto c = fgetc(_fp);
	if (c == '\n') {
		_line++;
	}

	return c;
}

/**
 * return 0		非注释		需恢复流
 * 		  -1	有错			不用恢复
 * 		  1		注释			不用恢复
 */
int Xml::is_comment() {

	int c;

	if ((c = fgetc(_fp)) == XML_COM) {
		// !可能是注释
		while ((c = fgetc(_fp)) != EOF) {
			if (c == XML_COM) {
				if ((c = fgetc(_fp)) == XML_CONFE) {
					return 1;
				}
				ungetc(c, _fp);
			}
			else if (c == '-') {
				if ((c = xml_fgetc()) == '-') {
					if ((c = fgetc(_fp)) == XML_CONFE) {
						return 1;
					}
					ungetc(c, _fp);
				}
				ungetc(c, _fp);
			}
			else {
				continue;
			}
		}
		return -1;
	}
	else {
		ungetc(c, _fp);
	}

	return 0;
}

std::vector<std::string> Xml::get_elements_internal(std::vector<std::string> element_names, XmlType type) const {

	uint32_t ready = 0;
	uint32_t matched = 0;
	std::vector<std::string> ret;

	if ((type == XmlType::XML_ELEM) && (element_names.size() == 0)) {
		ready = 1;
	}

	int j = 0;
	for (auto it = _xml_nodes.begin(); it != _xml_nodes.end(); it++) {

		if (j >= element_names.size()) {
			return ret;
		}

		if ((ready != 1) && (element_names[j].empty())) {
			if (matched == 1) {
				ready = 1;
			}
			else {
				break;
			}
		}

		if ((ready == 1) && (it->_xml_type == type)) {
			if (((type == XmlType::XML_ATTR) && (it->_relation == j - 1) && (!it->_name.empty())) ||
					((type == XmlType::XML_ELEM) && (it->_relation == j) && (!it->_name.empty()))) {
				ret.push_back(it->_name);
			}
		}
		else if ((it->_xml_type == XmlType::XML_ELEM) && (it->_relation == j) &&
				(!element_names[j].empty())) {
			if (it->_name == element_names[j]) {
				j++;
				matched = 1;
				continue;
			}
		}

		if (matched == 1) {
			if (((it->_xml_type == XmlType::XML_ATTR) && (j > it->_relation + 1)) ||
					((it->_xml_type == XmlType::XML_ELEM) && (j > it->_relation))) {
				j = 0;
				matched = 0;
				if (element_names.size() == 0) {
					ready = 1;
				}
				else {
					ready = 0;
				}
			}
		}
	}

	return ret;
}

std::vector<std::string> Xml::get_content_internal(std::vector<std::string> element_names, std::string attr) {
	int i = 0;
	unsigned int j = 0, l = 0, matched = 0;
	std::vector<std::string> ret;

	if (_foll >= 0 && _foll == _curr) {
		_foll = 0;
		return ret;
	}

	if (_foll > 0) {
		for (i = _foll; i >= 0; i--) {
			_foll = i;
			if (_xml_nodes[i]._relation == 0) {
				break;
			}
		}
		i = _foll;
	}
	else {
		i = 0;
	}

	/* Loop over all nodes */
	for (j = 0, l = (unsigned int)i; l < _curr; l++) {

		if (j >= element_names.size()) {
			ret.clear();
			return ret;
		}

		if (element_names[j].empty()) {
			if (matched != 1) {
				break;
			}
		}

		/* If the type is not an element and the relation doesn't match, keep going
		 */
		if ((_xml_nodes[l]._xml_type != XmlType::XML_ELEM) || (_xml_nodes[l]._relation != j)) {
			/* If the node relation is higher than the current xml
			 * node, zero the position and look at it again (i--).
			 */
			if (j > _xml_nodes[l]._relation) {
				j = 0;
				matched = 0;
				l--;
			} else {
				continue;
			}
		}
		/* If the element name matches what we are looking for */
		else if (!element_names[j].empty() && _xml_nodes[l]._name == element_names[j]) {
			j++;
			matched = 1;

			/* Get content if we are at the end of the array */
			if (element_names[j].empty()) {
				/* If we have an attribute to match */
				if (!attr.empty()) {
					unsigned int m = 0;
					for (m = l + 1; m < _curr; m++) {
						if (_xml_nodes[m]._xml_type == XmlType::XML_ELEM) {
							break;
						}
						if (attr == _xml_nodes[m]._name) {
							l = m;
							break;
						}
					}
				}

				if (!_xml_nodes[l]._content.empty()) {
					/* Increase the size of the array */
					ret.push_back(_xml_nodes[l]._content);
					matched = 1;
					if (!attr.empty()) {
						break;
					}
					else if (_foll != 0) {
						_foll = (int) l + 1;
						break;
					}
				}

				/* Set new array pointer */
				if ((l < _curr - 1) && (_xml_nodes[l + 1]._xml_type == XmlType::XML_ELEM)) {
					j = _xml_nodes[l + 1]._relation;
				}
			}
			continue;
		}

		if (j > _xml_nodes[l]._relation) {
			j = 0;
			matched = 0;
		}
	}

	return ret;
}

/* Get one value for a specific attribute */
std::string Xml::get_attribute_content(std::vector<std::string> element_names, std::string attribute_name) {

	auto result = get_content_internal(element_names, attribute_name);
	if (result.size() == 0) {
		return "";
	}
	if (!result[0].empty()) {
		return result[0];
	}
	return "";
}

/* Get the contents for a specific element
 * Use element_name = NULL to start the state
 */
std::vector<std::string> Xml::get_contents(std::vector<std::string> element_names) {
	std::vector<std::string> ret;
	if (element_names.size() == 0) {
		_foll = -1;
		return ret;
	}
	return get_content_internal(element_names, "");
}

/* Get all values for a specific element */
std::vector<std::string> Xml::get_element_contents(std::vector<std::string> element_names) {
	_foll = 0;
	return get_content_internal(element_names, "");
}

/* Get one value for a specific element */
std::string Xml::get_one_content_for_element(std::vector<std::string> element_names) {
	_foll = 0;
	auto result = get_content_internal(element_names, "");
	if (result.size() == 0) {
		return "";
	}

	if (!result[0].empty()) {
		return result[0];
	}
	return "";
}

/* Get the elements children of the element_name */
std::vector<std::string> Xml::get_elements(std::vector<std::string> element_names) const {
	return get_elements_internal(element_names, XmlType::XML_ELEM);
}

/* Get the attributes of the element_name */
std::vector<std::string> Xml::get_attributes_for_elements(std::vector<std::string> element_names) const {
	return get_elements_internal(element_names, XmlType::XML_ATTR);
}

/* Check if a element exists
 * The element_name must be NULL terminated (last char)
 */
uint32_t Xml::element_exist(std::vector<std::string> element_names) const {
	if (element_names.size() == 0) {
		return 0;
	}

	unsigned int j = 0, matched = 0, totalmatch = 0;
	for (auto it = _xml_nodes.begin(); it != _xml_nodes.end(); it++) {
		if (j >= element_names.size()) {
			j = 0;
		}

		if ((it->_xml_type == XmlType::XML_ELEM) && (it->_relation == j)) {
			if (element_names[j] == it->_name) {
				j++;
				matched = 1;
				if (j >= element_names.size()) {
					j = 0;
					totalmatch++;
				}
				continue;
			}
		}
		if ((matched == 1) && (j > it->_relation) && (it->_xml_type == XmlType::XML_ELEM)) {
			j = 0;
			matched = 0;
		}
	}

	return totalmatch;
}

/* Check if a root element exists */
uint32_t Xml::root_element_exist(std::string element_name) const {
	std::vector<std::string> element_names;
	element_names.push_back(element_name);
	return element_exist(element_names);
}

/* Get the elements by node */
std::vector<Node> Xml::get_element_by_node(const Node& node) const {

	std::vector<Node> ret;
	uint32_t i, m;				// m记录当前处理的层级
	if (node._key == NODE_INVALID) {
		i = 0;
		m = 0;
	}
	else {
		i = node._key;
		m = _xml_nodes[i++]._relation + 1;
	}

	for (; i < _curr; i++) {
		if (_xml_nodes[i]._xml_type == XmlType::XML_ELEM) {
			if ((_xml_nodes[i]._relation == m) && (!_xml_nodes[i]._name.empty())) {
				auto l = i + 1;			// 如果i的index为element，那么下一个很可能就是attribute

				Node one_node;
				one_node._element = _xml_nodes[i]._name;
				one_node._content = _xml_nodes[i]._content;
				one_node._key = i;

				while (l < _curr) {
					if ((_xml_nodes[l]._xml_type == XmlType::XML_ATTR) &&
							(_xml_nodes[l]._relation == m) &&
							(!_xml_nodes[l]._name.empty()) && (!_xml_nodes[l]._content.empty())) {
						one_node._attributes.push_back(_xml_nodes[l]._name);
						one_node._values.push_back(_xml_nodes[l]._content);

						l++;
					}
					else {
						break;
					}
				}
				ret.push_back(one_node);
				continue;
			}
		}

		if ((_xml_nodes[i]._xml_type == XmlType::XML_ELEM) && (m > _xml_nodes[i]._relation)) {
			if (node._key == NODE_INVALID) {
				continue;
			}
			else {
				break;
			}
		}
	}

	return ret;
}

int Xml::apply_variables() {

	unsigned int j =0, s = 0;

	std::vector<std::string> var;
	std::vector<std::string> value;
	std::string var_placeh;

	for (auto i = 0; i < _curr; i++) {
		if (_xml_nodes[i]._xml_type == XmlType::XML_VARIABLE_BEGIN) {
			int _found_var = 0;

			for (j = i + 1; j < _curr; j++) {

                if (_xml_nodes[j]._relation < _xml_nodes[i]._relation) {
                    break;
                }
                else if (_xml_nodes[j]._xml_type == XmlType::XML_ATTR) {
                    if (_xml_nodes[j]._name == XML_VAR_ATTRIBUTE) {
                        if (_xml_nodes[j]._content.empty()) {
                        	_err = "Invalid variable content.";
                        	_err_line = _xml_nodes[j]._line;
                        	return -1;
                        }
                        /* 长度检查 */

                        /* If not used, it will be cleaned later */
                        var.push_back(_xml_nodes[j]._content);
                        _err = "";

                        _found_var = 1;
                        break;
                    } else {
                    	_err.append("Only \"").append(XML_VAR_ATTRIBUTE).append("\" is allowed as an attribute for a variable.");
                        _err_line = _xml_nodes[j]._line;
                        return -1;
                    }
                }
			}

			if ((_found_var == 0) || (_xml_nodes[i]._content.empty())) {
				_err = "No value set for variable.";
				return -1;
			}

			value.push_back(_xml_nodes[i]._content);
			_err = "";
			s++;
		}
		else if (((_xml_nodes[i]._xml_type == XmlType::XML_ELEM) || (_xml_nodes[i]._xml_type == XmlType::XML_ATTR)) &&
				(!_xml_nodes[i]._content.empty())) {

            unsigned int tp = 0;
            size_t init = 0;
            std::string tmp_content;
            std::string lvar;

            if (_xml_nodes[i]._content.size() <= 2) {
                continue;
            }

            /* Check if any variable is defined */
            if (s == 0) {
                continue;
            }

            tmp_content = _xml_nodes[i]._content;

            /* Read the whole string */
            int index = 0;
            while (index < tmp_content.length()) {
                if (tmp_content[index] == (char)XmlType::XML_VARIABLE_BEGIN) {
                    tp = 0;
                    index++;

                    while (1) {
                        if ((tmp_content[index] == (char)XmlType::XML_VARIABLE_BEGIN)
                                || (tmp_content[index] == '\0')
                                || (tmp_content[index] == '.')
                                || (tmp_content[index] == '|')
                                || (tmp_content[index] == ',')
                                || (tmp_content[index] == ' ')) {

                            /* Look for var */
                            for (j = 0; j < s; j++) {
                                if (var[j].empty()) {
                                    break;
                                }

                                if (var[j] == lvar) {
                                    continue;
                                }

                                size_t tsize = _xml_nodes[i]._content.size() + value[j].length() - tp + 1;
                                var_placeh = _xml_nodes[i]._content;
                                _xml_nodes[i]._content = "";

                                _xml_nodes[i]._content.insert(0, var_placeh, 0, tsize);
                                _xml_nodes[i]._content.insert(init, value[j], 0, tsize - init);

                                init = _xml_nodes[i]._content.size();
                                _xml_nodes[i]._content.insert(0, tmp_content.substr(index), 0, tsize - init);
                                var_placeh = "";
                                break;
                            }

                            /* Variable not found */
                            if ((j == s) && (lvar.length() >= 1)) {
                            	_err = "Unknown variable: " + lvar;
                                _err_line = _xml_nodes[i]._line;
                                return -1;
                            } else if (j == s) {
                                init++;
                            }

                            goto go_next;
                        }

                        lvar[tp] = tmp_content[index];
                        tp++;
                        index++;
                    }
                } /* IF XML_VAR_BEGIN */

                index++;
                init++;
go_next:
                continue;
            } /* WHILE END */
		}
	}

	return 0;
}


void Xml::print_all() {
	info(module.c_str(), "**************** print all begin *******************");
	for (auto it = _xml_nodes.begin(); it != _xml_nodes.end(); it++) {
		std::string xml_type;
		if (it->_xml_type == XmlType::XML_ATTR) {
			xml_type = "attr";
		}
		else if (it->_xml_type == XmlType::XML_ELEM) {
			xml_type = "elem";
		}
		info(module.c_str(), "%d, %s, %s, %s", it->_relation, xml_type.c_str(), it->_name.c_str(), it->_content.c_str());
	}
	info(module.c_str(), "**************** print all end *******************");
	return;
}

}



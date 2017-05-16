/*
 *  test.cpp, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年3月22日
 *      Author: yongxue@cyongxue@163.com
 */


#include<string>
#include<iostream>
int main()
{
    const std::string path="/root/config/";
    auto const pos=path.find_last_of('/');
    const auto leaf=path.substr(pos+1);

    std::cout << path.length() << std::endl;
    std::cout << pos << std::endl;
    std::cout << leaf << '\n';

    return 0;
}

#pragma once
#include<iostream>
#include<string.h>
#include<string>
#include<stdlib.h>
#include<mysql.h>
MYSQL* initmysql();
int connectmysql(MYSQL*myfd);
int insertmysql(MYSQL*myfd,const std::string&name,const std::string&school,const std::string&hobby);
void selectmysql(MYSQL*myfd,std::string&str);
void closemysql(MYSQL* myfd);
//VALUES("zhangsan","man","coding") C++中的字符串不以'\0'标注结尾


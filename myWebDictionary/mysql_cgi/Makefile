CURR_PATH=$(shell pwd)
LIB=$(CURR_PATH)/../lib
INCLUDE=-I$(LIB)/include#-I添加头文件的搜索路径
MYSQL_LIB=-L$(LIB)/lib #-L搜索指定路径下的库
LD_FLAGS=-lmysqlclient#链接到动态库
.PHONY:all
all:insert_cgi  search_cgi
insert_cgi:insert_cgi.cpp comm.cpp
	g++ -o $@ $^ $(INCLUDE) $(MYSQL_LIB) $(LD_FLAGS)

#select_cgi:select_cgi.cpp comm.cpp
#	g++ -o $@ $^ $(INCLUDE) $(MYSQL_LIB) $(LD_FLAGS)

search_cgi:search_cgi.cpp comm.cpp
	g++ -o $@ $^ $(INCLUDE) $(MYSQL_LIB) $(LD_FLAGS)
.PHONY:clean
clean:
	rm -rf insert_cgi search_cgi
#.PHONY:output
#output:
#	mkdir -p output/wwwroot
#	mkdir -p output/lib
#	cp httpd output
#	cp -rf lib/lib/* output/lib
#	cp wwwroot/*.html output/wwwroot
#	cp wwwroot/image/* output/wwwroot/image
#	cp wwwroot/cgi/math_cgi output/wwwroot/cgi 

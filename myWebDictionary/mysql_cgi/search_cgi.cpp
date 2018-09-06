
#include"comm.h"
using namespace std;

void version()
{
	std::cout<<"Client Version: "<<mysql_get_client_info()<<std::endl;
}
int main()
{
	//从浏览器上面获得环境变量参数
	//getenv(METHOD)
	char arg[1024];

	//判断是GET还是POST请求方法
	//GET方法获取QUERY_STRING
	//POST方法获取CONTENT_LENGTH
	if(getenv("METHOD"))
	{
		if(strcmp("GET",getenv("METHOD"))==0)
		{
			strcpy(arg,getenv("QUERY_STRING"));
		}
		else
		{
			int content_length=atoi(getenv("CONTENT_LENGTH"));
			int i=0;
			char c;
			for(;i<content_length;i++)
			{
				read(0,&c,1);
				arg[i]=c;
			}	
			arg[i]='\0';
		}
	//	std::cout<<arg<<std::endl;
	}
	strtok(arg,"=");
	const char*str=strtok(NULL,"=");
	std::string tmp="'";
	const char* ret="'";
	tmp.append(str);
	tmp.append(ret);
	MYSQL*myfd=initmysql();
	connectmysql(myfd);
	selectmysql(myfd,tmp);
	//关闭mysqlfd
	closemysql(myfd);
	return 0;
}

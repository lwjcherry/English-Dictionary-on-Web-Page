#include"comm.h"

//初始化数据库
MYSQL* initmysql()
{
	MYSQL*myfd=mysql_init(NULL);
	return myfd;	
}
int connectmysql(MYSQL*myfd)
{
	int ret=0;
	//MYSQL*mysql_real_connect
	//(MYSQL*mysql,要连接的已经初始化的mysql句柄
	//const char*host,IP地址，写环回地址
	//const char*user,mysql数据库的用户名
	//const char *passwd,用户密码为空串
	//const char*db,要操作的数据库名
	//unsigned in prot,3306端口号
	//const char *unix_socket,NULL
	//unsigned long clientflag);0
	if(mysql_real_connect(myfd,"127.0.0.1","root","mysql","dictionary",3306,NULL,0))
	{
		std::cout<<"connect success!"<<std::endl;
	}
	else
	{
		std::cerr<<"connect faild!"<<std::endl;
		ret=-1;
	}
	return ret;
}
int insertmysql(MYSQL*myfd,const std::string&word,const std::string&meaning,const std::string&formation)
{
	//INSERT INTO students(name ,school,hobby) VALUES("ZYL","ZX","MM")这是输入的sql语句但是真正要插入的只是VALUES后面的信息，并且要将双引号和逗号剔除
	std::string sql="INSERT INTO word (word,meaning,formation) VALUES(\"";
	sql+=word;
	sql+="\",\"";
	sql+=meaning;
	sql+="\",\"";
	sql+=formation;
	sql+="\")";
	std::cout<<sql<<std::endl;
	return mysql_query(myfd,sql.c_str());//将信息插入到students数据表格中，sql.c_str()是要执行的sql语句，因为sql在这里是c++类型的字符串所以要在插入的时候转成C语言的	
}
void selectmysql(MYSQL*myfd,std::string& str)
{
	//MYSQL_RES*mysql_store_result(MYSQL*mysql);
	std::string sql="SELECT * FROM word where word=";
	sql.append(str);
	mysql_query(myfd,sql.c_str());//执行sql查询语句
	//利用mysql_store_result函数读取查询的数据
	MYSQL_RES*result=mysql_store_result(myfd);
	if(result==NULL)
	{
		std::cout<<"not exit"<<std::endl;
		return;
	}
	//读取MYSQL_RES中的数据
	//获取mysql_num_rows行数
	int rows=mysql_num_rows(result);
	//获取列数	
	int cols=mysql_num_fields(result);
	std::cout<<"<table border=\"1\">"<<std::endl;
	int i=0;
	for(;i<rows;i++)
	{
		std::cout<<"<tr>"<<std::endl;
		int j=0;
		//获取结果内容，返回一个mysql——row变量，其实就是一个char**
		//可以当做二维数组来使用
		MYSQL_ROW row=mysql_fetch_row(result);
		for(;j<cols;j++)
		{
			std::cout<<"<td>"<<row[j]<<"</td>";
		}
		std::cout<<"</tr>"<<std::endl;
	}
	std::cout<<"</table>"<<std::endl;
}
void closemysql(MYSQL* myfd)
{
	mysql_close(myfd);
}


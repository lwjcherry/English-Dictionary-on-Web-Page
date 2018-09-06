#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/socket.h>
#include<string.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<pthread.h>
#include<fcntl.h>
#include<signal.h>
#include<sys/sendfile.h>
#define MAX 1024

int getLine(int sock,char line[],int num)
{
	char c='j';
	int i=0;
	while(c!='\n'&&i<num-1)
	{
		ssize_t size=recv(sock,&c,1,0);
		if(size>0)
		{
			if(c=='\r')
			{
				//recv的窥探功能，只是将sock的字符拷贝一份拿出来，该字符还存在于socket缓冲区中
				recv(sock,&c,1,MSG_PEEK);
				//\r\n
				if(c=='\n')
				{
					recv(sock,&c,1,0);
				}
				// \r
				else
				{
					c='\n';
				}
			}
			line[i++]=c;
		}
		else
		{
			break;
		}
	}
	line[i]=0;
	return i;
}
//清除头部 
void clearHeader(int sock)
{
	char line[MAX];
	do
	{
		getLine(sock,line,MAX);
	}while(strcmp(line,"\n")!=0);
}
static void show_404(int sock,char*path)
{
	char line[MAX];
	sprintf(line,"HTTP/1.0 404 Not Found\r\n");
	send(sock,line,strlen(line),0);
	

	sprintf(line,"Content-Type: text/html\r\n");
	send(sock,line,strlen(line),0);
	sprintf(line,"\r\n");
	send(sock,line,strlen(line),0);

	struct stat st;
	stat("wwwroot/404.html",&st);
	int fd=open("wwwroot/404.html",O_RDONLY);
	sendfile(sock,fd,NULL,st.st_size);
	close(fd);
}

static void show_400(int sock,char*path)
{
	char line[MAX];
	sprintf(line,"HTTP/1.0 400 Request contains syntax error\r\n");
	send(sock,line,strlen(line),0);

	sprintf(line,"Content-Type: text/html\r\n");
	send(sock,line,strlen(line),0);
	struct stat st;
	stat("wwwroot/400.html",&st);
	sprintf(line,"\r\n");
	send(sock,line,strlen(line),0);
	int fd=open("wwwroot/400.html",O_RDONLY);
	sendfile(sock,fd,NULL,st.st_size);
	close(fd);
}

static void show_500(int sock,char*path)
{
	char line[MAX];
	sprintf(line,"HTTP/1.0 500 Server Unable To Complete request\r\n");
	send(sock,line,strlen(line),0);

	sprintf(line,"Content-Type: text/html\r\n");
	send(sock,line,strlen(line),0);
	struct stat st;
	stat("wwwroot/500.html",&st);
	sprintf(line,"\r\n");
	send(sock,line,strlen(line),0);
	int fd=open("wwwroot/500.html",O_RDONLY);
	sendfile(sock,fd,NULL,st.st_size);
	close(fd);
}
//错误处理函数
void echoError(int sock,int status_code,char *path)
{
	switch(status_code)
	{
		case 404:
			show_404(sock,path);
			break;
		case 500:
			show_500(sock,path);
			break;
		case 400:
			show_400(sock,path);
			break;
		default:
			break;
	}
}
//处理返回请求的页面数据的显示格式
int echo_www(int sock,char*path,int size)
{
	clearHeader(sock);//不关心报头，直接清掉
	//HTTP里面的空行是为了区分边界，防止粘包问题
	int fd=open(path,O_RDONLY);
	if(fd<0)
	{
		return 500;
	}
	char *stuff=path+strlen(path)-1;
	while(*stuff!='.' && stuff!=path)
	{
		stuff--;
	}
	char line[MAX];
	sprintf(line,"HTTP/1.0 200 OK\r\n");
	send(sock,line,strlen(line),0);
	if(strcmp(stuff,".html")==0)
	{
		sprintf(line,"Content-Type: text/html\r\n");
	}
	else if(strcmp(stuff,".css")==0)
	{
		sprintf(line,"Content-Type: text/css\r\n");
	}
	else if(strcmp(stuff,".js")==0)
	{
		sprintf(line,"Content-Type: application/x-javascript\r\n");
	}
	
	else if(strcmp(stuff,".jpg")==0)
	{
		sprintf(line,"Content-Type:image/jpeg\r\n");
	}
	else if(strcmp(stuff,".png")==0)
	{
		sprintf(line,"Content-Type:image/x-png\r\n");
	}
	
	send(sock,line,strlen(line),0);
	
	sprintf(line,"Content-Length:%d\r\n",size);
	send(sock,line,strlen(line),0);
	sprintf(line,"\r\n");
	send(sock,line,strlen(line),0);
	//不用经过内核效率高
	sendfile(sock,fd,NULL,size);
	close(fd);
}

int cgi_exe(int sock,char*method,char*path,char*query_string)
{
	char line[MAX];
	char method_env[MAX/16];
	char content_length_env[MAX/16];
	char query_string_env[MAX];
	int content_length=-1;
	int i=0;
	if(strcasecmp(method,"GET")==0)
	{
		clearHeader(sock);
	}
	else
	{
		do
		{
			//获取报文长度
			getLine(sock,line,MAX);
			if(strncasecmp(line,"Content-Length: ",16)==0)
			{
				content_length=atoi(line+16);
			}
		}while(strcmp(line,"\n")!=0);
		if(content_length<0)
		{
			return 400;
		}
	}
	int input[2];
	int output[2];
	pipe(input);
	pipe(output);
	int id=fork();
	if(id<0)
	{
		return 500;
	}
	else if(id==0)
	{
		close(input[1]);
		close(output[0]);
		dup2(input[0],0);
		dup2(output[1],1);
		//当执行execl函数之后原来程的代码段、数据段、栈、堆栈会被另外一个程序替代
		//执行execl函数之后进程的ID并不会被改变所以将method、content_length等参数通过环境变量的方法放到该进程中
		//因为环境变量在整个进程中定义的话在该进程中就可以使用		
		sprintf(method_env,"METHOD=%s",method);
		putenv(method_env);
		if(strcasecmp(method,"GET")==0)
		{
			sprintf(query_string_env,"QUERY_STRING=%s",query_string);
			putenv(query_string_env);
		}
		else
		{
			sprintf(content_length_env,"CONTENT_LENGTH=%d",content_length);
			putenv(content_length_env);
		}
		execl(path,path,NULL);
		close(input[0]);
		close(output[1]);
		exit(0);
	}
	else
	{
		close(input[0]);
		close(output[1]);

		char c;
		if(strcasecmp(method,"POST")==0)
		{
			//把从socket里面读取的正文内容写到input[1]里面
			for(i=0;i<content_length;i++)
			{
				recv(sock,&c,1,0);
				write(input[1],&c,1);
			}
		}
		//组织回复
		sprintf(line,"HTTP/1.0 200 OK\r\n");
		send(sock,line,strlen(line),0);
		sprintf(line,"Content-Type: text/html\r\n");
		send(sock,line,strlen(line),0);
		sprintf(line,"\r\n");
		send(sock,line,strlen(line),0);
		//从output[0]里面读取子进程处理的可执行文件的结果
		while(read(output[0],&c,1)>0)
		{
			send(sock,&c,1,0);
		}
		waitpid(id,NULL,0);

		close(input[1]);
		close(output[0]);
	}
	return 200;
}
void* handlerRequest(void *arg)
{
	int sock=(int)arg;
	char line[MAX];
	char method[MAX/16];
	char url[MAX];
	char path[MAX];
	char* query_string=NULL;//提取出请求的参数
	getLine(sock,line,MAX);
	printf("line:%s\n",line);
	int i=0;
	int j=0;
	int cgi=0;//请求带参的处理模式
	int status_code=200;
	//分离出请求方法
	while(i<sizeof(method)-1&&j<sizeof(line)&&!isspace(line[j]))
	{
		method[i]=line[j];
		i++;
		j++;
	}
	method[i]=0;
	//strcasecmp忽略大小写问题的字符串比较
	//Get Post ...
	if(strcasecmp(method,"GET")==0)
	{
	}
	else if(strcasecmp(method,"POST")==0)
	{
		cgi=1;
	}
	else
	{
		status_code=404;
		clearHeader(sock);
		goto end;
	}
	while(j<sizeof(line)&&isspace(line[j]))
	{
		j++;
	}
	i=0;
	//分离出URL
	while(i<sizeof(url)-1&&j<sizeof(line)&&!isspace(line[j]))
	{
		url[i]=line[j];
		i++;
		j++;
	}
	url[i]=0;
	//分离参数
	//GET请求的参数在URL里面
	if(strcasecmp(method,"GET")==0)
	{
		query_string=url;
		while(*query_string!='\0')
		{
			if(*query_string=='?')
			{
				*query_string='\0';
				cgi=1;
				query_string++;
				break;
			}
			query_string++;
		}
	}
	//URL里面有'/'
	sprintf(path,"wwwroot%s",url);
	if(path[strlen(path)-1]=='/')// wwwroot/,直接将默认网页连接到后面
	{
		strcat(path,"index.html");
	}
	printf("method:%s\n",method);
	printf("path:%s\n",path);
	printf("cgi:%d\n",cgi);
	printf("query_string:%s\n",query_string);
	//stat显示文件的所有信息 0表示成功 -1失败
	struct stat st;
	if(stat(path,&st)<0)
	{
		status_code=404;
		clearHeader(sock);
		goto end;
	}	
	else
	{
		if(S_ISDIR(st.st_mode))
		{
			strcat(path,"/index.html");//当path是一个目录的时候返回默认网页/index.html
		}
		else if((st.st_mode & S_IXUSR)||(st.st_mode & S_IXGRP)||(st.st_mode & S_IXOTH))
		{
			cgi=1;
		}
		else
		{}
	
		//判断cgi若为1则需要处理参数进入CGI运行模式，否则直接返回用户请求的资源
		if(cgi)
		{
			status_code=cgi_exe(sock,method,path,query_string);
		}	
		else
		{
			status_code=echo_www(sock,path,st.st_size);
		}
	}	
end:
	if(status_code!=200)
	{
		echoError(sock,status_code,path);
	}
	close(sock);
}
int  startup(int port)
{
	int sock;
	socklen_t len=sizeof(struct sockaddr_in);
	sock=socket(AF_INET,SOCK_STREAM,0);
	if(sock<0)
	{
		perror("socket");
		exit(2);
	}
	struct sockaddr_in local;
	local.sin_family=AF_INET;
	local.sin_port=htons(port);
	//INADDR_ANY:任意的IP地址Address to accept any incoming messages
	local.sin_addr.s_addr=htonl(INADDR_ANY);
	int ret=bind(sock,(struct sockaddr*)&local,len);
	if(ret<0)
	{
		perror("bind");
		exit(3);
	}
	if(listen(sock,10))
	{
		perror("listen");
		exit(4);
	}
	return sock;
}
int main(int argc,char* argv[])
{
	if(argc!=2)
	{
		printf("Usage:./httpd port\n");
		return 1;
	}
	signal(SIGPIPE,SIG_IGN);
	int newfd=0;
	int lst_fd=startup(atoi(argv[1]));
	while(1)
	{
		//接受新的连接
		struct sockaddr_in cli_addr;
		socklen_t len=sizeof(struct sockaddr_in);
		newfd=accept(lst_fd,(struct sockaddr*)&cli_addr,&len);
		if(newfd<0)
		{
			perror("accept");
			continue;
		}
		
		//创建线程去处理请求
		printf("service get a new connect!!!\n");
		pthread_t tid;
		pthread_create(&tid,NULL,handlerRequest,(void*)newfd);
		pthread_detach(tid);
	}
	return 0;
}

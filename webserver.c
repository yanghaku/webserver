#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<errno.h>
#include<netinet/in.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<sys/stat.h>
#include<fcntl.h>
#include "http.h"


int create_daemon(); //创建守护进程
int create_socket(int); //创建server的socket
void work(int);         //处理请求

int main(int argc,char* argv[]){
	int server_port=-1;
	int opt;
	while((opt=getopt(argc,argv,"p:"))!=-1){//在参数中获取设定的端口号
		if(opt=='p'){
			if(optarg==NULL){
				fprintf(stderr,"error: option -p need a argument!\n");
				exit(1);
			}
			if(sscanf(optarg,"%d",&server_port)!=1){
				fprintf(stderr,"error: oprtion -p need a number argument 0~65535 !!\n");
				exit(1);
			}
			if(server_port<0 || server_port > 65535){
				fprintf(stderr,"error: the port number must be 0~65535!!\n");
				exit(1);
			}
		}
	}
	// 如果参数中没有设定port的值,返回错误退出
	if(!~server_port){//判断是否为-1
		fprintf(stderr,"error: please use option -p to set a listen port!\n");
		exit(1);
	}
	
	int server_sockfd=create_socket(server_port);
	if(!~server_sockfd){//如果创建server的socket失败, 返回-1
		fprintf(stderr,"create socket fail! error num: %d \"%s\"\n",errno,strerror(errno));
		exit(1);
	}

	fprintf(stderr,"create socket ok! port= %d , father pid = %d\n ",server_port,getpid());
	
	if(!~create_daemon()){
		//创建守护进程失败！
		exit(2);
	}
	//成为守护进程......

	work(server_sockfd); //开始处理请求

	return 0;
}


int create_daemon(){//创建守护进程
	pid_t pid=fork();
	
	if(pid<0){//fork 失败
		fprintf(stderr,"create daemon process fail!! error num:%d \"%s\"\n",errno,strerror(errno));
		return -1;
	}
	if(pid>0){//父进程运行到这里就停止
		exit(0);
	}
	//fprintf(stderr,"create daemon successful! the son pid= %d\n\n",getpid());

	signal(SIGTTOU,SIG_IGN);
	signal(SIGHUP,SIG_IGN);
	signal(SIGTSTP,SIG_IGN);
	signal(SIGTTIN,SIG_IGN);
	umask(0); //将文件掩码设置为0
	setsid(); //子进程创建一个新的会话, 并且脱离原先的终端
	//chdir("/");
	close(0);//关闭所有打开的文件描述符
	close(1);
	close(2);
	return 0;
}


int create_socket(int server_port){//创建服务器的socketfd
	int server_sockfd;
	//创建服务器的socket, 参数为AF_INET, SOCK_STREAM //TCP
	if((server_sockfd=socket(AF_INET,SOCK_STREAM,0))==-1)return -1;

	//创建本地的地址和port, 并与socket绑定
	struct sockaddr_in server_addr;
	bzero(&server_addr,sizeof server_addr);
	server_addr.sin_family=AF_INET;
	server_addr.sin_port=htons(server_port);
	server_addr.sin_addr.s_addr=htonl(INADDR_ANY);
	if(bind(server_sockfd,(struct sockaddr*)&server_addr,sizeof server_addr)==-1)return -1;

	//设定相应队列大小
	if(listen(server_sockfd,100)==-1)return -1;

	return server_sockfd;
}

void work(int server_sockfd){
	static unsigned char buf[BUF_SIZE];
	struct sockaddr_in ac_sockaddr;
	socklen_t addr_size=sizeof(ac_sockaddr);
	int ac_sockfd;
	while(1){
		if((ac_sockfd=accept(server_sockfd,(struct sockaddr*)&ac_sockaddr,&addr_size))==-1)continue;
		pid_t pid=fork();
		if(pid==0){//子进程
			int len=recv(ac_sockfd,buf,BUF_SIZE,0);
			int ans=handle_request(buf);
			send(ac_sockfd,buf,ans,0);
			close(ac_sockfd);
			exit(0);
		}
		else if(pid==-1){//生成子进程出错
			close(ac_sockfd);
		}
		waitpid(-1,NULL,WNOHANG);
		//如果父进程,直接忽略进入下一次循环接受请求
	}
}



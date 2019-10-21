#include<stdio.h>
#include<string.h>
#include<time.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<errno.h>
#include<unistd.h>


int get_server_socket(int server_port){
	int server_sockfd;
	//创建服务器的socket, 参数为AF_INET, SOCK_STREAM //TCP
	if((server_sockfd=socket(AF_INET,SOCK_STREAM,0))==-1){
		fprintf(stderr,"socket create error! error number: %d\n",errno);
		exit(2);
	}

	//创建本地的地址和port, 并与socket绑定
	struct sockaddr_in server_addr;
	bzero(&server_addr,sizeof server_addr);
	server_addr.sin_family=AF_INET;
	server_addr.sin_port=htons(server_port);
	server_addr.sin_addr.s_addr=htonl(INADDR_ANY);
	if(bind(server_sockfd,(struct sockaddr*)&server_addr,sizeof server_addr)==-1){
		fprintf(stderr,"socket bind error! error number: %d message is: %s\n",errno,strerror(errno));
		exit(3);
	}
	//设定相应队列大小
	if(listen(server_sockfd,100)==-1){
		fprintf(stderr,"socket listen error! error number: %d\n",errno);
		exit(4);
	}
	fprintf(stderr,"socket create success ! listen is %s:%d\n",inet_ntoa(server_addr.sin_addr),server_port);
	return server_sockfd;
}


int fuck(unsigned char* buf,const char* url){
	const char* head="HTTP/1.1 200 OK\r\nDate: Fri, 18 Oct 2019 07:21:11 GMT\r\nServer: yb_webserver(Ubuntu)\r\nContent-Length: %d\r\nKeep-Alive: timeout=5, max=100\r\nConnection: Keep-Alive Content-Type: text/html; charset=UTF-8\r\n\r\n";
	const char* err="HTTP/1.1 404 Not found\r\n";
	int ch,content_len=0,len=0;
	static unsigned char content[10000];
	chdir("html");
	FILE* fp=fopen(url,"rb");
	chdir("..");
	if(fp==NULL){
		fprintf(stderr,"file open fail! url:\"%s\" is not found\n",url);
		strcpy(buf,err);
		return strlen(buf);
	}
	while((ch=fgetc(fp))!=EOF)content[content_len++]=ch;
	sprintf(buf,head,content_len);
	len=strlen(buf);
	strcat(buf+len,content);
	return len+content_len;
}

int main(int argc,char* argv[]){
	int server_port=-1;
	int opt;
	while((opt=getopt(argc,argv,"p:"))!=-1){
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
	// 如果参数中没有port的值,返回错误退出
	if(!~server_port){
		fprintf(stderr,"error: please use option -p to set a listen port!\n");
		exit(1);
	}
	
	int server_sockfd=get_server_socket(server_port);

	static unsigned char buf[10000];
	static char url[256]; 
	struct sockaddr_in ac_sockaddr;
	socklen_t addr_size=sizeof(ac_sockaddr);
	int ac_sockfd;
	while(1){
		if((ac_sockfd=accept(server_sockfd,(struct sockaddr*)&ac_sockaddr,&addr_size))==-1){
			fprintf(stderr,"accept error! %s %d\n",strerror(errno),errno);
			continue;
		}
		int len=recv(ac_sockfd,buf,10000,0);
		fprintf(stderr,"accept %s:%d\n",inet_ntoa(ac_sockaddr.sin_addr),ntohs(ac_sockaddr.sin_port));
		for(int i=0;i<len;++i)fputc(buf[i],stderr);fprintf(stderr,"\n");
		for(int i=0;i<len;++i){
			if(buf[i]=='/'){
				if(buf[i+1]==' ')strcpy(url,"index.html");
				else sscanf(buf+i+1,"%s",url);
				break;
			}
		}
		len=fuck(buf,url);
		len=send(ac_sockfd,buf,len,0);
		fprintf(stderr,"send len=%d\n",len);
		close(ac_sockfd);
	}
	return 0;
}

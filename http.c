#include "http.h"
#include<string.h>
#include<stdio.h>

static unsigned content[BUF_SIZE];//返回的内容缓冲区

int get_content(const char* url);//返回长度, 失败返回-1

int get_head(unsigned char*,int);




int handle_request(unsigned char* buf){
	char url[256];
	for(int i=0;buf[i];++i){
		if(buf[i]=='/'){
			if(buf[i+1]==' ')strcpy(url,"index.html");
			else sscanf(buf+i+1,"%s",url);
			break;
		}
	}
	int content_len=get_content(url);
	int len=get_head(buf,content_len);
	if(content_len==-1)return len;
	strcat(buf+len,content);
	return len+content_len;
}



int get_head(unsigned char* buf,int len){
	const char* head="HTTP/1.1 200 OK\r\nDate: Fri, 18 Oct 2019 07:21:11 GMT\r\nServer: yb_webserver(Ubuntu)\r\nContent-Length: %d\r\nKeep-Alive: timeout=5, max=100\r\nConnection: Keep-Alive Content-Type: text/html; charset=UTF-8\r\n\r\n";
	const char* err="HTTP/1.1 404 Not Found\r\nDate: Fri, 18 Oct 2019 07:21:11 GMT\r\nServer: yb_webserver(Ubuntu)\r\nContent-Length: 53\r\nKeep-Alive: timeout=5, max=100\r\nConnection: Keep-Alive Content-Type: text/html; charset=UTF-8\r\n\r\n<!Doctype html><html><body><h1>404</h1></body></html>";
	if(len==-1)strcpy(buf,err);
	else sprintf(buf,head,len);
	return strlen(buf);
}

int get_content(const char* url){//得到返回内容
	chdir("html");
	FILE* fp=fopen(url,"rb");
	chdir("..");
	if(fp==NULL)return -1;//没有对应的目录
	return fread(content,sizeof(char),BUF_SIZE,fp);
}



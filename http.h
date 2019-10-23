#ifndef HTTP_INCLUDE_H
#define HTTP_INCLUDE_H

#define BUF_SIZE 200000
#define URL_MAX_LEN 8183//chrome 最大支持长度

#define WORK_DIR "html" //服务器工作目录
#define GET 1
#define POST 2

//#define DEBUG

int handle_request(unsigned char*);


typedef struct Http_Request{
	int method;//只实现 GET请求 
	char url[URL_MAX_LEN];	//url
} http_request;


typedef struct Http_Response{
	int status;//实现三种：200 OK ; 404 Not Found ; 503 Server Unavailable
	char content_type[30];// text/html; text/css; application/javascript; image/png,jpg,jpeg,gif
	char date[30];	//
	int content_length;
} http_response;



#endif

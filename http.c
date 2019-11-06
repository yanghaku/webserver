#include "http.h"
#include<string.h>
#include<stdio.h>
#include<time.h>
#include<dirent.h>
#include<syslog.h>
#include<unistd.h>


const char* http_version="HTTP/1.1"; //http协议的版本
const char* server_name="yb_webserver"; //server_name


static http_request request;	//保存解析后的http请求
static http_response response; //保存生成的响应头

static unsigned char content[BUF_SIZE];//返回的内容缓冲区

int get_content(FILE* fp);//将内容读入到content返回长度, 失败返回-1 (如果fp==NULL,那就返回ls的内容

int get_head(unsigned char* buf,int content_len);//将生成的响应头保存在buf里,len为内容的长度

int parse_request(unsigned char* buf);//将http的请求解析出来,如果解析错误或者是不支持,就返回-1

void print_dir(char* pre_dir,int pre_len,char* dir,int* len);//递归访问所有目录

void run_process(int url_len); //运行指定的程序,并将结果保存在run/tmp{pid}.out中 (url的长度传过去,减少一次重复计算)

void write_log();//记录日志

int handle_request(unsigned char* buf){//处理http请求，生成http的response
#ifdef DEBUG
	fprintf(stderr,"request:\n%s\n",buf);
#endif

	time_t now;time(&now);
	strcpy(response.date,ctime(&now));//时间
	response.content_type[0]=0;//将返回的文件类型清空
	int len,content_len;
	FILE* fp=NULL;

	if(parse_request(buf)==-1){//503
		response.status=503;
		fp=fopen("503.html","rb");
	}
	else if(request.url[0]!='\0'){//即url不为空的时候
		int url_len=strlen(request.url);
		if(url_len>=4 && request.url[url_len-4]=='.' && request.url[url_len-3]=='r' && 
					request.url[url_len-2]=='u' && request.url[url_len-1]=='n'){//run!
			sprintf(content,"run/tmp%d.out",getpid());//在调用getcontent之前,content还未使用,所以这里构造的时候利用content作为buffer
			run_process(url_len);
			fp=fopen(content,"rb");//运行的程序结果保存在tmp.out中
		}
		else{//正常尝试打开文件
			fp=fopen(request.url,"rb");
		}
		if(fp==NULL){//没有对应的文件的时候返回404
			fp=fopen("404.html","rb");
			response.status=404;
			strcpy(response.content_type,"text/html");//404的时候返回404.html
		}
		else response.status=200;
	}
	else response.status=200;
	if(response.content_type[0]==0)strcpy(response.content_type,"text/html");//503或别的情况的时候

	//如果是NULL,就是url为 '/'的时候,就返回ls的内容
	content_len=get_content(fp);
	len=get_head(buf,content_len);
	
	for(int i=0;i<content_len;++i){//将内容与首部连接
		buf[len++]=content[i];
	}
#ifdef DEBUG
	fprintf(stderr,"response:\n");
	if(len>1000)for(int i=0;i<1000;++i)fprintf(stderr,"%c",buf[i]);
	else for(int i=0;i<len;++i)fprintf(stderr,"%c",buf[i]);
	fprintf(stderr,"\n");
#endif
	// 如果运行了文件,就将生成的临时文件删除
	sprintf(content,"run/tmp%d.out",getpid());
	if(access(content,F_OK)==0)remove(content);
	write_log();
	return len;//返回总长度
}


int parse_request(unsigned char* buf){//将http的请求解析出来,如果解析错误或者是不支持,就返回-1
	if( !( buf[0]=='G' && buf[1]=='E' && buf[2]=='T'))return -1;
	request.method=GET;
	for(int i=0;buf[i];++i){
		if(buf[i]=='/'){
			if(buf[i+1]==' ')request.url[0]='\0';
			else{
				sscanf(buf+i+1,"%s",request.url);
				int len=strlen(request.url)-1;
				while(len>-1 && request.url[len]!='.')--len;
				if(len>=0&&request.url[len]=='.'){//有后缀
					if(strcmp(request.url+len+1,"css")==0)//css
						strcpy(response.content_type,"text/css");
					else if(strcmp(request.url+len+1,"js")==0)//js
						strcpy(response.content_type,"application/javascript");
					else if(strcmp(request.url+len+1,"png")==0)
						strcpy(response.content_type,"image/png");
					else if(strcmp(request.url+len+1,"ico")==0)
						strcpy(response.content_type,"image/ico");
					else if(strcmp(request.url+len+1,"jpg")==0)
						strcpy(response.content_type,"image/jpg");
					else strcpy(response.content_type,"text/html");//默认
				}
				else strcpy(response.content_type,"text/html");//默认
			}
			return 1;
		}
	}
	return -1;
}

int get_head(unsigned char* buf,int content_len){//将生成的响应头保存在buf里,len为内容的长度
	strcpy(buf,http_version);
	int len=strlen(buf);
	if(response.status==200)strcpy(buf+len," 200 OK\r\nDate: ");
	else if(response.status==503)strcpy(buf+len," 503 Server Unavailable\r\nDate: ");
	else if(response.status==404)strcpy(buf+len," 404 Not Found\r\nDate: ");
	len += strlen(buf+len);
	strcpy(buf+len,response.date);
	len += strlen(buf+len);
	sprintf(buf+len-1,"\r\nServer: %s\r\nContent-Length: %d\r\nContent-Type: %s; charset=UTF-8\r\n\r\n",server_name,content_len,response.content_type);
	return len+strlen(buf+len);
}


int get_content(FILE* fp){//得到返回内容
	if(fp)return fread(content,sizeof(unsigned char),BUF_SIZE,fp);
	fp=fopen("template.html","rb");
	if(fp==NULL)return 0;//template.html被删除了！！
	int len=fread(content,sizeof(unsigned char),BUF_SIZE,fp);
	static char dir_pre[256];
	print_dir(dir_pre,0,".",&len);
	chdir(WORK_DIR);
	strcpy(content+len,"});</script></body></html>");
	fclose(fp);
	return len+26;
}

void print_dir(char* dir_pre,int pre_len,char* dir,int* len){//递归访问所有目录
	struct dirent* x;
	DIR* dir_point=opendir(dir);
	if(dir_point==NULL){
		return;//打不开这个文件夹(可能没有权限之类的),直接退出
	}
	content[(*len)++]='[';//当前层js数组的左括号
	chdir(dir);
	while(x=readdir(dir_point)){//遍历这个文件夹的内容
		//如果是上一级或者上上级,则跳过
		if(strcmp(x->d_name,".")==0 || strcmp(x->d_name,"..")==0)continue;

		if(x->d_type==DT_DIR){//文件夹就继续递归
			sprintf(content+*len,"{text:\"%s\",tags:[\"folder\"],nodes:",x->d_name);
			*len += strlen(content+*len);
			
			dir_pre[pre_len]='/';
			strcpy(dir_pre+pre_len+1,x->d_name);
			print_dir(dir_pre,pre_len+1+strlen(x->d_name),x->d_name,len);
			dir_pre[pre_len]='\0';

			content[(*len)++]='}';
			content[(*len)++]=',';
		}
		else if(x->d_type==DT_REG){//实体文件
			sprintf(content+*len,"{text:\"%s\",tags:[\"file\"],href:\"%s/%s\"},",x->d_name,dir_pre,x->d_name);
			*len += strlen(content+*len);
		}
	}
	chdir("..");
	closedir(dir_point);
	content[(*len)++]=']';//当前层js数组的右括号
}


void run_process(int url_len){
	static char commend[URL_MAX_LEN+10];
#ifdef DEBUG
	system(request.url);
#endif
	strcpy(commend,request.url);
	sprintf(commend+url_len," 2> %s 1>&2",content);

#ifdef DEBUG
	fprintf(stderr,"run: %s\n",commend);
#endif
	system(commend);
}

void write_log(){
	/** 记录日志 */
	static char method[20];
	if(request.method==GET)strcpy(method,"GET");
	else if(request.method==POST)strcpy(method,"POST");
	else strcpy(method,"Unknown");
	openlog("webserver_YB",LOG_CONS | LOG_PID , 0);
	syslog(LOG_DEBUG ," method: %s  url: %s  response code: %d \n",method,request.url,response.status);
	closelog();
}

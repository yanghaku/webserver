#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>

int fuck(int argc,char* argv[]){
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
	return server_port;
}
	
int main(int argc,char* argv[]){
//	int x=fuck(argc,argv);
	pid_t pid=fork();
	if(pid==0){
		pid_t sonson=fork();
		if(sonson==0){//gradeson
			sleep(5);
			int x=fuck(argc,argv);
			fprintf(stderr,"this is the gradeson\n");
			sleep(1);
		}
		else if(sonson==-1)fprintf(stderr,"gradeson error\n");
		else{
			sleep(1);
			fprintf(stderr,"this is son\n");
			sleep(1);
		}
	}
	else if(pid==-1){
		fprintf(stderr,"error\n");
	}
	else{
		for(int i=0;i<1;++i){
			fprintf(stderr,"this is the father\n");
			sleep(1);
		}
	}

	return 0;
}

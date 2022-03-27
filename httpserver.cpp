#include<iostream>
#include<fstream>

#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include<sys/socket.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<netinet/in.h>

#include<unistd.h>
#include<fcntl.h>

using namespace std;

//建立http標頭檔格式
struct httpHeader{
	char  request[2048];            //Request Header
	char  request_line[60];         //第一行:Get /...
	
	char  file_name[30];            //請求物件名稱
	char  method[10];               //請求形式 ex:GET
    char  type[20];                 //請求物件資料結構
    
    char  response_header[2048];    //Response Header
    char  content_type[50];         //回傳MIME型態
    char* response_data;            //回覆資料
};

char* get_content(const char * file){           //從需求物件名稱裡找尋回傳MIME型態
	fstream mime;
    char* file_type =new char [30];             //回傳宣告
	char  gets [128];
	char* token;
	
	strcpy(file_type,file);                     //抓取型態 ex:1.png → png
	strtok(file_type,".");
	strcpy(file_type,strtok(NULL," "));
		
    mime.open("MIMETYPE.txt",ios::in);          //回傳MIME型態表
	
	while(mime.getline(gets,sizeof(gets)))      //一行一行抓取
	{
		if((token = strtok(gets," "))!=NULL)    //抓取檔案型態與MIME型態間的空格 — 格式:html text/html
		{
			if(strcmp(gets,file_type)==0)       //比較檔案型態與檔案內型態是否一致
			{
				strcpy(file_type,strtok(NULL," ")); //複製到回傳陣列
				break;
			}
		}
	}
	return file_type;
	
}


int main(int argc,char * argv[]){

    //初始化宣告socket變數
    int server_socket=0;
    int client_socket=0;
    int ports =0;
    
    //宣告address 長度；與int 無異 POSIX
    socklen_t client_addr_size; 
    //宣告sockaddr_in型態；區分port和 ip address
    struct sockaddr_in server_address,client_address;
    
    //開啟socket；SOCK_STREAM為TCP
    if((server_socket = socket(AF_INET, SOCK_STREAM,0))==-1)
    {
    	cout<<"Error: Socket "<<strerror(errno)<<endl;
    	exit(1);
    }
    //判斷是否輸入port數，無則設定為8000
    if(argv[1]==NULL)
    {
    	ports=8000;
    }
    else
    {
    	sscanf(argv[1],"%d",&ports);
    }
    //宣告server所屬家族、port和address
    server_address.sin_family = AF_INET;
    server_address.sin_port =  htons(ports);
    server_address.sin_addr.s_addr = INADDR_ANY;    //任意主機

    //定名且連接到傳輸提供者之位址上
    if(bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address))!=0)
    {
    	cout<<"Bind:"<<strerror(errno)<<endl;
    	close(server_socket);
    	exit(1);
    }
    //監聽是否超過5人使用(僅定義)
    if(listen(server_socket, 5)!=0)
    {
    	cout<<"Listen:"<<strerror(errno)<<endl;
        close(server_socket);
        exit(1);	
    }
    while(1){
    	
    	httpHeader data;
    	
        //清空陣列
    	memset(data.request,0,sizeof(data.request));
    	memset(data.response_header,0,sizeof(data.response_header));		
    	

    	client_addr_size = sizeof(client_address);		

        //接受連線	
    	if((client_socket = accept(server_socket, (struct sockaddr *)&client_address,&client_addr_size))==-1)
    	{	
    		cout<<"Accept:"<<strerror(errno)<<endl;
    		break;
    	}
    	//接收request
    	int receive=recv(client_socket,data.request,sizeof(data.request),0);
    	
        //判斷是否有誤 or client關閉連線
    	if(receive==-1)
    	{
    		cout<<"Receive:"<<strerror(errno)<<endl;
    		break;
    	}
    	if(receive==0)
    	{
    		cout<<"Client Disconnect"<<endl;
    		break;
    	}

        //將requst的首行:GET /file HTTP/1.1 拆解
        strcpy(data.request_line,strtok(data.request,"\r\n"));
        strcpy(data.method,strtok(data.request," "));
        strcpy(data.file_name, strtok(NULL, " "));
	
        //是否為GET請求
        if(strcmp(data.method,"GET")!=0)
        {
        	strcpy(data.response_header,"HTTP/1.0 400 Bad Request\r\n\r\n");
        	send(client_socket, data.response_header, sizeof(data.response_header),0);
	
        }
        //是否為HTTP請求
        else if(strstr(data.request_line,"HTTP/")== NULL)
        {
        	strcpy(data.response_header,"HTTP/1.0 400 Bad Request\r\n\r\n");
        	send(client_socket, data.response_header, sizeof(data.response_header),0);
		
        }
        else
        {	fstream http_file;

            // GET / HTTP/1.1 則直接抓取index;
            if(strcmp(data.file_name,"/")==0)
            {
                http_file.open("index.html",ios::binary|ios::in);
                strcpy(data.type,get_content("index.html"));
            } 
            //GET /file HTTP/1.1 抓取file物件
            else
            {
                http_file.open(data.file_name+1,ios::binary|ios::in);
                strcpy(data.type,get_content(data.file_name+1));
                        
            }
            //無法開啟或找不到，則回傳404
            if(!http_file)
            {
                    strcpy(data.response_header,"HTTP/1.0 404 Not Found\r\n\r\n");
                    send(client_socket, data.response_header, sizeof(data.response_header),0);
                
            }	
            else{
                //回傳200與MIME型態給client
                strcpy(data.response_header,"HTTP/1.0 200 OK\r\n");
                sprintf(data.content_type,"Content-Type:%s\r\n\r\n",data.type);
                strcat(data.response_header,data.content_type);
                
                send(client_socket, data.response_header, sizeof(data.response_header),0);
                
                //將檔案從頭看到尾，找出length
                http_file.seekg(0, ios::end);
                long fileSize = http_file.tellg();
                http_file.seekg(0, ios::beg);

                //動態新增一個fileSize長度陣列
                data.response_data=new char[fileSize];
                
                //讀入陣列並傳送回覆
                http_file.read(data.response_data,fileSize);
                send(client_socket, data.response_data, fileSize,0);
                
                memset(data.response_data,0,sizeof(data.response_data));
                    
            }
            http_file.close();	//關閉檔案
	}
	close(client_socket);//關閉目前接受之連線
   }
   close(server_socket);//關閉server連線
   return 0;
}


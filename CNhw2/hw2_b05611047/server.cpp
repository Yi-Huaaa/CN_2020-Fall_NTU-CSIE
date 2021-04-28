/*Ref: 
1. dir: https://stackoverflow.com/questions/4204666/how-to-list-files-in-a-directory-in-a-c-program/17683417            
2. put:
3. get: https://wenchiching.wordpress.com/2009/10/14/linux-c-socketclientserver-transfer-file%E5%82%B3%E9%80%81%E6%AA%94%E6%A1%88/ 
4. select(): https://www.geeksforgeeks.org/socket-programming-in-cc-handling-multiple-clients-on-server-without-multi-threading/
*/
#include <iostream>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h> 
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros 
#include <errno.h> 
#include <netinet/in.h> 
#include "opencv2/opencv.hpp"
#include <string>
#include <sstream>
#include <sys/stat.h>
#include <pthread.h>
#define BUFF_SIZE 1024
using namespace std;
using namespace cv;

void list_file(int remoteSocket, char Buf[]){
    int sent;
    DIR *d = opendir("./server_f");// opendir() returns a pointer of DIR type
    struct dirent *dir;// Pointer for directory entry
    bzero(Buf, sizeof(char)* BUFF_SIZE);
    if(d){
        while((dir = readdir(d)) != NULL){
            strcpy(Buf, dir->d_name);
            Buf[strlen(Buf)] = ' ';
            sent = send(remoteSocket, Buf, BUFF_SIZE, 0);
            //printf("%s\n",dir->d_name);
            bzero(Buf, sizeof(char)* BUFF_SIZE); 
        }
        closedir(d);
    }
    
    else { //if(d == NULL), opendir returns NULL if couldn't open directory
        strcpy(Buf, "the file contains NULL\n");
    }
}
void copy_flnm(char Buf[], char filenm[]){
    printf("Buf = %s\n", Buf);
    for (int i = 4; i < strlen(Buf); i++){
        filenm[i-4] = Buf[i];
        printf("filenm[%d] = %c, Buf[%d] = %c\n",i-4, filenm[i-4], i, Buf[i]);
    }    
    printf("copy file name: %s\n", filenm);

}
//void find_file(char filenm[], char Buf[], char exist[]){
void find_file(char filenm[], char exist[]){
    int find_file = 0;
    //printf("Into find_file\n");
    DIR *d = opendir("./server_f");
    struct dirent *dir;
    if(d){
        while((dir = readdir(d)) != NULL){
            //printf("filenm: %s, dir->d_name: %s\n", filenm, dir->d_name);
            //printf("strlen(filenm) = %d, strlen(dir->d_name) = %d\n",strlen(filenm), strlen(dir->d_name));
            if(strcmp(filenm, dir->d_name) == 0){
                find_file = 1;
                printf("Finded out the file, file name: %s\n", filenm);
                break;
            }
        }
        closedir(d);
    }
    //printf("find_file = %d\n", find_file);
    if(find_file == 1){
        exist[0] = '1';
        printf("exist[0] = %c, Finding file successful.\n",exist[0]);
    }
    else{
        exist[0] = '0';
        printf("exist[0] = %c, Finding file failed.\n",exist[0]);
    }
}
void send_file(char filenm[], int remoteSocket){
    int numbytes;
    FILE *fp;
    char tmp_buf[BUFF_SIZE];
    string str_1 = "./server_f/";
    string str_2 = filenm;
    string str_3 = str_1 + str_2;
    fp = fopen(str_3.c_str(), "rb");
    bzero(tmp_buf, sizeof(char)* BUFF_SIZE); 
    
    while(!feof(fp)){
        numbytes = fread(tmp_buf, sizeof(char), sizeof(tmp_buf), fp);
        printf("fread %d bytes, ", numbytes);
        numbytes = write(remoteSocket, tmp_buf, BUFF_SIZE);
        printf("Sending %d bytes\n",numbytes);
        bzero(tmp_buf, sizeof(char)* BUFF_SIZE);
    }
}
void rec_file(char exist[], int remoteSocket, char instruction[], char filenm[]){
    if(exist[0] == '0'){//檔案不存在
        printf("the %s does not exist.\n", filenm);
    }
    else{//檔案存在
        FILE *fp;
        int numbytes;
        string str_1 = "./server_f/";
        string str_2 = filenm;
        string str_3 = str_1 + str_2;
        if ((fp = fopen(str_3.c_str(), "wb")) == NULL){
            perror("fopen error");
            exit(1);
        }
        while(1){
            numbytes = read(remoteSocket, instruction, BUFF_SIZE);
            printf("read %d bytes, ", numbytes);
            if(numbytes <= 0){
                break;
            }
            numbytes = fwrite(instruction, sizeof(char), numbytes, fp);
            printf("fwrite %d bytes\n", numbytes);
            bzero(instruction, sizeof(char)* BUFF_SIZE);
        }
        fclose(fp);
    }
}
int  play_video_ing[30][4];//，因為最多30個clients
char play_video_flnm[30][BUFF_SIZE]; 
void *doInChildThread(void *arg){
    int i = *(int*)arg;
    char filenm [BUFF_SIZE];
    for (int j = 0; j < BUFF_SIZE; j++)
        filenm[j] = play_video_flnm[i][j];
    int remoteSocket = play_video_ing[i][2]; // 2:remote

    play_video_ing[i][0] = 1; //0開始

    //-------------------------------------- 
    string str_1 = "./server_f/";
    string str_2 = filenm;
    string str_3 = str_1 + str_2;
    VideoCapture cap(str_3.c_str());
    // server
    Mat imgServer;
    //VideoCapture cap("./tmp.mpg");
    int sent;
    // get the resolution of the video
    int width = cap.get(CV_CAP_PROP_FRAME_WIDTH);//width
    int height = cap.get(CV_CAP_PROP_FRAME_HEIGHT);//height
    printf("width = %d, height = %d\n", width, height);    
    std::string wid_str = std::to_string(width);
    std::string hei_str = std::to_string(height);
    char buffer[BUFF_SIZE];
    bzero(buffer, sizeof(char)*BUFF_SIZE);
    
    strcpy(buffer, wid_str.c_str());
    sent = send(remoteSocket, buffer, BUFF_SIZE, 0);
    bzero(buffer, sizeof(char)*BUFF_SIZE);
    
    strcpy(buffer, hei_str.c_str());
    sent = send(remoteSocket, buffer, BUFF_SIZE, 0);
    bzero(buffer, sizeof(char)*BUFF_SIZE);

    //allocate container to load frames 
    imgServer = Mat::zeros(height, width, CV_8UC3);    
    // ensure the memory is continuous (for efficiency issue.)
    if(!imgServer.isContinuous()){
         imgServer = imgServer.clone();
    }
    int count = 0;
    while(1){
        //get a frame from the video to the container on server.
        cap >> imgServer;
        // get the size of a frame in bytes 
        int imgSize = imgServer.total() * imgServer.elemSize();

        //傳 imgSize
        std::string imgSize_str = std::to_string(imgSize);
        strcpy(buffer, imgSize_str.c_str());
        sent = send(remoteSocket, buffer, BUFF_SIZE, 0);
        printf("sent imgSize = %d to client\n", imgSize);
        bzero(buffer, sizeof(char)*BUFF_SIZE);
        
        uchar *buffer_u = (uchar*)malloc( imgSize * sizeof(uchar));

        // copy a frame to the buffer
        memcpy(buffer_u, imgServer.data, imgSize);
        sent = write(remoteSocket, buffer_u, imgSize);
        printf("Sending numbytes = %d bytes, imgSize = %d\n", sent, imgSize);
        bzero(buffer_u, sizeof(uchar)* imgSize);

        //char c 應該是client端傳過來的
        char c_buf[2];
        int read_c = read(remoteSocket, c_buf, 2);
        printf("get read_c = %c\n", c_buf[0]);
        if(c_buf[0] == 27){
            break;
        }
        //if(c_buf[0] == 27 || count == 200){
        //   break;
        //}
        count ++;
        printf("count = %d\n", count);

        free(buffer_u);
    } 
    printf("Server transfer over, count = %d\n", count);
    play_video_ing[i][1] = 1;//1 結束，代表這個影片播完了可以關閉socket了
    cap.release();
}

int main(int argc , char *argv[]){
    int status;
    status = mkdir("./server_f", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    char Buf[BUFF_SIZE];//Buf用來存放read到的資料
    char filenm[BUFF_SIZE];//fike name buffer
    FILE *fp;

    int localSocket, remoteSocket;// local: server, remote: client                            
    
    string port_stg = argv[1];
    int port = atoi(port_stg.c_str());
    printf("Get port number: %d\n",port);
    //int port = 8888;
    struct  sockaddr_in localAddr,remoteAddr;
    int addrLen = sizeof(struct sockaddr_in);  
    
    //(localSocket = socket(AF_INET , SOCK_STREAM , 0));
    if ((localSocket = socket(AF_INET , SOCK_STREAM , 0)) < 0){
        perror("socket create error!");
        exit(1);
    }
    printf("Server socket creation successful.\n");

    localAddr.sin_family = AF_INET;
    localAddr.sin_port = htons(port);
    localAddr.sin_addr.s_addr = INADDR_ANY;

    if(bind(localSocket,(struct sockaddr *)&localAddr , sizeof(localAddr)) < 0) {
        perror("Bind error!");
        exit(1);
    }
    printf("Binding successful.\n");

    if(listen(localSocket , 3) < 0){
    	perror("Listen error");
    	exit(1);
    }

    // select_1: start
    int max_clients = 30;
    int client_socket[30];
    int activity, max_sd, sd;
    fd_set readfds;
    for(int i = 0; i < max_clients; i++)
        client_socket[i] = 0;
    //sekect_1: end
    for(int i = 0; i < max_clients; i++){
        play_video_ing[i][0] = 0;//開始
        play_video_ing[i][1] = 0;//結束
        play_video_ing[i][2] = 0;//remoteSo = sd
        play_video_ing[i][3] = 0;// i 
    }
    int num = 0;
    pthread_t pid[30];


    while(1){ 
        printf("Waiting for connections...\n"); 
        printf("Server Port: %d\n", port);
        //select_2_start
        FD_ZERO(&readfds);
        FD_SET(localSocket, &readfds);
        max_sd = localSocket;
        //add child sockets to set
        for(int i = 0;  i< max_clients; i++){
            sd = client_socket[i];
            if(sd > 0)
                FD_SET(sd, &readfds);
            if(sd > max_sd)
                max_sd = sd;
        }
        activity = select(max_sd+1,&readfds, NULL, NULL, NULL);
        if ((activity < 0) && (errno != EINTR)) { 
            printf("select error\n"); 
        } 
        //If something happened on the master socket , 
        //then its an incoming connection 
        if (FD_ISSET(localSocket, &readfds)){ 
            //remoteSocket = accept(localSocket, (struct sockaddr *)&remoteAddr, (socklen_t*)&addrLen);  
            if ((remoteSocket = accept(localSocket, (struct sockaddr *)&remoteAddr, (socklen_t*)&addrLen)) < 0) {
                perror("Accept error");
                exit(1);
            }
            printf("Connection accepted.\n");
            //add new socket to array of sockets 
            int i = 0; 
            while (i < max_clients){
                if(client_socket[i] == 0){
                    client_socket[i] = remoteSocket;
                    printf("Adding to list of sockets as %d\n" , i);
                    break;
                }
                i++;
            } 
        } 
        //select_2_ends
        //select_3_starts: 包兩層
        for(int i = 0; i < max_clients; i++){
            sd = client_socket[i];
            //printf("1:::sd = %d\n", sd);
            if(FD_ISSET(sd, &readfds) &&play_video_ing[i][0] == 0){
                bzero(Buf, sizeof(char)* BUFF_SIZE); 
                int read_ins;//用來接指令的，他會存總共有幾個bits
                printf("Reading instruction...\n");
                if ((read_ins = read(sd, Buf, BUFF_SIZE)) < 0) {
                    perror("Read instruction error.");
                    exit (1);
                }
                //printf("get inst: %d bytes\n", read_ins);

                //作動
                if(Buf[0] == 'l' && Buf[1] == 's'){ //ls-1: OK!
                    printf("Instruction: %s\n", Buf);
                    list_file(sd, Buf);
                    close(sd);
                    client_socket[i] = 0;
                } 
                else if(Buf[0] == 'p' && Buf[1] == 'u' && Buf[2] == 't') { // put-2
                    printf("Instruction: %s\n", Buf);
                    //先接檔案名稱
                    printf("Receiving filenm...\n");
                    bzero(filenm, sizeof(char)* BUFF_SIZE);
                    if ((read_ins = read(sd, filenm, BUFF_SIZE)) < 0) {
                        perror("Get filenm error");
                        exit (1);
                    }
                    //printf("get filenm: %d bytes\n",read_ins);
                    char exist[2];
                    //確認檔案存在
                    bzero(exist, sizeof(char)* 2);
                    int file_exist = read(sd, exist, 2);
                    printf("file_exist = %d, exist[0] = %c\n", file_exist, exist[0]);
                    rec_file(exist, sd, Buf, filenm);//這裡的remote socket有可能是local socket，
                    
                    close(sd);
                    client_socket[i] = 0;
                }
                else if(Buf[0] == 'g' && Buf[1] == 'e' && Buf[2] == 't') { // get: client get, server send-OK
                    printf("Instruction: %s\n", Buf);
                    //先接檔案名稱
                    printf("Receiving filenm...\n");
                    bzero(filenm, sizeof(char)* BUFF_SIZE);
                    if ((read_ins = read(sd, filenm, BUFF_SIZE)) < 0) {
                        perror("Get filenm error");
                        exit (1);
                    }
                    printf("Receiving filenm successful.\n");
                    char exist[2];
                    //確認檔案存在
                    bzero(exist, sizeof(char)* 2);
                    printf("Finding file...\n");
                    find_file(filenm, exist);
                    int sent;
                    sent = send(sd, exist, 2, 0);
                    //存在後再傳檔案
                    if(exist[0] == '1'){
                        printf("Sending file start...\n");
                        send_file(filenm, sd);
                        printf("Sending file successful.\n");
                    }
                    close(sd);
                    client_socket[i] = 0;
                }
                else if(Buf[0] == 'p' && Buf[1] == 'l' && Buf[2] == 'a' && Buf[3] == 'y') { 
                    printf("Instruction: play\n");
                    printf("Receiving filenm...\n");
                    bzero(filenm, sizeof(char)* BUFF_SIZE);
                    if ((read_ins = read(sd, filenm, BUFF_SIZE)) < 0) {
                        perror("Get filenm error");
                        exit (1);
                    }
                    printf("Receiving filenm successful.\n");
                    char exist[2];
                    //確認檔案存在
                    bzero(exist, sizeof(char)* 2);
                    printf("Finding file...\n");
                    find_file(filenm, exist);
                    int sent;
                    sent = send(sd, exist, 2, 0);
                    //存在之後再播影片
                    if(exist[0] == '1'){
                        printf("Playing video starts...\n");
                        play_video_ing[i][2] = sd; // remoteSocket的值
                        play_video_ing[i][3] = i; // client_scoket[i];
                        for (int j = 0; j < BUFF_SIZE; j++)
                            play_video_flnm[i][j] = filenm[j];
                        int* input = new int(i);
                        pthread_create(&pid[i], NULL, doInChildThread, (void*)input);
                        num ++;
                        printf("Playing video successful.\n");
                    }
                    //-------這裡比較特別------先不要關閉socket
                }
                else{
                    printf("command not found.\n");
                    close(sd);
                    client_socket[i] = 0;
                }
                for (int i = 0; i < max_clients; i++){
                    if(play_video_ing[i][0] == 1 && play_video_ing[i][1] == 1){ 
                    // 有被設定成播放影片，且播放玩了
                        printf("close playing video socket: %d\n",i);
                        close(play_video_ing[i][2]);//他的remoteSocket
                        client_socket[ play_video_ing[i][3] ] = 0;
                        pthread_join(pid[i], NULL);
                        play_video_ing[i][0] = 0;
                        play_video_ing[i][1] = 0;
                        play_video_ing[i][2] = 0;
                        play_video_ing[i][3] = 0;
                    }
                }
                //close(sd);
                //client_socket[i] = 0;
            }
        }
        //select_3_ends，包兩層
	}
    close(localSocket);
    return 0;
}



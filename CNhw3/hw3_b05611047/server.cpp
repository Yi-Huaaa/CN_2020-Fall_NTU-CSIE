/*
1. 影片播放的esc按鍵
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
#include <sys/time.h> 
#include <errno.h> 
#include <netinet/in.h> 
#include "opencv2/opencv.hpp"
#include <string>
#include <sstream>
#include <sys/stat.h>
#include <pthread.h>
#define BUFF_SIZE 4000
using namespace std;
using namespace cv;

typedef struct {
	int length;
	int seqNumber;
	int ackNumber;
	int fin;
	int syn;
	int ack;
} header;

typedef struct{
	header head;
	char data[4000];
} segment;

void setIP(char *dst, char *src) { //strcmp == 0, 代表一樣
    if(strcmp(src, "0.0.0.0") == 0 || strcmp(src, "local") == 0
            || strcmp(src, "localhost")) {
        sscanf("127.0.0.1", "%s", dst);// sscanf: source, type, 存入
    } else {
        sscanf(src, "%s", dst);
    }
}
void find_file(char filenm[], char exist[]){
    int find_file = 0;
    DIR *d = opendir("./server_f");
    struct dirent *dir;
    if(d){
        while((dir = readdir(d)) != NULL){
            if(strcmp(filenm, dir->d_name) == 0){
                find_file = 1;
                printf("Finded out the file, file name: %s\n", filenm);
                break;
            }
        }
        closedir(d);
    }
    if(find_file == 1){
        exist[0] = '1';//有找到
    }
    else{
        exist[0] = '0';
    }
}
void print_seg(segment s_tmp){
    printf("-----------\n");
    printf("s_tmp.head.length = %d\n", s_tmp.head.length);
    printf("s_tmp.head.seqNumber = %d\n",s_tmp.head.seqNumber);
    printf("s_tmp.head.ackNumber = %d\n",s_tmp.head.ackNumber);
    printf("s_tmp.head.fin = %d\n",s_tmp.head.fin);
    printf("s_tmp.head.syn = %d\n",s_tmp.head.syn);
    printf("s_tmp.head.ack = %d\n",s_tmp.head.ack);
    //printf("s_tmp.data = %s\n", s_tmp.data);
    printf("-----------\n");

}
int total_data = 0;
int winSize =  1;
int thrhold = 16;
segment big_seg [500000]; // 改成malloc
int big_seg_resend [500000];//判斷是不是要resend
//segment * big_seg;
int big_seg_cnt = 0;

segment constr_pkt(char buffer[], int ox_final, int data_length){
    segment s_tmp;
    total_data ++;    
    memset(&s_tmp, 0, sizeof(s_tmp));

    memcpy(s_tmp.data ,buffer, data_length);
    s_tmp.head.ackNumber = 0;
    s_tmp.head.syn = 0; // 直接設成0
    s_tmp.head.ack = 0;
    s_tmp.head.fin = ox_final;
    s_tmp.head.seqNumber = total_data; //第幾份data
    s_tmp.head.length = data_length;

    return s_tmp;
}
void sent_pkt(segment s_send, int ox_final, int senderSocket, sockaddr_in agent, socklen_t agent_size, int num_of_pkt ){
	int segment_size = sizeof(s_send);
    //printf("segment_size = %d\n", segment_size);
    if(ox_final == 0){//尚未終止
        sendto(senderSocket, &s_send, segment_size, 0, (struct sockaddr *)&agent, agent_size);
        if(big_seg_resend[num_of_pkt] == 0){
            printf("send     data   #%d,    winSize = %d\n", s_send.head.seqNumber, winSize);
        }else{
            printf("resnd    data   #%d,    winSize = %d\n", s_send.head.seqNumber, winSize);
        }
	}
	else{
        sendto(senderSocket, &s_send, segment_size, 0, (struct sockaddr *)&agent, agent_size);  
        printf("send     fin \n");
	}
}

int get_ack(int senderSocket, sockaddr_in agent, socklen_t agent_size){    //receive ack
    segment s_ack;
    int segment_size;
    memset(&s_ack, 0, sizeof(s_ack)); 
    segment_size = recvfrom(senderSocket, &s_ack, sizeof(s_ack), 0, (struct sockaddr *)&agent, &agent_size);
    if(s_ack.head.ack != 0){ // 是ack
        if(s_ack.head.fin == 0){ //是finack
            printf("recv     ack    #%d\n", s_ack.head.ackNumber);
            return s_ack.head.ackNumber;
        }else{
            printf("recv    finack\n");
            return -1;// 接到 fin ack
        }        
    }else{
        return 0;//沒有接到東西
    }
}
void data_proces(char buffer[], int data_length, int ox_final, int senderSocket, sockaddr_in agent, socklen_t agent_size, int last_flush){
    int cur_acknum = 1, get_acknum = 0;
    if (big_seg_cnt < winSize && last_flush == 0){ 
        big_seg_cnt ++;
        big_seg[big_seg_cnt] = constr_pkt(buffer, ox_final, data_length);
        big_seg_resend[big_seg_cnt] = 0;
    }
    else{ // 當搜集滿winSize後，開始弄 go back N
        if(last_flush == 1){
            winSize = 1;
        }
        int already_sent_pkt = 0;
        for(int j = 0; j < winSize; j++){//傳pkt
            sent_pkt(big_seg[j+1], ox_final, senderSocket, agent, agent_size, j+1); 
            big_seg_resend[j+1] = 1;
        }
            
        cur_acknum = big_seg[1].head.seqNumber;//得到第一個的seqNum // big_seg_cnt: 0不存東西
        for(int j = 0; j < winSize; j++){//收ack
            get_acknum = get_ack(senderSocket, agent, agent_size);
            if(get_acknum == 0){
                printf("time     out,\n");
                break; //time out
            }
            if(cur_acknum == get_acknum){
                already_sent_pkt ++;
                cur_acknum ++;
            }
        }
            
        if(last_flush == 0){ //處理
            for(int k = (winSize+1); k < big_seg_cnt; k++){
                big_seg_resend[k] = 0;
            }
            big_seg_cnt = (big_seg_cnt - already_sent_pkt + 1);//移動  要小心這個＋1是要配合下面for的小於的關係

            for(int k = 1; k < big_seg_cnt; k++){
                big_seg[k] = big_seg[k + already_sent_pkt];
                big_seg_resend[k] = big_seg_resend[ k + already_sent_pkt];
            }

            if(already_sent_pkt == winSize){
                if(winSize < thrhold){
                    winSize *= 2; // 一次放大兩倍
                }else{
                    winSize ++; // 一次加一個
                }
            }else{//有pkt loss
                if( (winSize / 2) > 1 ){
                    thrhold = (winSize/2);
                }else{
                    thrhold = 1;
                }
                winSize = 1; //winSize變成1
            } 

            big_seg[big_seg_cnt] = constr_pkt(buffer, ox_final, data_length);  //新增要添入的
            big_seg_resend[big_seg_cnt] = 0;
        }else{ //last_flush == 1
            for(int k = (winSize+1); k < big_seg_cnt; k++){
                big_seg_resend[k] = 0;
            }

            big_seg_cnt = (big_seg_cnt - already_sent_pkt + 1);//移動
            
            for(int k = 1; k < big_seg_cnt; k++){
                big_seg[k] = big_seg[k + already_sent_pkt];
                big_seg_resend[k] = big_seg_resend[k + already_sent_pkt];
            }

        }
    }
}

unsigned long shw_hash(unsigned char *str, size_t length) {
    unsigned long hash = 5381;
    for (size_t i = 0; i < length; i++) {
        hash = ((hash << 5) + hash) + int(*(str + i)); /* hash * 33 + c */
    }
    return hash;
}

void shw_hang() {
    char buffer[10];
    read(STDIN_FILENO, buffer, 10);
}

void shw_hook_start(int width, int height) {
    printf("width = %d, height = %d\n", width, height);
    //shw_hang();
}

void shw_hook_mid(unsigned char *buffer_u, int imgSize) {
    // unsigned long h = shw_hash(buffer_u, imgSize);
    // printf("shw: imgSize = %d, hash = %lu\n", imgSize, h);
    static int cnt = 0;
    char fname[128];
    sprintf(fname, "shw/server_%d", cnt++);

    FILE *fp;
    if ((fp = fopen(fname, "wb")) == NULL){
        perror("fopen error");
        exit(1);
    }
    int numbytes = fwrite(buffer_u, sizeof(char), imgSize, fp);
    printf("imgSize = %d, fwrite numbytes = %d\n", imgSize, numbytes);
    //shw_hang();
    fclose(fp);
}

void shw_hook_end() {
    //exit(2);
}

void play_video(char filenm[], int senderSocket, sockaddr_in agent, socklen_t agent_size){
    int ox_final = 0, last_flush = 0;
    //get file
    string str_1 = "./server_f/";
    string str_2 = filenm;
    string str_3 = str_1 + str_2;
    // server
    VideoCapture cap(str_3.c_str());
    Mat imgServer;
    // get the resolution of the video
    int width = cap.get(CV_CAP_PROP_FRAME_WIDTH);//width
    int height = cap.get(CV_CAP_PROP_FRAME_HEIGHT);//height
    std::string wid_str = std::to_string(width);
    std::string hei_str = std::to_string(height);

    char buffer[BUFF_SIZE];
    bzero(buffer, sizeof(char)*BUFF_SIZE);
    
    strcpy(buffer, wid_str.c_str());
    data_proces(buffer, strlen(buffer), ox_final, senderSocket, agent, agent_size, last_flush);
    bzero(buffer, sizeof(char)*BUFF_SIZE);

    strcpy(buffer, hei_str.c_str());
    data_proces(buffer, strlen(buffer), ox_final, senderSocket, agent, agent_size, last_flush);
    bzero(buffer, sizeof(char)*BUFF_SIZE);
    
    imgServer = Mat::zeros(height, width, CV_8UC3);    
    if(!imgServer.isContinuous()){
         imgServer = imgServer.clone();
    }

    //shw_hook_start(width, height);

    int big_cnt = 0, a = 6;
    //while(a--){
    while(1){ // shw
        cap >> imgServer;
        int imgSize = imgServer.total() * imgServer.elemSize();

        //傳 imgSize
        std::string imgSize_str = std::to_string(imgSize);
        strcpy(buffer, imgSize_str.c_str());
        data_proces(buffer, strlen(buffer),ox_final, senderSocket, agent, agent_size, last_flush);
        bzero(buffer, sizeof(char)*BUFF_SIZE);
        if(imgSize < 1){
            break;
        }
        
        // copy a frame to the buffer
        uchar *buffer_u = (uchar*)malloc(imgSize * sizeof(uchar));
        bzero(buffer_u, sizeof(uchar)* imgSize);
        memcpy(buffer_u, imgServer.data, imgSize);

        //shw_hook_mid(buffer_u, imgSize);

        int accum = 0; 
        int count = ((imgSize/BUFF_SIZE) + 1);//count: 每一針總共要傳幾次
        int last_of_aframe = imgSize % BUFF_SIZE;//每一針的最後一個pkt是多少bytes
        while(count --){
            if(count > 0){
                memcpy(buffer, buffer_u + accum, BUFF_SIZE);
                data_proces(buffer, BUFF_SIZE, ox_final, senderSocket, agent, agent_size, last_flush);
                accum += sizeof(buffer);
                bzero(buffer, sizeof(char)*BUFF_SIZE);
            }else{ // count == 1, 同一針的最後一個
                memcpy(buffer, buffer_u + accum, last_of_aframe);
                data_proces(buffer, last_of_aframe, ox_final, senderSocket, agent, agent_size, last_flush);
                accum += sizeof(buffer);
                bzero(buffer, sizeof(char)*BUFF_SIZE);
            }
        }        
        bzero(buffer_u, sizeof(uchar)* imgSize);
        free(buffer_u);
        big_cnt ++;
        //shw_hook_end();
    } 
    cap.release();
    //將尚未送出的東西全部送出去的機制
    last_flush = 1;
    int num_flush = big_seg_cnt;
    //printf("num_flush = %d\n", num_flush);
    for(int k = 0; k < num_flush; k++){
        data_proces(buffer, 0,ox_final, senderSocket, agent, agent_size, last_flush);   
    }

    // final segment
    ox_final = 1;
    segment final = constr_pkt(buffer, ox_final, 1);
    int final_ack = 0;
    while(final_ack != -1){
        sent_pkt(final, ox_final, senderSocket, agent, agent_size, -1); // fin ack
        final_ack = get_ack( senderSocket, agent, agent_size);
    }
}
void send_file(char filenm[], int senderSocket, sockaddr_in agent, socklen_t agent_size){
    int numbytes;
    int ox_final = 0;
    int last_flush = 0;
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
        data_proces(tmp_buf, numbytes, ox_final, senderSocket, agent, agent_size, last_flush);
        //numbytes = write(remoteSocket, tmp_buf, BUFF_SIZE);
        printf("Sending %d bytes\n",numbytes);
        bzero(tmp_buf, sizeof(char)* BUFF_SIZE);
    }
    last_flush = 1;
    int num_flush = big_seg_cnt;
    printf("num_flush = %d\n", num_flush);
    for(int k = 0; k < num_flush; k++){
        data_proces(tmp_buf, numbytes, ox_final, senderSocket, agent, agent_size, last_flush);   
    }

    // final segment
    ox_final = 1;
    segment final = constr_pkt(tmp_buf, ox_final, 1);
    int final_ack = 0;
    while(final_ack != -1){
        sent_pkt(final, ox_final, senderSocket, agent, agent_size, -1); // fin ack
        final_ack = get_ack( senderSocket, agent, agent_size);
    }    
}

int main(int argc , char *argv[]){
	//前置作業，勿動
    int status;
    status = mkdir("./server_f", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    char Buf[BUFF_SIZE];//Buf用來存放read到的資料
    char filenm[BUFF_SIZE];//file name buffer
    FILE *fp;

    //本次作業
    int senderSocket;// local: server, remote: agent 
    struct sockaddr_in sender, agent, receiver, tmp_addr;
    socklen_t sender_size, recv_size, tmp_size, agent_size;
    char ip[3][50];
    int port[3];
    //big_seg = (segment*)malloc(2000000 * sizeof(segment));    
    if(argc != 6){
        fprintf(stderr,"用法: %s <sender IP> <recv IP> <sender port> <agent port> <recv port>\n", argv[0]);
        fprintf(stderr, "例如: ./server local local 8887 8888 8889\n");
        exit(1);
    } else {
        setIP(ip[0], argv[1]); //sender IP
        setIP(ip[1], "local"); //agent IP
        setIP(ip[2], argv[2]); //recv IP

        sscanf(argv[3], "%d", &port[0]); //sender port
        sscanf(argv[4], "%d", &port[1]); //agent port
        sscanf(argv[5], "%d", &port[2]); //recv port
    }


    /*Create UDP socket, TCP: AF_INET , SOCK_STREAM , 0*/
    if ((senderSocket = socket(PF_INET, SOCK_DGRAM, 0)) < 0){
        perror("socket create error!");
        exit(1);
    }

    /*Configure settings in local struct*/
    sender.sin_family = AF_INET;
    sender.sin_port = htons(port[0]);
    sender.sin_addr.s_addr = inet_addr(ip[0]);
    memset(sender.sin_zero, '\0', sizeof(sender.sin_zero));  
    
    if(bind(senderSocket,(struct sockaddr *)&sender, sizeof(sender)) < 0) {
        perror("Bind error!");
        exit(1);
    }

    struct timeval read_timeout;
    read_timeout.tv_sec = 0;
    read_timeout.tv_usec = 500000;
    setsockopt(senderSocket, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof read_timeout);
  
    /*Configure settings in agent struct*/
    agent.sin_family = AF_INET;
    agent.sin_port = htons(port[1]);
    agent.sin_addr.s_addr = inet_addr(ip[1]);
    memset(agent.sin_zero, '\0', sizeof(agent.sin_zero));    

    /*Configure settings in receiver struct*/
    receiver.sin_family = AF_INET;
    receiver.sin_port = htons(port[2]);
    receiver.sin_addr.s_addr = inet_addr(ip[2]);
    memset(receiver.sin_zero, '\0', sizeof(receiver.sin_zero)); 

    /*Initialize size variable to be used later on*/
    sender_size = sizeof(sender);
    recv_size = sizeof(receiver);
    agent_size = sizeof(agent);
    tmp_size = sizeof(tmp_addr);


    //開始測試
    //printf("Start to test.\n");

    //while(1){
        bzero(Buf, sizeof(char)* BUFF_SIZE); 
        //if(Buf[0] == 'p' && Buf[1] == 'l' && Buf[2] == 'a' && Buf[3] == 'y') { 
        bzero(filenm, sizeof(char)* BUFF_SIZE);
            
        strcpy(filenm, "tmp.mpg");
        printf("playing: tmp.mpg\n");

        char exist[2];
        bzero(exist, sizeof(char)* 2);
        find_file(filenm, exist);
        segment s_file_name;
        memset(&s_file_name, 0, sizeof(s_file_name));

        int segment_size = recvfrom(senderSocket, &s_file_name, sizeof(s_file_name), 0, (struct sockaddr *)&agent, &agent_size);
        memcpy(filenm, s_file_name.data, s_file_name.head.length);
        if(exist[0] == '1'){
            //printf("Playing video starts...\n");
            play_video( filenm, senderSocket,agent, agent_size);
        }
        /*
        }
        else{
            //strcpy(filenm, "old_server.cpp");
            strcpy(filenm, "tmp.mpg");
            send_file(filenm, senderSocket, agent, agent_size);
            //printf("Command not found.\n");
        }
        */
	//}

    return 0;
}



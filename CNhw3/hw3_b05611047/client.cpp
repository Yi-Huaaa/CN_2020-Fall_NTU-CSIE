#include <iostream>
#include <sys/socket.h> 
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h> 
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include "opencv2/opencv.hpp"
#include <string>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>
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

const size_t BIG_BUFF_SZ = 32; // shw
int testing_fin_ack = 0;
segment s_tmp;
segment BIG_BUFF[BIG_BUFF_SZ]; // 最大是32
int big_buf_cnt = 0;
int last_ack_pkt = 0; // 類似total_data的變數
void flush_buf(){
    printf("flush\n");
    for(int i = 0; i < BIG_BUFF_SZ; i++){
        memset(&BIG_BUFF[i], 0, sizeof(BIG_BUFF[i])); 
    }
    big_buf_cnt = 0;
}
int get_pkt(int receiverSocket, sockaddr_in agent, socklen_t agent_size){
    //receive pkt
    int segment_size;
    memset(&s_tmp, 0, sizeof(s_tmp)); 
    segment_size = recvfrom(receiverSocket, &s_tmp, sizeof(s_tmp), 0, (struct sockaddr *)&agent, &agent_size);
    if(big_buf_cnt < BIG_BUFF_SZ){
        BIG_BUFF[big_buf_cnt] = s_tmp;
        if(s_tmp.head.fin != 1){
            printf("recv    data    #%d\n", s_tmp.head.seqNumber);
            //print_seg(s_tmp);
            return 1;//代表成功接到pkt
        }else{
            printf("recv    fin\n");
            testing_fin_ack = 1;
            //print_seg(s_tmp);
            return -1;
        }
    }else{ // buffer overflow
        if(s_tmp.head.fin != 1){
            printf("drop    data    #%d\n", s_tmp.head.seqNumber);
            //print_seg(s_tmp);
            return 0;//沒有成功接到pkt, ack要回傳上一個            
        }else{
            BIG_BUFF[big_buf_cnt] = s_tmp;
            printf("recv    fin\n");
            testing_fin_ack = 1;
            //print_seg(s_tmp);
            return -1;
        }
    }
}

segment s_ack;
void send_ack(int receiverSocket, segment s_needack, sockaddr_in agent, socklen_t agent_size){
    // send back ack
    int segment_size;
    memset(&s_ack, 0, sizeof(s_ack)); 
    segment_size = sizeof(s_ack);
    s_ack.head.length = 0;
    s_ack.head.seqNumber = 0;

    if( (last_ack_pkt + 1) ==  s_needack.head.seqNumber){//接到我要的
        s_ack.head.ackNumber = s_needack.head.seqNumber;
        last_ack_pkt ++;
    }else if ( (last_ack_pkt + 1) > s_needack.head.seqNumber ){//接到已經有的
        s_ack.head.ackNumber = s_needack.head.seqNumber;
    }else{//接到超過我要的: s_needack.head.seqNumber
        s_ack.head.ackNumber = last_ack_pkt;
    }

    s_ack.head.syn = 0;
    s_ack.head.ack = 1;
    if(s_needack.head.fin == 0){
        s_ack.head.fin = 0;
        sendto(receiverSocket, &s_ack, segment_size, 0, (struct sockaddr *)&agent, agent_size);
        printf("send    ack     #%d\n", s_ack.head.ackNumber);
    }else{
        s_ack.head.fin = 1;
        sendto(receiverSocket, &s_ack, segment_size, 0, (struct sockaddr *)&agent, agent_size);
        printf("send    finack\n");
    }
}

void data_proces(int ox_final,int receiverSocket, sockaddr_in agent, socklen_t agent_size){
    int old_last_ack_pkt = last_ack_pkt;
    while(old_last_ack_pkt == last_ack_pkt){ //last_ack_pkt被++的時候就是有回傳對的ack的時候
        int get = get_pkt(receiverSocket, agent, agent_size);//get == 1, 成功接到data, == 0 滿了倍flush掉了
        if(get == 1 || get == -1){
            send_ack(receiverSocket, BIG_BUFF[big_buf_cnt], agent, agent_size);
        }
        else{// buffer overflow
            send_ack(receiverSocket, BIG_BUFF[big_buf_cnt], agent, agent_size);
            flush_buf();
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

void shw_hook(unsigned char *buffer_u, int imgSize) {
    // unsigned long h = shw_hash(iptr, imgSize);
    // printf("shw: imgSize = %d, hash = %lu\n", imgSize, h);
    // char buffer[10];
    // read(STDIN_FILENO, buffer, 10);
    static int cnt = 0;
    char fname[128];
    sprintf(fname, "shw/client_%d", cnt++);

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

void playing_video(int receiverSocket, sockaddr_in agent, socklen_t agent_size){
    int ox_final = 0;
    Mat imgClient;

    int width, height, imgSize;
    char buffer[BUFF_SIZE];
    bzero(buffer, sizeof(char)*BUFF_SIZE);
    
    data_proces(ox_final, receiverSocket, agent, agent_size);
    strcpy(buffer, BIG_BUFF[big_buf_cnt].data);
    big_buf_cnt++;
    string str_wid = buffer;
    width = atoi(str_wid.c_str());
    bzero(buffer, sizeof(char)*BUFF_SIZE);
    
    data_proces(ox_final, receiverSocket, agent, agent_size);
    strcpy(buffer, BIG_BUFF[big_buf_cnt].data);   
    big_buf_cnt++;
    string str_hei = buffer;
    height = atoi(str_hei.c_str());
    bzero(buffer, sizeof(char)*BUFF_SIZE);

    imgClient = Mat::zeros(height, width, CV_8UC3);//長、寬
    if(!imgClient.isContinuous()){
         imgClient = imgClient.clone();
    }

    //shw_hook_start(width, height);

    int big_cnt = 0, a = 6;
    //while(a--){
    while(1){ // shw
        //先接imgSize
        data_proces(ox_final, receiverSocket, agent, agent_size);
        memcpy(buffer, BIG_BUFF[big_buf_cnt].data, BIG_BUFF[big_buf_cnt].head.length);
        big_buf_cnt++;
        string imgSize_str = buffer;
        imgSize = atoi(imgSize_str.c_str());
        //printf("get imgSize: %d\n", imgSize);
        bzero(buffer, sizeof(char)*BUFF_SIZE);
        if(imgSize < 1){
            break;
        }

        //再接frame
        uchar *buffer_u = (uchar*)malloc( imgSize * sizeof(uchar));
        bzero(buffer_u, sizeof(uchar)*imgSize);
        int accum = 0;
        //printf("start receiving file...\n");
        int count = ((imgSize/BUFF_SIZE) + 1);//count: 每一針總共要傳幾次
        int last_of_aframe = imgSize % BUFF_SIZE;//每一針的最後一個pkt是多少bytes
        while(count --){
            if(count > 0){ //前面幾包
                data_proces(ox_final, receiverSocket, agent, agent_size);
                memcpy(buffer_u + accum, BIG_BUFF[big_buf_cnt].data, BUFF_SIZE);//cp後到前
                accum += BUFF_SIZE;
                big_buf_cnt++;
            }else{ //最後一包
                data_proces(ox_final, receiverSocket, agent, agent_size);
                memcpy(buffer_u + accum, BIG_BUFF[big_buf_cnt].data, last_of_aframe);//cp後到前
                accum += last_of_aframe;
                big_buf_cnt++;
            }
        }
        // copy a frame from buffer to the container on client
        uchar *iptr = imgClient.data;
        memcpy(iptr,buffer_u,imgSize);
        //shw_hook(iptr, imgSize);
        imshow("Client Video", imgClient);
        char c = (char)waitKey(33.3333);
        big_cnt ++;
        //printf("big_cnt = %d\n", big_cnt);
        free(buffer_u);
    }
    destroyAllWindows();
    segment s_final;
    memset(&s_final, 0, sizeof(s_final)); 
    s_final.head.fin = 1;
    while(ox_final != -1){
        ox_final = get_pkt(receiverSocket, agent, agent_size);
        if(ox_final == -1)
            send_ack(receiverSocket, s_final, agent, agent_size);
    }
    flush_buf();
}

void rec_file(char filenm[], int receiverSocket, sockaddr_in agent, socklen_t agent_size){
        int ox_final = 0;
        FILE *fp;
        int numbytes;
        string str_1 = "./client_f/";
        string str_2 = filenm;
        string str_3 = str_1 + str_2;
        if ((fp = fopen(str_3.c_str(), "wb")) == NULL){
            perror("fopen error");
            exit(1);
        }
        while(1){
            data_proces(ox_final, receiverSocket, agent, agent_size);
            printf("read %d bytes, ", numbytes);
            numbytes = fwrite(BIG_BUFF[big_buf_cnt].data, sizeof(char), BIG_BUFF[big_buf_cnt].head.length, fp);
            if(testing_fin_ack == 1){
                break;
            }
            printf("fwrite %d bytes\n", numbytes);
        }
        fclose(fp);
        flush_buf();
}

int main(int argc , char *argv[]){
    int status;
    int receiverSocket;
    status = mkdir("./client_f", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    char instruction[BUFF_SIZE];
    char filenm[BUFF_SIZE];

    //本次作業
    struct sockaddr_in sender, agent, receiver, tmp_addr;
    socklen_t sender_size, recv_size, tmp_size, agent_size;
    char ip[3][50];
    int port[3];
    
    if(argc != 6){
        fprintf(stderr,"用法: %s <sender IP> <recv IP> <sender port> <agent port> <recv port>\n", argv[0]);
        fprintf(stderr, "例如: ./client local local 8887 8888 8889\n");
        exit(1);
    } else {
        setIP(ip[0], argv[1]); //sender IP
        setIP(ip[1], "local"); //agent IP
        setIP(ip[2], argv[2]); //recv IP

        sscanf(argv[3], "%d", &port[0]); //sender port
        sscanf(argv[4], "%d", &port[1]); //agent port
        sscanf(argv[5], "%d", &port[2]); //recv port
    }

    /*Create UDP socket, TCP: AF_INET , UDP: SOCK_STREAM , 0*/
    if ((receiverSocket = socket(PF_INET, SOCK_DGRAM, 0)) < 0){
        perror("socket create error!");
        exit(1);
    }

    /*Configure settings in sender struct*/
    sender.sin_family = AF_INET;
    sender.sin_port = htons(port[0]);
    sender.sin_addr.s_addr = inet_addr(ip[0]);
    memset(sender.sin_zero, '\0', sizeof(sender.sin_zero));  

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

    if(bind(receiverSocket,(struct sockaddr *)&receiver, sizeof(receiver)) < 0) {
        perror("Bind error!");
        exit(1);
    }

    /*Initialize size variable to be used later on*/
    sender_size = sizeof(sender);
    recv_size = sizeof(receiver);
    agent_size = sizeof(agent);
    tmp_size = sizeof(tmp_addr);


    //開始測試
    printf("Giving instruction: play <filename>\n");
    bzero(instruction, sizeof(char)* BUFF_SIZE);
    bzero(filenm, sizeof(char)* BUFF_SIZE);
    scanf("%s %s", instruction, filenm);
    //strcpy(filenm, "old_server.cpp");
    //rec_file(filenm, receiverSocket, agent, agent_size);
    segment s_file_name;
    memset(&s_file_name, 0, sizeof(s_file_name));
    memcpy(s_file_name.data, filenm, BUFF_SIZE);
    s_file_name.head.length = strlen(filenm);
    s_file_name.head.seqNumber = 0;
    s_file_name.head.ackNumber = 0;
    s_file_name.head.fin = 0;
    s_file_name.head.syn = 0;
    s_file_name.head.ack = 0;
    int segment_size = sizeof(s_file_name);
    sendto(receiverSocket, &s_file_name, segment_size, 0, (struct sockaddr *)&agent, agent_size);
    playing_video(receiverSocket, agent, agent_size);

    return 0;
}

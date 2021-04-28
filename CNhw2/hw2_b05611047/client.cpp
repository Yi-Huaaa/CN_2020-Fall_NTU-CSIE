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
#define BUFF_SIZE 1024
using namespace std;
using namespace cv;


void rec_ls(int localSocket, char instruction[]){
    int rec;
    bzero(instruction, sizeof(char)* BUFF_SIZE);
    while(1){
        rec = read(localSocket, instruction, BUFF_SIZE);
        for(int i = 0; i < strlen(instruction); i++){
            //printf("%c", instruction[i]);
        }
        //printf(" ");
        if(rec <= 0){
            break;
        }
        bzero(instruction, sizeof(char)* BUFF_SIZE);
    }
}
void find_file(char filenm[], char exist[]){
    int find_file = 0;
    DIR *d = opendir("./client_f");
    struct dirent *dir;
    if(d){
        while((dir = readdir(d)) != NULL){
            if(strcmp(filenm, dir->d_name) == 0){
                find_file = 1;
                //printf("Finded out the file, file name: %s\n", filenm);
                break;
            }
        }
        closedir(d);
    }
    if(find_file == 1){
        exist[0] = '1';
        //printf("exist[0] = %c, Finding file successful.\n",exist[0]);
    }
    else{
        exist[0] = '0';
        //printf("exist[0] = %c, Finding file failed.\n",exist[0]);
    }
}
void rec_file(char exist[], int localSocket, char instruction[], char filenm[]){
    if(exist[0] == '0'){//檔案不存在
        //printf("the %s does not exist.\n", filenm);
    }
    else{//檔案存在
        FILE *fp;
        int numbytes;
        string str_1 = "./client_f/";
        string str_2 = filenm;
        string str_3 = str_1 + str_2;
        //fp = fopen(str_3.c_str(), "rb");
        if ((fp = fopen(str_3.c_str(), "wb")) == NULL){
            perror("fopen error");
            exit(1);
        }
        while(1){
            numbytes = read(localSocket, instruction, BUFF_SIZE);
            //printf("read %d bytes, ", numbytes);
            if(numbytes <= 0){
                break;
            }
            numbytes = fwrite(instruction, sizeof(char), numbytes, fp);
            //printf("fwrite %d bytes\n", numbytes);
            bzero(instruction, sizeof(char)* BUFF_SIZE);
        }
        fclose(fp);
    }
}
void send_file(char filenm[], int localSocket){
    int numbytes;
    FILE *fp;
    char tmp_buf[BUFF_SIZE];
    string str_1 = "./client_f/";
    string str_2 = filenm;
    string str_3 = str_1 + str_2;
    //fp = fopen(str_3.c_str(), "rb");
    fp = fopen(str_3.c_str(), "rb");
    bzero(tmp_buf, sizeof(char)* BUFF_SIZE); 
    
    while(!feof(fp)){
        numbytes = fread(tmp_buf, sizeof(char), sizeof(tmp_buf), fp);
        //printf("fread %d bytes, ", numbytes);
        numbytes = write(localSocket, tmp_buf, BUFF_SIZE);
        //printf("Sending %d bytes\n", numbytes);
        bzero(tmp_buf, sizeof(char)* BUFF_SIZE);
    }
}
void cmd_not_fd(){
    //printf("Command not found.\n");    
}
int judge_mpg(char filenm[]){
//If the video file is not a “.mpg” file while playing a video file, please print out “The ‘<videofile>’ is not a mpg file.”
    int i;
    for(i = 0; i < strlen(filenm); i++)
        if(filenm[i] == '.')
            break;
    if(filenm[i+1] == 'm' && filenm[i+2] == 'p' && filenm[i+3] == 'g' && strlen(filenm) == (i+4))
        return 1;
    else
        return 0;
}
uchar * BIG_BUFFER;
uchar * BIG_BUFFER_2;
int width, height, imgSize, IMGSZ;
char c_buf [2] = {'a', 'a'}; 
char c;
int can_play_1 = 0, can_play_2 = 0;//是否可以開始播影片
int tmp_played;

void *doInChildThread(void *arg){
    while( 1 ){
        if(can_play_1 == 0) {
            sleep(5);
        }
        else{
            break;
        }
    }
    int count = 0;
    Mat imgClient;
    imgClient = Mat::zeros(height, width, CV_8UC3);//長、寬
    // ensure the memory is continuous (for efficiency issue.)
    if(!imgClient.isContinuous()){
        imgClient = imgClient.clone();
    }
    while(count < 100){
        uchar *iptr = imgClient.data;
        memcpy(iptr,BIG_BUFFER + (count*IMGSZ), IMGSZ);
        //playing video
        imshow("Child Video", imgClient);
        count ++;
        char aaa = (char)waitKey(33.3333);
        if(aaa == 27){
            c = 27;
            break;
        }
    }
    can_play_1 = 0;
    tmp_played ==1;
    
    count = 0;  
    while(count < 100){
        uchar *iptr = imgClient.data;
        memcpy(iptr,BIG_BUFFER_2 + (count*IMGSZ),IMGSZ);
        //playing video
        imshow("Child Video", imgClient);
        count ++;
        char aaa = (char)waitKey(33.3333);
        if(aaa == 27){
            c = 27;
            sleep(2);
            destroyAllWindows();
            break;
        }
    }
    can_play_2 = 0;
    tmp_played = 2;
    //printf("Out of child_2.\n");
    while(c!=27){
        if (can_play_1 != 0 && tmp_played == 2) {
            count = 0;  
            while(count < 100){
                uchar *iptr = imgClient.data;
                memcpy(iptr,BIG_BUFFER + (count*IMGSZ),IMGSZ);
                //playing video
                imshow("Child Video", imgClient);
                count ++;
                ////printf("while_1: 66666\n");
                char aaa = (char)waitKey(33.3333);
                ////printf("aaa = %c\n", aaa);
                if(aaa == 27){
                    c = 27;
                    //printf("byebye.\n");
                    sleep(2);
                    destroyAllWindows();
                    break;
                }
            }
            can_play_1 = 0;
            tmp_played = 1;
        }
        //if (can_play_1 == 0 && can_play_2 != 0){
        else if (can_play_2 != 0 && tmp_played == 1) {
            //printf("2 can play, but 1 can't\n");
            count = 0;  
            //printf("playing 2, width = %d, height = %d, IMGSZ = %d\n", width, height, IMGSZ);
            while(count < 100){
                uchar *iptr = imgClient.data;
                memcpy(iptr,BIG_BUFFER_2 + (count*IMGSZ),IMGSZ);
                //playing video
                imshow("Child Video", imgClient);
                count ++;
                ////printf("while_2: 66666\n");
                char aaa = (char)waitKey(33.3333);
                ////printf("aaa = %c\n", aaa);
                if(aaa == 27){
                    c = 27;
                    //printf("byebye.\n");
                    sleep(2);
                    destroyAllWindows();
                    break;
                }
            }
            can_play_2 = 0;
            tmp_played = 2;
        }
        //if(can_play_1 == 0 && can_play_2 == 0){
        else {
            char aaa = (char)waitKey(33.3333);
            //printf("aaa = %c\n", aaa);
            if(aaa == 27){
                c = 27;
                //printf("byebye.\n");
                sleep(2);
                destroyAllWindows();
                break;
            }
            //printf("sleeping\n");
            //sleep(1);
        }
    }
    sleep(2);
    destroyAllWindows();
}

void cycle_play(int num, uchar * BIG_BUF, int localSocket){
    char buffer[BUFF_SIZE];
    bzero(buffer, sizeof(char)*BUFF_SIZE);//接寬    
    int rec = 1, sent;
    int onehd_times = 0;    
    int accum = 0, act_accum = 0;
    while(onehd_times < 100 && rec > 0) {
        rec = read(localSocket, buffer, BUFF_SIZE);
        string imgSize_str = buffer;
        imgSize = atoi(imgSize_str.c_str());
        bzero(buffer, sizeof(char)*BUFF_SIZE);
        accum = 0;
        while(accum < imgSize && rec > 0){
            if( (imgSize - accum) < 1024){
                rec = read(localSocket,BIG_BUF + act_accum, (imgSize - accum));
            }
            else{
                rec = read(localSocket, BIG_BUF + act_accum, 1024);
            }
            accum += rec;
            act_accum += rec;
        }
        onehd_times ++;

        if(c == 27){
            c_buf[0] = c;
            sent = write(localSocket, c_buf, 2);
            bzero(c_buf, sizeof(char)*2);
            break;
        }
        else{
            sent = write(localSocket, c_buf, 2);
        }
    }
    if(num == 1){    
        can_play_1 = 1;  
        ////printf("supply 1 ok, onehd_times = %d\n",onehd_times);
    }  
    else{
        can_play_2 = 1;
        ////printf("supply 2 ok, onehd_times = %d\n",onehd_times);

    }
}

void playing_video(int localSocket){
    //先接長寬
    char buffer[BUFF_SIZE];
    bzero(buffer, sizeof(char)*BUFF_SIZE);//接寬
    int rec = read(localSocket, buffer, BUFF_SIZE);
    string str_wid = buffer;
    width = atoi(str_wid.c_str());
    bzero(buffer, sizeof(char)*BUFF_SIZE);//接長
    rec = read(localSocket, buffer, BUFF_SIZE);
    string str_hei = buffer;
    height = atoi(str_hei.c_str());
    bzero(buffer, sizeof(char)*BUFF_SIZE);
    ////printf("height = %d, width = %d\n", height, width);
    int onehd_times = 0; 
    //先接一偵：
    ////printf("master: receiving imgSize...\n");
    rec = read(localSocket, buffer, BUFF_SIZE);
    string imgSize_str = buffer;
    imgSize = atoi(imgSize_str.c_str());
    IMGSZ = imgSize; 
    ////printf("master_get imgSize = %d,  IMGSZ = %d\n", imgSize, IMGSZ);
    bzero(buffer, sizeof(char)*BUFF_SIZE);
    //再接frame
    BIG_BUFFER = (uchar*)malloc( (imgSize*100) * sizeof(uchar));
    bzero(BIG_BUFFER, sizeof(uchar)*(imgSize*100));
    int accum = 0, act_accum = 0;
    ////printf("start receiving file, the first frame...\n");
    //一次只讀 1024 bytes
    while(accum < imgSize && rec > 0){
        if( (imgSize - accum) < 1024){
            rec = read(localSocket,BIG_BUFFER + accum, (imgSize - accum));
        }
        else{
            rec = read(localSocket, BIG_BUFFER + accum, 1024);
        }
        accum += rec;
        act_accum += rec;
    }
    onehd_times ++;
    //送char C給server端
    int sent = write(localSocket, c_buf, 2);
    //在接99偵
    while(onehd_times < 100 && rec > 0) {
        //先接imgSize
        ////printf("receiving imgSize, second part, onehd_times = %d...\n", onehd_times);
        rec = read(localSocket, buffer, BUFF_SIZE);
        string imgSize_str = buffer;
        imgSize = atoi(imgSize_str.c_str());
        ////printf("get imgSize: %d\n", imgSize);
        bzero(buffer, sizeof(char)*BUFF_SIZE);
        //再接frame
        ////printf("start receiving file, second part...\n");
        accum = 0;
        while(accum < imgSize && rec > 0){
            if( (imgSize - accum) < 1024){
                rec = read(localSocket,BIG_BUFFER + act_accum, (imgSize - accum));
            }
            else{
                rec = read(localSocket, BIG_BUFFER + act_accum, 1024);
            }
            accum += rec;
            act_accum += rec;
        }
        ////printf("onehd_times = %d, accum = %d,act_accum = %d,rec = %d\n",onehd_times, accum,act_accum,rec);
        ////printf("(act_accum/imgSize) = %d\n", (act_accum/imgSize));
        onehd_times ++;

        if(c == 27){
            c_buf[0] = c;
            sent = write(localSocket, c_buf, 2);
            bzero(c_buf, sizeof(char)*2);
            break;
        }
        else{
            sent = write(localSocket, c_buf, 2);
        }
    }
    can_play_1 = 1;

    BIG_BUFFER_2 = (uchar*)malloc( (IMGSZ*100) * sizeof(uchar));
    bzero(BIG_BUFFER_2, sizeof(uchar)*(IMGSZ*100));
    onehd_times = 0;    
    accum = 0; 
    act_accum = 0;
    rec = 1;
    while(onehd_times < 100 && rec > 0) {
        //先接imgSize
        ////printf("receiving imgSize, second part, onehd_times = %d...\n", onehd_times);
        rec = read(localSocket, buffer, BUFF_SIZE);
        string imgSize_str = buffer;
        imgSize = atoi(imgSize_str.c_str());
        ////printf("get imgSize: %d\n", imgSize);
        bzero(buffer, sizeof(char)*BUFF_SIZE);
        //再接frame
        ////printf("start receiving file, second part...\n");
        accum = 0;
        while(accum < imgSize && rec > 0){
            if( (imgSize - accum) < 1024){
                rec = read(localSocket,BIG_BUFFER_2 + act_accum, (imgSize - accum));
            }
            else{
                rec = read(localSocket, BIG_BUFFER_2 + act_accum, 1024);
            }
            accum += rec;
            act_accum += rec;
        }
        ////printf("master2: onehd_times = %d, accum = %d,act_accum = %d,rec = %d\n",onehd_times, accum,act_accum,rec);
        ////printf("(act_accum/imgSize) = %d\n", (act_accum/imgSize));
        onehd_times ++;

        if(c == 27){
            c_buf[0] = c;
            sent = write(localSocket, c_buf, 2);
            bzero(c_buf, sizeof(char)*2);
            break;
        }
        else{
            sent = write(localSocket, c_buf, 2);
        }
    }
    can_play_2 = 1;

    //printf("master_2 100 over and onehd_times = %d\n", onehd_times);
    //接下來就是循環一直接
    while(c != 27){
        //printf("master: can_play_1 = %d, can_play_2 = %d\n", can_play_1, can_play_2);
        if(can_play_1 == 0 && can_play_2 != 0){
            //printf("master: get in to the first supply 1 time\n");
            cycle_play(1, BIG_BUFFER, localSocket);
        }
        if (can_play_1 != 0 && can_play_2 ==0){
            //printf("master: get in to the first supply 2 time\n");
            cycle_play(2, BIG_BUFFER_2, localSocket);
        }
        if (can_play_1 == 0 && can_play_2 == 0){
            if(tmp_played == 1){
                //printf("child just finish buf_1\n");
                cycle_play(2, BIG_BUFFER_2, localSocket);
                cycle_play(1, BIG_BUFFER, localSocket);
            }
            if(tmp_played == 2){
                //printf("child just finish buf_2\n");
                cycle_play(1, BIG_BUFFER, localSocket);
                cycle_play(2, BIG_BUFFER_2, localSocket);                
            }
        }
        //printf("master nothing, sleep 5 s\n");
        sleep(1);
    }
    //當break出這個while就代表影片結束播放了
    c_buf[0] = c;
    sent = write(localSocket, c_buf, 2);
    bzero(c_buf, sizeof(char)*2);
}

int main(int argc , char *argv[]){
    int status;
    status = mkdir("./client_f", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    
    char instruction[BUFF_SIZE];
    char filenm[BUFF_SIZE];
    string port_stg = argv[1];
    int port = atoi(port_stg.c_str());
    //printf("Get port number: %d\n",port);

    while(1){
        /*Get a socket into TCP/IP */
        int localSocket, recved;
        localSocket = socket(AF_INET , SOCK_STREAM , 0);
        if (localSocket < 0){
            perror("Create socket failed.");
            exit(1);
        }

        /* Bind to an arbitrary return address.*/
        struct sockaddr_in localAddr;
        bzero(&localAddr, sizeof(localAddr));// #include <string.h>，將為sizeof(x)的前幾個東西清成0
        
        localAddr.sin_family = PF_INET;
        localAddr.sin_port = htons(port);
        localAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

        /*Connect to the server連線.*/
        if (connect(localSocket, (struct sockaddr *)&localAddr, sizeof(localAddr)) < 0){
            perror("Connection failed.");
            exit(3);
        }

        bzero(instruction, sizeof(char)* BUFF_SIZE);
        bzero(filenm, sizeof(char)* BUFF_SIZE);
        //printf("Giving instruction:\n");
        scanf("%s", instruction);
        //printf("instruction: %s\n", instruction);

        if(instruction[0] == 'l' && instruction[1] == 's' && strlen(instruction) == 2){ //ls-OK!
            //printf("Into list\n");
            if(write(localSocket, instruction, sizeof(char)* BUFF_SIZE) < 0){
                perror("Write to server error, inst: ls!");
                exit(4);
            }
            rec_ls(localSocket, instruction);
        }
        else if (instruction[0] == 'p' && instruction[1] == 'u' && instruction[2] == 't' && strlen(instruction) == 3){ //put, upload files
            //printf("Into put\n");
            scanf("%s", filenm);
            int write_val = write(localSocket, instruction, BUFF_SIZE);
            ////printf("write inst: %d bytes\n", write_val);
            if( write_val < 0){
                perror("Write to server error, inst: put.");
                exit(1);
            }
            write_val = write(localSocket, filenm, BUFF_SIZE);
            ////printf("write filenm: %d bytes\n", write_val);

            if(write_val < 0){
                perror("Write to server error, filenm.");
                exit(1);
            }
            //printf("write filenm: %s, write_val = %d\n", filenm, write_val );
            //先確認檔名存在
            char exist[2];
            //確認檔案存在
            bzero(exist, sizeof(char)* 2);
            //printf("Finding file...\n");
            find_file(filenm, exist);
            int sent;
            sent = write(localSocket, exist, 2);
            //printf("sent = %d, exist[0] = %c\n", sent, exist[0]);
            //存在後再傳檔案
            if(exist[0] == '1'){
                //printf("Sending file start...\n");
                send_file(filenm, localSocket);//這裡很怪，localSocket跟remote socket搞不懂@@
                //printf("Sending file successful.\n");
            }
        }
        else if (instruction[0] == 'g' && instruction[1] == 'e' && instruction[2] == 't' && strlen(instruction) == 3){ //get, download file
            //printf("Into get\n");
            scanf("%s", filenm);
            if(write(localSocket, instruction, BUFF_SIZE) < 0){
                perror("Write to server error, inst: get.");
                exit(1);
            }            
            if(write(localSocket, filenm, BUFF_SIZE) < 0){
                perror("Write to server error, filenm.");
                exit(1);
            }
            char exist[2];
            bzero(exist, sizeof(char)* 2);
            int file_exist = read(localSocket, exist, 2);
            rec_file(exist, localSocket, instruction, filenm);
        }
        else if (instruction[0] == 'p' && instruction[1] == 'l' && instruction[2] == 'a' && instruction[3] == 'y' && strlen(instruction) == 4){ //play
            //printf("Into playing video\n");
            scanf("%s", filenm);
            int jud_mpg = judge_mpg(filenm);
            if(jud_mpg == 0){//如果不是mpg檔案
                //隨便寫著指令過去，讓他cmd not found
                instruction[0] = 'h';
                instruction[1] = 'a';
                if(write(localSocket, instruction,  BUFF_SIZE) < 0){
                    perror("Write to server error, inst: play!");
                    exit(1);
                }
                //printf("%s does not a .mpg file.\n",filenm );
            }
            else{//是mpg檔
                ////printf("222\n");
                //是mpg檔
                if(write(localSocket, instruction,  BUFF_SIZE) < 0){
                    perror("Write to server error, inst: play!");
                    exit(1);
                }
                if(write(localSocket, filenm, BUFF_SIZE) < 0){
                    perror("Write to server error, filenm");
                    exit(1);
                }
                ////printf("333\n");
                char exist[2];
                bzero(exist, sizeof(char)* 2);
                int file_exist = read(localSocket, exist, 2);
                if(exist[0] == '1'){
                    //先開一個 thread 去做buffer
                    pthread_t child_buf;
                    //child 負責播影片
                    pthread_create(&child_buf, NULL, doInChildThread, NULL);
                    //主程式用來接frame
                    playing_video(localSocket);
                    pthread_join(child_buf, NULL);
                }
                else{
                    //printf("%s does not exist.\n", filenm);
                }                
            }

        }
        else {
            instruction[0] = 'h';
            instruction[1] = 'a';
            if(write(localSocket, instruction,  BUFF_SIZE) < 0){
                perror("Write to server error, inst: cmd not found.");
                exit(1);
            }
            cmd_not_fd();  
        }
        close(localSocket);
    }
    return 0;
}

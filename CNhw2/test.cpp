#include <pthread.h>

// global variable
int  play_video_ing[30][4];//，因為最多30個clients
char play_video_flnm[30][BUFF_SIZE];
 
void *doInChildThread(void *arg){
    int i = *(int*)arg;
    char filenm [BUFF_SIZE];
    for (int j = 0; j < BUFF_SIZE; j++)
        filenm[j] = play_video_flnm[i][j];
    int remoteSocket = play_video_ing[i][2]; // 2:remote

    play_video_ing[i][0] = 1; //0開始

    play_video_ing[i][1] = 1;//1 結束，代表這個影片播完了可以關閉socket了
}

int main(){
    1. close(sd)、要移動到每一個地方
    2. listen 下面就要多出： 
    for(int i = 0; i < max_clients; i++){
        play_video_ing[i][0] = 0;//開始
        play_video_ing[i][1] = 0;//結束
        play_video_ing[i][2] = 0;//remoteSo = sd
        play_video_ing[i][3] = 0;// i 
    }
    pthread_t pid[30];


    2. 319行：
        if(FD_ISSET(sd, &readfds)){
        改成：
        if(FD_ISSET(sd, &readfds) &&play_video_ing[i][0] == 0){
    3. 放影片的main
                    if(exist[0] == '1'){
                        printf("Playing video starts...\n");
                        playing_video(filenm, sd);
                        printf("Playing video successful.\n");
                    }
                    變成：填入2, 3, filenm, input, create
                     if(exist[0] == '1'){
                        printf("Playing video starts...\n");
                       
                        play_video_ing[i][2] = sd; // remoteSocket的值
                        play_video_ing[i][3] = i; // client_scoket[i];
                        for (int j = 0; j < BUFF_SIZE; j++)
                            play_video_flnm[i][j] = filenm[j];
                        int* input = new int(i);
                        pthread_create(&pid[i], NULL, doInChildThread, (void*)input);
                       
                        printf("Playing video successful.\n");
                    }
    4. 最後面要加上，cmd not found
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

}



# Computer Network
* Professor: AC Pang  
* Title: HomeWork for coding     
* Classroom: 102

# HW2
* Implementation of a simple Network Storage system with these functions: 
  1. Client can watch a “.mpg” video (streaming) on the server
  2. Client can upload files to server
  3. Client can download files from server
  4. Client can list what files are in the folder of server
* For video steaming, you don’t need to send audio. You can just send raw frames (or encoded frames).
  * NOTE:  I had only sent the raw frame!
* Video streaming requires buffering at both client side and server side. (bonus)
  * NOTE: Buffering had been done
* After upload and download, you need to ensure the files are identical between source and destination.
* In this assignment, all the transmission should be implemented by the socket of TCP.
* Server is required to support multiple connections. That is, there can be more than 1 client connecting to the server simultaneously
* After building up of a connection, a user can enter the commands below on the client,
  1. $ ls // Then, the client’s will print out what files is in the server’s folder.
  2. $ put <filename> // Then, the client will upload the file with the <filename> to the server’s folder
  3. $ get <filename> // Then, the client will download the file with the <filename> to the client’s folder
  4. $ play <videofile> // Then, the client will play the <videofile> from the buffer at server side to the client
* If the command doesn’t exist or the command format is wrong, please print out “Command not found.” or “Command format error.” on the client.
* If the file doesn’t exist while putting or getting a file, please print out “The ‘<filename>’ doesn’t exist.” on the client.
* If the video file is not a “.mpg” file while playing a video file, please print out “The ‘<videofile>’ is not a mpg file.”
* Client should be able to send another command after a command is finished.
* The multiple connections should be implemented with pthread.h, while the video player should be implemented with OpenCV.
* The implementation must be in C or C++
* Makefile path: ./109-1-CN_NTU-CSIE-/CNhw2/hw2_b05611047/Makefile
* !<img width="649" alt="截圖 2021-05-13 09 41 02" src="https://user-images.githubusercontent.com/41279685/118065326-56280e80-b3cf-11eb-9086-f7aa83f41bf7.png">

# HW3
* Very similar to homework-2.  
* Differents:
  1. Change the socket from TCP to **UDP socket**
  2. There is a agent, who would randomly drop out the packets. So, we needed to deal with:   
    A. **Go-Back-N**, for both (1) normal case, and (2) packet loss case.  
    B. Go-Back-N with **congestion control**.  
    C-1. Packet loss: Time out & Retransmission (Go-Back-N).  
    C-2. Packet loss: Sequence number.  
    C-3. Packet loss: Completeness and correctness of transmitted file.  
    D. Buffer handling.  
      * Buffer Overflow: Drop the packets during out of buffer.  
      * Flush (write) to the file: Only when buffer overflows or all packets in range are received.   
    E. Slow start.  
    F. Show message:    
    - Sender: send, recv, data, ack, fin, finack, sequence number, time out, resnd, winSize, threshold.   
    - Receiver: send, recv, data, ack, fin, finack, sequence number, drop, flush.   
    - Agent: get, fwd, data, ack, fin, finack, sequence number, drop, loss rate.   

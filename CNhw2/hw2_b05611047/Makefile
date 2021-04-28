CC = g++
OPENCV =  `pkg-config --cflags --libs opencv`
PTHREAD = -pthread
STD = -std=c++11

CLIENT = client.cpp
SERVER = server.cpp
CLI = client
SER = server

all: server client
  
server: $(SERVER)
	$(CC) $(STD) $(SERVER) -o $(SER) $(OPENCV) $(PTHREAD)
client: $(CLIENT)
	$(CC) $(CLIENT) -o $(CLI)  $(OPENCV) $(PTHREAD)

.PHONY: clean

clean:
	rm $(CLI) $(SER)

#g++ client.cpp -o client `pkg-config --cflags --libs opencv` -pthread
#g++ -std=c++11 server.cpp -o server `pkg-config --cflags --libs opencv` -pthread



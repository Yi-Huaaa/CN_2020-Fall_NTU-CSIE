CC = g++
OPENCV =  `pkg-config --cflags --libs opencv`
PTHREAD = -pthread
STD = -std=c++11

CLIENT = client.cpp
SERVER = server.cpp
AGENT = agent.c
#SHW_FILE = shw_player.cpp
CLI = client
SER = server
AGE = agent
#SHW = shw_player

all: server client agent

server: $(SERVER)
		$(CC) $(STD) $(SERVER) -o $(SER) $(OPENCV) $(PTHREAD)
client: $(CLIENT)
		$(CC) $(CLIENT) -o $(CLI)  $(OPENCV) $(PTHREAD)
agent: $(AGENT)
		$(CC) $(AGENT) -o $(AGE) $(OPENCV) $(PTHREAD)
#shw: $(SHW_FILE)
#        $(CC) $(SHW_FILE) -o $(SHW) $(OPENCV) $(PTHREAD)

.PHONY: clean

clean:
	rm $(CLI) $(SER) $(AGE) $(SHW)

#g++ client.cpp -o client `pkg-config --cflags --libs opencv` -pthread
#g++ -std=c++11 server.cpp -o server `pkg-config --cflags --libs opencv` -pthread
CC=g++
LFLAGS=-lpthread -std=c++11

all:
	$(CC) $(LFLAGS) -o client client.cpp
	$(CC) $(LFLAGS) -o server server.cpp

server:
	$(CC) -c -o server server.cpp

client:
	$(CC) -c -o client client.cpp

.PHONY:	clean

clean:
	rm -f client server
all: server.cpp
	g++ -std=c++11 -Wall -o server server.cpp -lldap -llber -lpthread
clean:
	rm server

run: server
	./server -p 10001 -d ./emailFolder
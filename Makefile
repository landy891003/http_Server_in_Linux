all: server

server:httpserver.cpp
	g++ httpserver.cpp -o server
clean:
	rm -f server 

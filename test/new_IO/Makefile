all: client server

client: client.cpp
	make -C ../../build IO_client.o
	g++ -g3 -I ../../include client.cpp ../../build/IO_client.o -o client -lpthread

server: server.cpp ../../build/IO_server.o
	make -C ../../build IO_server.o
	g++ -g3 -I ../../include server.cpp  ../../build/IO_server.o -o server -lpthread


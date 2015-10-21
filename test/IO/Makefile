all: ./client ./server

./client: ./client.cpp ../../build/IO.o
	g++ -g3 -std=c++0x client.cpp ../../build/IO.o -I ./include -lboost_system -lpthread -o client

./server: ./server.cpp ../../build/IO.o
	g++ -g3 -std=c++0x server.cpp ../../build/IO.o -I ./include -lboost_system -lpthread -o server

../../build/IO.o: ../../src/common/include/IO.hpp ../../src/common/IO.cpp
	make -C ../../src/common/

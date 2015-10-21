all: ./client ./client2 ./server ./server2

./client: ./client.cpp 
	g++ -g3 -std=c++0x client.cpp -lboost_system -lpthread -o client

./client2: ./client2.cpp ../../build/IO.o
	g++ -g3 -std=c++0x client2.cpp ../../build/IO.o -I ./include -lboost_system -lpthread -o client2

./server:
	g++ -g3 -std=c++0x server.cpp -lboost_system -lpthread -o server

./server2: ./server2.cpp ../../build/IO.o
	g++ -g3 -std=c++0x server2.cpp ../../build/IO.o -I ./include -lboost_system -lpthread -o server2
	

../../build/IO.o: ../../src/common/include/IO.hpp ../../src/common/IO.cpp
	g++ -g3 -std=c++0x -c ../../src/common/IO.cpp -o ../../build/IO.o

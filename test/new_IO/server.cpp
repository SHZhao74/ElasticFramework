#include <iostream>
#include <cstdlib>
#include <cstdio>
#include "IO.hpp"

namespace NEWSCL{
	void *(*threadPerSocket)(void*);
}
void *run(void* psock){
	int sock = *(static_cast<int*>(psock));
	char buf[128];
	std::cout << "connection comes" << std::endl;
	
	if(recv(sock, buf, 128, 0) < 0 ){
		perror("recv error");
		exit(1);
	}
	if(send(sock, "HI", 2, 0) < 0 ){
		perror("send error");
		exit(1);
	}
	
}
int main(){
	NEWSCL::SocketIOServer server(8000);
	NEWSCL::threadPerSocket = run;
	server.bindAndListen(20);
	server.mainLoop();
}

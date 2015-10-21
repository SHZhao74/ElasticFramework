#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include "IO.hpp"

namespace NEWSCL{
	void *(*threadPerSocket)(void*);
}
void *run(void* psock){
	int sock = *(static_cast<int*>(psock));
	char buf[128], send_buf[128];
	std::cout << "connection comes" << std::endl;
	
	if(recv(sock, buf, 128, 0) < 0 ){
		perror("recv error");
		exit(1);
	}
	sprintf(send_buf, "REG succeed");
	if(send(sock, send_buf, strlen(send_buf) + 1, 0) == -1){
		perror("send error2");
	}   

	//if(send(sock, "HI", 2, 0) < 0 ){
	//	perror("send error");
	//	exit(1);
	//}
	
}
int main(){
	NEWSCL::SocketIOServer server(6789);
	NEWSCL::threadPerSocket = run;
	server.bindAndListen(20);
	server.mainLoop();
}

#include "IO.hpp"
#include <cstring>
#include <iostream>
int main(){
	NEWSCL::SocketIOClient client("127.0.0.1", 8000);
	client.connect();
	char buf[128];
	client.send("reg OK", strlen("reg OK"));
	client.recv(buf, 128);
	std::cout << buf << std::endl; 


}

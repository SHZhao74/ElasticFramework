#include "IO.hpp"
#include <iostream>
int main(){
	char buf[1024] = "1234567892222";
	NEWSCL::SocketIOClient client("localhost", 6789);
	printf("wait for sending\n");
	//client.send(buf, 10);
	client.send(buf, 1024);
	printf("wait for recving\n");
	std::cout << "size:" << client.recv(buf, 1024) << std::endl;
	std::cout << buf <<std::endl;
	while(true);

}

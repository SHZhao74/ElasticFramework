#include "IO.hpp"
#include <iostream>
int main(){
	char buf[50] = "123456789";
	NEWSCL::SocketIOClient client("localhost", "6789");
	//client.send(buf, 10);
	std::cout << "size:" << client.recv(buf, 50) << std::endl;
	std::cout << buf <<std::endl;
	while(true);

}

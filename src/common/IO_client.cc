#include <cstring>

#include "IO.h"

int newscl::SocketIOClient::Connect(){
	struct sockaddr_in pin;

	bzero(&pin, sizeof(pin));
	pin.sin_family = AF_INET;
	pin.sin_addr.s_addr = inet_addr(ip_.c_str());
	pin.sin_port = htons(port_);

	sock_ = socket(AF_INET, SOCK_STREAM, 0);
	if(sock_ == -1){
		perror("[ERROR] call to socket");
		exit(-1);
	}
	if (::connect(sock_, (sockaddr *)&pin, sizeof(pin)) == -1) {
		perror("[ERROR] call to connect");
		exit(1);
	}
	return sock_;
}


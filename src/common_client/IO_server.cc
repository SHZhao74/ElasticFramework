#include <vector>
#include <cstring>

#include <pthread.h>

#include "IO.h"

namespace newscl {
	void *(*thread_per_socket)(void*);
};
// newscl::SocketIOServer
int newscl::SocketIOServer::BindAndListen(const unsigned int max_connections) {
	struct sockaddr_in sin;

	sock_ = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_ == -1) {
		perror("call to socket");  //用來將上一個函數發生錯誤的原因輸出到標準錯誤(stderr)
		exit(1);
	}
	int on = 1;
	if (setsockopt(sock_,
                 SOL_SOCKET,
                 SO_REUSEADDR,
                 (const char *)&on,
                 sizeof(on)) == -1) {
		perror("SO_REUSEADDRsetsockopt() error");
	}

	bzero(&sin, sizeof(sin));  //將一段內存內容全清為零
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(INADDR_ANY);  //將32位主機字符順序轉換成網絡字符順序
	sin.sin_port = htons(port_);  //用來將參數指定的16位元hostshort轉換成網絡字符順序

	if (bind(sock_, (struct sockaddr *)&sin, sizeof(sin)) == -1) {
		perror("call to bind");
		exit(1);
	}

	if (listen(sock_, max_connections) == -1) {
		perror("call to listen");
		exit(1);
	}
	max_connections_ = max_connections;
	return 0;
}

void newscl::SocketIOServer::MainLoop() {
	std::vector<ThreadData> tmpSock(max_connections_);
	std::vector<pthread_t> pids(max_connections_);
	struct sockaddr_in pin;
	decltype(max_connections_) counter = 0;
	unsigned int addrsize = sizeof(pin);

	while (true) {
		tmpSock[counter].client_sock = accept(sock_,
                                          (struct sockaddr *)&pin,
                                          &addrsize);
		if (tmpSock[counter].client_sock == -1) {
			perror("call to accept");
			exit(1);
		}

		pthread_create(&pids[counter],
                   NULL,
                   thread_per_socket,
                   reinterpret_cast<void*>(&tmpSock[counter]));
		counter = counter == max_connections_ - 1 ? 0 : counter + 1;
	}
}

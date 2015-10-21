#ifndef NEWSCL_IO_HEADER
#define NEWSCL_IO_HEADER
#include <string>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

namespace newscl {
	class IOInterface {
		public:
			virtual int Recv(char * const buf, const unsigned int bufLen) = 0;
			virtual int Send(char const * const buf, const unsigned int bufLen, bool is_buffer = false) = 0;
	};
	extern void *(*thread_per_socket)(void*);

	int SockRecv(int sock, char * const buf, const unsigned int bufLen);
	int SockSend(int sock, char const * const buf, const unsigned int bufLen, bool is_buffer = false);

	class SocketIOClient:public IOInterface {
		protected:
			std::string ip_;
			int port_;
			int sock_;

		public:
			SocketIOClient(std::string ip, const int port): ip_(ip), port_(port) {};
			~SocketIOClient(){};

      // manipulator
			inline int Recv(char * const buf, const unsigned int buf_len) {
				return SockRecv(sock_, buf, buf_len);
			}
			inline int Send(char const * const buf, const unsigned int buf_len, bool is_buffer = false) {
				return SockSend(sock_, buf, buf_len, is_buffer);
			}
			int Connect();

      // accessor
			inline decltype(ip_) ip() const { return ip_; }
			inline decltype(port_) port() const { return port_; }
			inline decltype(sock_) sock() const { return sock_; }
	};

	class SocketIOServer{
		protected:
			unsigned int max_connections_;
			int sock_;
			int port_;

		public:
			explicit SocketIOServer(const int port): port_(port){};
			~SocketIOServer(){};

			struct ThreadData{
				int client_sock;
				void* data;
			};

      // manipulator
			int BindAndListen(unsigned int max_connections);
			void MainLoop();

      // accessor
			inline decltype(sock_) sock() const { return sock_; }
			inline decltype(port_) port()const { return port_; }
			inline decltype(max_connections_) max_connections() const{
        return max_connections_;
      }
	};
};
#endif

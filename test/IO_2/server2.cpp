#include "IO.hpp"
#include "config.hpp"


class my: public NEWSCL::tcp_connection{
	public:
		static pointer create(boost::asio::io_service &io_service){
			return pointer(new my(io_service));

		}
		void start(){
            char tmp[10] = "993456789";
			send(tmp, 10);
		}
		int recv(char *tmp, const unsigned int bufLen){}
		int send(const char *tmp, const unsigned int bufLen){
			boost::asio::async_write(socket_, boost::asio::buffer(tmp, 10),
                    boost::bind(&tcp_connection::handle_write, shared_from_this(),
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred));

		}
	protected:
		my(boost::asio::io_service &io_service): tcp_connection(io_service){};
};

int main(){
	NEWSCL::SocketIOAsyncServer<my> server(NEWSCL::port);
	server.getIO_service().run();
}

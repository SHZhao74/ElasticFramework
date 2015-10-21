#include "IO.hpp"
#include "config.hpp"
#include <boost/array.hpp>


class my: public NEWSCL::tcp_connection{
	private:
		char readBuf[1024];
	public:
		static pointer create(boost::asio::io_service &io_service){
			return pointer(new my(io_service));

		}
		void start(){
			recv(readBuf, 1024);
		}
		int recv(char *tmp, const unsigned int bufLen){
			boost::asio::async_read(socket_, boost::asio::buffer(readBuf, 1024),
                    boost::bind(&tcp_connection::handle_read, shared_from_this(),
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred));
		}

		int send(const char *tmp, const unsigned int bufLen){
			boost::asio::async_write(socket_, boost::asio::buffer(tmp, 10),
                    boost::bind(&tcp_connection::handle_write, shared_from_this(),
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred));

		}

		void handle_read(const boost::system::error_code& /*error*/,
			size_t /*bytes_transferred*/){
			std::cout << "asdfadfasdfasfd" << std::endl;
			std::cout << readBuf << std::endl;
			printf("handle_read");
            char tmp[10] = "993456789";
			send(tmp, 10);
		}
	protected:
		my(boost::asio::io_service &io_service): tcp_connection(io_service){};
};

int main(){
	NEWSCL::SocketIOAsyncServer<my> server(NEWSCL::port);
	server.getIO_service().run();
}

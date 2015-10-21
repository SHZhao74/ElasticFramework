#include <string>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#ifndef NEWSCL_IO_HEADER
#define NEWSCL_IO_HEADER

namespace NEWSCL{
	class IOInterface{
		public:
			virtual int recv(char *buf, const unsigned int bufLen) = 0;
			virtual int send(const char *buf, const unsigned int bufLen) = 0;
	};

	class SocketIO:public IOInterface{
		protected:
			boost::asio::io_service io_service_;
		public:
			virtual int recv(char *buf, const unsigned int bufLen);
			virtual int send(const char *buf, const unsigned int bufLen);
			boost::asio::io_service& getIO_service(){ return io_service_;}
			SocketIO();
			~SocketIO();
	};
	class SocketIOClient:public SocketIO{
		private:
		protected:
			std::string hostName_;
			std::string port_;
			boost::asio::ip::tcp::socket socket_;
		public:
			SocketIOClient(): SocketIO(), socket_(getIO_service()){};
			SocketIOClient(std::string hostName, std::string port);
			int recv(char *buf, const unsigned int bufLen);
			int send(const char *buf, const unsigned int bufLen);
			int connect(const std::string hostName, const std::string port);
	};

	class tcp_connection: public boost::enable_shared_from_this<tcp_connection>{
		public:
			typedef boost::shared_ptr<tcp_connection> pointer;
			static pointer create(boost::asio::io_service& io_service){
				return pointer(new tcp_connection(io_service));
			}
			boost::asio::ip::tcp::socket &socket(){ return socket_; }
			virtual void start(){};
		private:
		protected:
			boost::asio::ip::tcp::socket socket_;

			tcp_connection(boost::asio::io_service& io_service): socket_(io_service){}
			void handle_write(const boost::system::error_code& error,
					      size_t bytes_transferred){
				std::cout << "error:" << error.value() << std::endl;
				std::cout << "bytes:" << bytes_transferred << std::endl;

			}

	};

	template<typename T>
	class SocketIOServer:public SocketIO{
		private:

		protected:
			boost::asio::ip::tcp::acceptor acceptor_;

		public:
			SocketIOServer(int port): SocketIO(), acceptor_(io_service_, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)){
				startAccept();
			}
			int bindAndListen(int tport);
			int startAccept(){
				tcp_connection::pointer new_connection =
					T::create(acceptor_.get_io_service());

				acceptor_.async_accept(new_connection->socket(),
						boost::bind(&NEWSCL::SocketIOServer<T>::handle_accept, this, new_connection,
							boost::asio::placeholders::error));
			}
			void handle_accept(NEWSCL::tcp_connection::pointer new_connection, const boost::system::error_code &error){
				std::cerr << "A connection comming" << std::endl;
				if(!error){
					new_connection->start();
				}
				startAccept();
			}
	};
};
#endif

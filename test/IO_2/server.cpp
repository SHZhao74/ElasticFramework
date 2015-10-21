//
// server.cpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2012 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <ctime>
#include <iostream>
#include <string>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

std::string make_daytime_string()
{
	using namespace std; // For time_t, time and ctime;
	time_t now = time(0);
	return ctime(&now);
}

class tcp_connection: public boost::enable_shared_from_this<tcp_connection>
{
	public:
		typedef boost::shared_ptr<tcp_connection> pointer;

		// You must implement the static function in the derived base
		// because it will return a shared_ptr to the object not the raw pointer!
		static pointer create(boost::asio::io_service& io_service){}

		tcp::socket& socket()
		{
			return socket_;
		}

		virtual void start() = 0;

		virtual void handle_write(const boost::system::error_code& /*error*/,
				size_t /*bytes_transferred*/){};
		virtual void handle_read(const boost::system::error_code& /*error*/,
				size_t /*bytes_transferred*/){};
	protected:
		tcp_connection(boost::asio::io_service& io_service)
			: socket_(io_service)
		{
		}


		tcp::socket socket_;
}; 

class my: public tcp_connection{
	public:
		static pointer create(boost::asio::io_service &io_service){
			return pointer(new my(io_service));
		}
		void start(){
			char tmp[10] = "123456789";

			boost::asio::async_write(socket_, boost::asio::buffer(tmp, 10),
					boost::bind(&tcp_connection::handle_write, shared_from_this(),
						boost::asio::placeholders::error,
						boost::asio::placeholders::bytes_transferred));	
		}
	protected:
		my(boost::asio::io_service &io_service): tcp_connection(io_service){}
		void handle_write(const boost::system::error_code& /*error*/,
				size_t /*bytes_transferred*/)
		{
			std::cout << "hi!" << std::endl;
		}
};

template <typename T>
class tcp_server
{
	public:
		tcp_server(boost::asio::io_service& io_service)
			: acceptor_(io_service, tcp::endpoint(tcp::v4(), 6789))
		{
			start_accept();
		}

	private:
		void start_accept()
		{
			tcp_connection::pointer new_connection =
				T::create(acceptor_.get_io_service());

			acceptor_.async_accept(new_connection->socket(),
					boost::bind(&tcp_server::handle_accept, this, new_connection,
						boost::asio::placeholders::error));
		}

		void handle_accept(tcp_connection::pointer new_connection,
				const boost::system::error_code& error)
		{
			if (!error)
			{
				new_connection->start();
			}

			start_accept();
		}

		tcp::acceptor acceptor_;
};

int main()
{
	try
	{
		boost::asio::io_service io_service;
		tcp_server<my> server(io_service);
		io_service.run();
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}

	return 0;
}

#ifndef SERVER_CALCULATOR_CPP_SERVER_H
#define SERVER_CALCULATOR_CPP_SERVER_H

#include "Session.h"

using boost::asio::ip::tcp;

class Server {
public:
    Server(boost::asio::io_context& io_context,int port)
        :_acceptor(io_context, tcp::endpoint(tcp::v4(),port)){
        accept_new_client();
    }
private:
    void accept_new_client();

    tcp::acceptor _acceptor;
};


#endif //SERVER_CALCULATOR_CPP_SERVER_H

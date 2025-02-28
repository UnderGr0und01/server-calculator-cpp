#include "Server.h"

void Server::accept_new_client() {
    _acceptor.async_accept(
            [this](boost::system::error_code ec, tcp::socket socket){
                if(!ec){
                    std::make_shared<Session>(std::move(socket))->start();
                }
                Server::accept_new_client();
            });
}

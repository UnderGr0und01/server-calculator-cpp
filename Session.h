#ifndef SERVER_CALCULATOR_CPP_SESSION_H
#define SERVER_CALCULATOR_CPP_SESSION_H

#include <boost/asio.hpp>
#include <iostream>
#include <memory>
#include <string>

using boost::asio::ip::tcp;

class Session : public std::enable_shared_from_this<Session>{
public:
    Session(tcp::socket socket): _socket(std::move(socket)){};

    void start();
private:
    void read_form_client();
    void send_data_to_client(const std::string& response);

    std::string handle_command(const std::string& command);
    tcp::socket _socket;
    enum { max_length = 1024 };
    char _data[max_length];
};


#endif //SERVER_CALCULATOR_CPP_SESSION_H

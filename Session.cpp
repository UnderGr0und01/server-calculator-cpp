#include "Session.h"

void Session::start() {
    std::cout<<"New client connetcted!"<<std::endl;
    Session::read_form_client();
}

void Session::read_form_client() {
    auto self(shared_from_this());
    _socket.async_read_some(boost::asio::buffer(_data),[this,self](boost::system::error_code ec, std::size_t length){
        if(!ec){
            std::string command(_data,length);
            std::string response = handle_command(command);
            if (command == "exit\n")
            {
                std::cout<<"Closing session for client."<<std::endl;
                return;
            }
            Session::send_data_to_client(response);
        } else{
            std::cerr<<"Client disconnected."<<std::endl;
        }
    });
}

void Session::send_data_to_client(const std::string &response) {
    auto self(shared_from_this());
    boost::asio::async_write(_socket,boost::asio::buffer(response),
        [this,self](boost::system::error_code ec,std::size_t /*length*/){
        if(!ec){
            Session::read_form_client();
        }
    });
}

std::string Session::handle_command(const std::string &command) {
    std::cout<<"Recieved command: "<<command<<std::endl;

    if(command == "hello\n"){
        return "Hello from server!\n";
    } else if (command == "exit\n"){
        return "Goodbye!\n";
    }else{
        return "Unknown command. Try again or 'exit'.\n";
    }
}
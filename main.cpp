#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sstream>
#include "chrono"
#include "thread"
#include <pqxx/pqxx>
#include "exprtk/exprtk.hpp"

class Client{
public:
    int _clientSocket;
    bool _isAuthenticated;

private:
    std::string _username;
    std::string _password;
    int _balance;
public:
    Client():_username(" "),_password(" "),_isAuthenticated(false),_balance(0),_clientSocket(0){};
    ~Client()= default;

    void initClientSocket(const int &serverSocket);

    void setUsername(const std::string &username){
        _username = username;
    }

    std::string getUsername(){
        return _username;
    }

    void setPassword(const std::string &password){
        _password = password;
    }

    std::string getPassword(){
        return _password;
    }

    void setBalance(const int &balance);

    int getBalance() const{
        return _balance;
    };
};

int startServer(const int &serverSocket)
{
    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(8080);

    if (bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1)
    {
        std::cerr << "Error while binding socket" << std::endl;
        close(serverSocket);
        return 1;
    }

    if (listen(serverSocket, 5) == -1)
    {
        std::cerr << "Error while waiting connection" << std::endl;
        close(serverSocket);
        return 1;
    }

    std::cout << "Server start and waiting connection..." << std::endl;
    return 0;
}

int initServerSocket()
{
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1)
    {
        std::cerr << "Error while create socket";
        return 1;
    }
    return serverSocket;
}

void Client::initClientSocket(const int &serverSocket)
{
    sockaddr_in clientAddress{};
    socklen_t clientAddressLength = sizeof(clientAddress);
    _clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddress, &clientAddressLength);
    if (_clientSocket == -1)
    {
        std::cerr << "Error while accept connection" << std::endl;
        close(serverSocket);
    }

    std::cout << "Connection success" << std::endl;

    std::cout<<"Client socket= "<<_clientSocket<<std::endl;
}

bool authenticate(Client client)
{
    try
    {
        pqxx::connection connection("dbname=postgres user=postgres password=7456 hostaddr=172.17.0.2 port=5432");
        //        pqxx::connection connection("dbname=postgres user=postgres password=7456 hostaddr=127.0.0.1 port=5432");

        if (connection.is_open())
        {
            pqxx::work transaction(connection);
            std::string query = "SELECT COUNT(*) FROM users WHERE username = '" + client.getUsername() + "' AND password = '" + client.getPassword() + "';";

            std::cout << query << std::endl;

            pqxx::result res = transaction.exec(query);
            int count = res[0][0].as<int>();
            transaction.commit();

            return count > 0;
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error auth: " << e.what() << std::endl;
    }
    return false;
}

int getBalanceFromDB(Client client)
{
    pqxx::connection connection("dbname=postgres user=postgres password=7456 hostaddr=172.17.0.2 port=5432");
    pqxx::work transaction(connection);
    std::string query = "SELECT balance FROM users WHERE username = '" + client.getUsername() + "' AND password= '" + client.getPassword() + "';";

    std::cout << query << std::endl;

    pqxx::result res = transaction.exec(query);
    int balance = res[0][0].as<int>();
    transaction.commit();
//    client.setBalance(res[0][0].as<int>());
    return balance;
}

//void updateBalance(Client &client)
//{
//    pqxx::connection connection("dbname=postgres user=postgres password=7456 hostaddr=172.17.0.2 port=5432");
//    pqxx::work transaction(connection);
//    std::string query = "UPDATE users SET balance = " + std::to_string(client.getBalance()) + " WHERE username = '" + client.getUsername() + "' AND password= '" + client.getPassword() + "';";
//    pqxx::result res = transaction.exec(query);
//    transaction.commit();
//}

void Client::setBalance(const int &balance)
{
    pqxx::connection connection("dbname=postgres user=postgres password=7456 hostaddr=172.17.0.2 port=5432");
    pqxx::work transaction(connection);
    std::string query = "UPDATE users SET balance = " + std::to_string(balance) + " WHERE username = '" + _username + "' AND password= '" + _password + "';";
    pqxx::result res = transaction.exec(query);
    transaction.commit();
    _balance = balance;
}

void login(Client &client)
{
    char buffer[1024];
    ssize_t bytesRead;

    while ((bytesRead = read(client._clientSocket, buffer, sizeof(buffer))) > 0)
    {
        std::istringstream iss(buffer);
        std::string command;
        std::string username,password;
        iss >> command;
        bytesRead = read(client._clientSocket, buffer, sizeof(buffer));
        std::cout << "iss = " << command << std::endl;
        if (command == "login")
        {
            iss >> username;
            client.setUsername(username);

            std::istringstream iss2(buffer);
            std::string nextCommand;
            iss2 >> nextCommand;
            std::cout << "iss2 = " << nextCommand << std::endl;

            if (nextCommand == "logout")
            {
//               delete(client);
                close(client._clientSocket);
            }
            else if (nextCommand == "password")
            {
                iss2 >> password;
                client.setPassword(password);
                client._isAuthenticated = authenticate(client);
                std::string response = client._isAuthenticated ? "Auth success!\n" : "Auth error: user does not exist.\n";
                ssize_t bytesWritten = write(client._clientSocket, response.c_str(), response.length());
                if (bytesWritten == -1)
                {
                    std::cerr << "Error while sending data to client" << std::endl;
                    close(client._clientSocket);
                }
                break;
            }
        }
        else
        {
            std::string response = "You need to login! Type login <username>\n";

            ssize_t bytesWritten = write(client._clientSocket, response.c_str(), response.length());

            if (bytesWritten == -1)
            {
                std::cerr << "Error while sending data to client" << std::endl;
                client._isAuthenticated = false;
                close(client._clientSocket);
            }
        }
    }
    if (bytesRead == -1)
    {
        std::cerr << "Error while reading data from client(login)" << std::endl;
    }
}

std::string getCurrentDateTime()
{
    auto now = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::string dateTime = std::ctime(&time);
    dateTime.pop_back();
    return dateTime;
}

void logEntry(const std::string &user, const std::string &request, const double &result)
{
    std::string dateTime = getCurrentDateTime();
    std::ofstream logFile("server.log", std::ios::app);
    if (logFile.is_open())
    {
        logFile << "Date/Time: " << dateTime << std::endl;
        logFile << "User: " << user << std::endl;
        logFile << "Request: " << request << std::endl;
        logFile << "Result: " << result << std::endl;
        logFile << "___________________________________________________________________________________"<<std::endl;
        logFile.close();
    }
    else
    {
        std::cerr << "Error opening log file" << std::endl;
    }
}

double calculate(const std::string &expression)
{
    typedef exprtk::expression<double> Expression;
    typedef exprtk::parser<double> Parser;

    Expression expr;
    Parser parser;

//    if (!parser.compile(expression, expr))
//    {
//        throw std::runtime_error("Failed to compile expression");
//    }
    if (!parser.compile(expression, expr))
    {
        std::cout<<("Failed to compile expression")<<std::endl;
    }

    return expr.value();
}

void commandHandler(Client &client)
{
    char buffer[1024];
    ssize_t bytesRead;

    while ((bytesRead = read(client._clientSocket, buffer, sizeof(buffer))) > 0)
    {
        std::istringstream isscommand(buffer);
        std::string command;

        std::cout << "Receive from client: " << std::string(buffer, bytesRead) << std::endl;

        isscommand >> command;
        std::cout << "command = " << command << std::endl;
        std::string expression;

        if (command == "calc")
        {
            int balance = getBalanceFromDB(client);
//            int balance = client.getBalance();
            if (balance <= 0)
            {
                std::string response = "You don't have balance to this action\n";

                ssize_t bytesWritten = write(client._clientSocket, response.c_str(), response.length());
                if (bytesWritten == -1)
                    std::cerr << "Error while sending data to client" << std::endl;
            }
            else
            {
                balance --;
                client.setBalance(balance);
//                updateBalance(client);

                isscommand >> expression;
                std::cout << "expression = " << expression << std::endl;
                double result = calculate(expression);

                std::string response = std::to_string(result) + "\n" + "Now your balance = " + std::to_string(client.getBalance()) + "\n";

                ssize_t bytesWritten = write(client._clientSocket, response.c_str(), response.length());

                if (bytesWritten == -1)
                    std::cerr << "Error while sending data to client" << std::endl;

                logEntry(client.getUsername(), expression, result);
            }
        }
        else if (command.substr(0, 6) == "logout")
        {
            std::string response = "Logout....\n";
            ssize_t bytesWritten = write(client._clientSocket, response.c_str(), response.length());

            client._isAuthenticated = false;
            login(client);
            // login(authenticated,clientSocket,serverSocket);
            if (bytesWritten == -1)
                std::cerr << "error while sending data to client" << std::endl;
            close(client._clientSocket);
        }
        else
        {
            std::string response = "Incorrect command\n";

            ssize_t bytesWritten = write(client._clientSocket, response.c_str(), response.length());
            if (bytesWritten == -1)
                std::cerr << "error while sending data to client" << std::endl;
        }
    }
    if (bytesRead == -1)
    {
        std::cerr << "Error while reading data from client(command handler)" << std::endl;
    }
}

int main()
{
    Client client;
    int serverSocket = initServerSocket();
    while (startServer(serverSocket) == 0)
    {

        client.initClientSocket(serverSocket);

        while (!client._isAuthenticated)
        {
            login(client);
        }

        while (client._isAuthenticated)
        {
            commandHandler(client);
        }
        close(client._clientSocket);
    }
    close(serverSocket);
    return 0;
}
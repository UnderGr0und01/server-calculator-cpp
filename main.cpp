#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sstream>
#include "chrono"
#include "thread"
#include <pqxx/pqxx>
#include "exprtk/exprtk.hpp"

int startServer(int serverSocket)
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

int initClientSocket(int serverSocket)
{
    sockaddr_in clientAddress{};
    socklen_t clientAddressLength = sizeof(clientAddress);
    int clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddress, &clientAddressLength);
    if (clientSocket == -1)
    {
        std::cerr << "Error while accept connection" << std::endl;
        close(serverSocket);
        return 1;
    }

    std::cout << "Connection success" << std::endl;
    return clientSocket;
}

bool authenticate(const std::string &username, const std::string &password)
{
    try
    {
        pqxx::connection connection("dbname=postgres user=postgres password=7456 hostaddr=172.17.0.2 port=5432");
        //        pqxx::connection connection("dbname=postgres user=postgres password=7456 hostaddr=127.0.0.1 port=5432");

        if (connection.is_open())
        {
            pqxx::work transaction(connection);
            std::string query = "SELECT COUNT(*) FROM users WHERE username = '" + username + "' AND password = '" + password + "';";

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

int getBalance(const std::string &username, const std::string &password)
{
    pqxx::connection connection("dbname=postgres user=postgres password=7456 hostaddr=172.17.0.2 port=5432");
    pqxx::work transaction(connection);
    std::string query = "SELECT balance FROM users WHERE username = '" + username + "' AND password= '" + password + "';";

    std::cout << query << std::endl;

    pqxx::result res = transaction.exec(query);
    int balance = res[0][0].as<int>();
    transaction.commit();
    return balance;
}

void updateBalance(const int &balance, const std::string &username, const std::string &password)
{
    pqxx::connection connection("dbname=postgres user=postgres password=7456 hostaddr=172.17.0.2 port=5432");
    pqxx::work transaction(connection);
    std::string query = "UPDATE users SET balance = " + std::to_string(balance) + " WHERE username = '" + username + "' AND password= '" + password + "';";
    pqxx::result res = transaction.exec(query);
    transaction.commit();
}

bool login(bool authenticated, const int &clientSocket)
{
    char buffer[1024];
    ssize_t bytesRead;

    std::string username, password;

    while ((bytesRead = read(clientSocket, buffer, sizeof(buffer))) > 0)
    {
        std::istringstream iss(buffer);
        std::string command;
        iss >> command;
        bytesRead = read(clientSocket, buffer, sizeof(buffer));
        std::cout << "iss = " << command << std::endl;
        if (command == "login")
        {
            iss >> username;

            std::istringstream iss2(buffer);
            std::string nextCommand;
            iss2 >> nextCommand;
            std::cout << "iss2 = " << nextCommand << std::endl;

            if (nextCommand == "logout")
            {
                close(clientSocket);
            }
            else if (nextCommand == "password")
            {
                iss2 >> password;
                authenticated = authenticate(username, password);
                std::string response = authenticated ? "Auth success!\n" : "Auth error: user does not exist.\n";
                ssize_t bytesWritten = write(clientSocket, response.c_str(), response.length());
                if (bytesWritten == -1)
                {
                    std::cerr << "Error while sending data to client" << std::endl;
                    close(clientSocket);
                    return 1;
                }
                return authenticated;
            }
        }
        else
        {
            std::string response = "You need to login! Type login <username>\n";

            ssize_t bytesWritten = write(clientSocket, response.c_str(), response.length());

            if (bytesWritten == -1)
            {
                std::cerr << "Error while sending data to client" << std::endl;
                close(clientSocket);
                return 1;
            }
        }
    }
    if (bytesRead == -1)
    {
        std::cerr << "Error while reading data from client" << std::endl;
    }
    return authenticated;
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
        logFile << std::endl;
        logFile.close();
    }
    else
    {
        std::cerr << "Error opening log file" << std::endl;
    }
}

double calculate(const std::string &expression) // TODO: handle empty expression
{
    typedef exprtk::expression<double> Expression;
    typedef exprtk::parser<double> Parser;

    Expression expr;
    Parser parser;

    if (!parser.compile(expression, expr))
    {
        throw std::runtime_error("Failed to compile expression");
    }

    return expr.value();
}

bool commandHandler(bool authenticated, int clientSocket)
{
    char buffer[1024];
    ssize_t bytesRead;

    while ((bytesRead = read(clientSocket, buffer, sizeof(buffer))) > 0)
    {
        std::istringstream isscommand(buffer);
        std::string command;

        std::cout << "Receive from client: " << std::string(buffer, bytesRead) << std::endl;

        //        std::string command(buffer, bytesRead);
        isscommand >> command;
        std::cout << "command = " << command << std::endl;
        std::string expression;

        //        if (command.substr(0, 4) == "calc")
        if (command == "calc")
        {
            //            std::string expression = command.substr(5);
            isscommand >> expression;
            std::cout << "expression = " << expression << std::endl;
            double result = calculate(expression);
            int balance = getBalance("admin", "admin");
            if (balance <= 0)
            {
                std::string response = "Yor don't have balance to this action\n";

                ssize_t bytesWritten = write(clientSocket, response.c_str(), response.length());
                if (bytesWritten == -1)
                    std::cerr << "Error while sending data to client" << std::endl;
            }
            else
            {
                balance--;
                updateBalance(balance, "admin", "admin");
                std::string response = std::to_string(result) + "\n" + "Now your balance = " + std::to_string(balance) + "\n";

                ssize_t bytesWritten = write(clientSocket, response.c_str(), response.length());

                if (bytesWritten == -1)
                    std::cerr << "Error while sending data to client" << std::endl;
            }
            logEntry("admin", expression, result);
        }
        else if (command.substr(0, 6) == "logout")
        {
            std::string response = "Logout....\n";
            ssize_t bytesWritten = write(clientSocket, response.c_str(), response.length());

            authenticated = false;
            login(authenticated, clientSocket);
            // login(authenticated,clientSocket,serverSocket);
            if (bytesWritten == -1)
                std::cerr << "error while sending data to client" << std::endl;
            close(clientSocket);
        }
        else
        {
            std::string response = "Incorrect command\n";

            ssize_t bytesWritten = write(clientSocket, response.c_str(), response.length());
            if (bytesWritten == -1)
                std::cerr << "error while sending data to client" << std::endl;
        }
    }
    if (bytesRead == -1)
    {
        std::cerr << "Error while reading data from client" << std::endl;
    }
    return authenticated;
}

int main()
{
    int serverSocket = initServerSocket();
    //    startServer(serverSocket);

    while (startServer(serverSocket) == 0)
    {

        int clientSocket = initClientSocket(serverSocket);

        bool authenticated = false;
        while (!authenticated)
        {
            authenticated = login(authenticated, clientSocket);
        }

        while (authenticated)
        {
            authenticated = commandHandler(authenticated, clientSocket);
        }
        // TODO: command logout
        close(clientSocket);
    }
    close(serverSocket);
    return 0;
}
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <cstdlib>
#include <sstream>
#include <pqxx/pqxx>

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

    // Ожидание подключений
    if (listen(serverSocket, 5) == -1)
    {
        std::cerr << "Error while waiting connection" << std::endl;
        close(serverSocket);
        return 1;
    }

    std::cout << "Server start and waiting connection..." << std::endl;
    return 0;
}

int initSocket()
{
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1)
    {
        std::cerr << "Error while create socket";
        return 1;
    }
    return serverSocket;
}

double calculate(const std::string &expression)
{
    std::istringstream iss(expression);
    double operand_1, operand_2;
    char operation;
    iss >> operand_1 >> operation >> operand_2;

    double result;
    switch (operation)
    {
    case '+':
        result = operand_1 + operand_2;
        break;
    case '-':
        result = operand_1 - operand_2;
        break;
    case '*':
        result = operand_1 * operand_2;
        break;
    case '/':
        result = operand_1 / operand_2;
        break;
    default:
        result = 0.0;
        break;
    }
    return result;
}

bool authenticate(const std::string &username, const std::string &password)
{
    try
    {
        pqxx::connection connection("dbname=postgres user=postgres password=7456 hostaddr=172.17.0.3 port=5432");
        // pqxx::connection connection("dbname=postgres user=postgres password=7456 hostaddr=127.0.0.1 port=5432");

        if (connection.is_open())
        {
            pqxx::work transaction(connection);
            std::string query = "SELECT COUNT(*) FROM users WHERE username = '" + username + "' AND password = '" + password + "';";

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

int main()
{
    int serverSocket = initSocket();
    startServer(serverSocket);

    while (true)
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

        bool authenticated = false;
        while (!authenticated)
        {
            char buffer[1024];
            ssize_t bytesRead = read(clientSocket, buffer, sizeof(buffer));
            if (bytesRead == -1)
            {
                std::cerr << "Error while reading socket" << std::endl;
                close(clientSocket);
                close(serverSocket);
                return 1;
            }

            std::string credentials(buffer, bytesRead);

            std::istringstream iss(credentials);

            std::string username, password;
            iss >> username >> password;
            std::cout << username << ' ' << password << std::endl;

            authenticated = authenticate(username, password);

            std::string response = authenticated ? "Auth success!\n" : "Auth error\n";
            ssize_t bytesWritten = write(clientSocket, response.c_str(), response.length());
            if (bytesWritten == -1)
            {
                std::cerr << "Error while wtire to socket" << std::endl;
                close(clientSocket);
                close(serverSocket);
                return 1;
            }
            if (authenticated)
            {
                std::cout << "Auth success. Waiting command... " << authenticated << std::endl;
            }
            else
            {
                std::cout << "Auth error. Retry...." << authenticated << std::endl;
            }
        }

        while (authenticated)
        {

            char buffer[1024];
            ssize_t bytesRead;
            while ((bytesRead = read(clientSocket, buffer, sizeof(buffer))) > 0)
            {

                std::cout << "Recive from client: " << std::string(buffer, bytesRead) << std::endl;

                std::string command(buffer, bytesRead);

                if (command.substr(0, 4) == "calc")
                {
                    std::string expression = command.substr(5);
                    double result = calculate(expression);

                    std::string response = std::to_string(result);

                    ssize_t bytesWritten = write(clientSocket, response.c_str(), response.length());

                    if (bytesWritten == -1)
                        std::cerr << "Error while sending data to client" << std::endl;
                }
                else if (command.substr(0,6) == "logout")
                {
                    authenticated = false;
                    std::string response = "Logout....\n";
                    ssize_t bytesWritten = write(clientSocket, response.c_str(), response.length());
                    if (bytesWritten == -1)
                        std::cerr << "error while sending data to client" << std::endl;
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
            authenticated = false;
        }
        // TODO: command logout
        close(clientSocket);
        close(serverSocket);
        std::cout << "Connection close" << std::endl;
    }

    close(serverSocket);

    return 0;
}
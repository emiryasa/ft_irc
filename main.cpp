// ircserv.cpp

#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include <cstring>
#include <cerrno>
#include <cstdlib>
#include <sstream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <poll.h>
#include <fcntl.h>

#define MAX_CLIENTS 100
#define BUFFER_SIZE 512

// Define colors for console output (optional)
#define RESET_COLOR "\033[0m"
#define RED_COLOR "\033[31m"
#define GREEN_COLOR "\033[32m"

class Client {
public:
    int fd;
    std::string nickname;
    std::string username;
    bool isAuthenticated;
    bool isOperator;

    Client(int fd) : fd(fd), isAuthenticated(false), isOperator(false) {}
};

class IRCServer {
private:
    int server_fd;
    std::string password;
    std::map<int, Client*> clients;
    std::vector<struct pollfd> poll_fds;

public:
    IRCServer(const std::string& port_str, const std::string& password);
    ~IRCServer();
    void run();

private:
    void initServer(const std::string& port_str);
    void acceptNewClient();
    void removeClient(int fd);
    void handleClientMessage(int fd);
    void sendMessage(int fd, const std::string& message);
    void broadcastMessage(const std::string& message, int exclude_fd = -1);
    void setNonBlocking(int fd);
    void parseCommand(Client* client, const std::string& message);
    void handlePASS(Client* client, const std::vector<std::string>& params);
    void handleNICK(Client* client, const std::vector<std::string>& params);
    void handleUSER(Client* client, const std::vector<std::string>& params);
    void handlePING(Client* client, const std::vector<std::string>& params);
    void handlePRIVMSG(Client* client, const std::vector<std::string>& params);
    // Add more command handlers as needed
};

IRCServer::IRCServer(const std::string& port_str, const std::string& password)
    : password(password) {
    initServer(port_str);
}

IRCServer::~IRCServer() {
    close(server_fd);
    for (std::map<int, Client*>::iterator it = clients.begin(); it != clients.end(); ++it) {
        close(it->first);
        delete it->second;
    }
}

void IRCServer::setNonBlocking(int fd) {
    fcntl(fd, F_SETFL, O_NONBLOCK);
}

void IRCServer::initServer(const std::string& port_str) {
    int port = atoi(port_str.c_str());

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        std::cerr << RED_COLOR << "Socket creation failed" << RESET_COLOR << std::endl;
        exit(EXIT_FAILURE);
    }

    setNonBlocking(server_fd);

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in address;
    std::memset(&address, 0, sizeof(address));

    address.sin_family = AF_INET; 

    address.sin_addr.s_addr = INADDR_ANY; // Listen on all interfaces
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << RED_COLOR << "Bind failed" << RESET_COLOR << std::endl;
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, SOMAXCONN) < 0) {
        std::cerr << RED_COLOR << "Listen failed" << RESET_COLOR << std::endl;
        exit(EXIT_FAILURE);
    }

    // Add server_fd to poll_fds
    struct pollfd server_pollfd;
    server_pollfd.fd = server_fd;
    server_pollfd.events = POLLIN;
    poll_fds.push_back(server_pollfd);

    std::cout << GREEN_COLOR << "Server listening on port " << port << RESET_COLOR << std::endl;
}

void IRCServer::run() {
    while (true) {
        int poll_count = poll(&poll_fds[0], poll_fds.size(), -1);

        if (poll_count == -1) {
            std::cerr << RED_COLOR << "Poll error: " << strerror(errno) << RESET_COLOR << std::endl;
            exit(EXIT_FAILURE);
        }

        for (size_t i = 0; i < poll_fds.size(); ++i) {
            if (poll_fds[i].revents & POLLIN) {
                if (poll_fds[i].fd == server_fd) {
                    acceptNewClient();
                } else {
                    handleClientMessage(poll_fds[i].fd);
                }
            }
        }
    }
}

void IRCServer::acceptNewClient() {
    sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);

    if (client_fd == -1) {
        std::cerr << RED_COLOR << "Accept failed" << RESET_COLOR << std::endl;
        return;
    }

    setNonBlocking(client_fd);

    if (clients.size() >= MAX_CLIENTS) {
        std::string msg = "ERROR :Server full\r\n";
        send(client_fd, msg.c_str(), msg.length(), 0);
        close(client_fd);
        return;
    }

    clients[client_fd] = new Client(client_fd);

    struct pollfd client_pollfd;
    client_pollfd.fd = client_fd;
    client_pollfd.events = POLLIN;
    poll_fds.push_back(client_pollfd);

    std::cout << GREEN_COLOR << "New client connected: FD " << client_fd << RESET_COLOR << std::endl;
}

void IRCServer::removeClient(int fd) {
    close(fd);
    delete clients[fd];
    clients.erase(fd);

    // Remove from poll_fds
    for (std::vector<struct pollfd>::iterator it = poll_fds.begin(); it != poll_fds.end(); ++it) {
        if (it->fd == fd) {
            poll_fds.erase(it);
            break;
        }
    }

    std::cout << RED_COLOR << "Client disconnected: FD " << fd << RESET_COLOR << std::endl;
}

void IRCServer::handleClientMessage(int fd) {
    char buffer[BUFFER_SIZE];
    int bytes_received = recv(fd, buffer, BUFFER_SIZE - 1, 0);

    if (bytes_received <= 0) {
        removeClient(fd);
        return;
    }

    buffer[bytes_received] = '\0';
    std::string message(buffer);

    Client* client = clients[fd];
    parseCommand(client, message);
}

void IRCServer::sendMessage(int fd, const std::string& message) {
    send(fd, message.c_str(), message.length(), 0);
}

void IRCServer::broadcastMessage(const std::string& message, int exclude_fd) {
    for (std::map<int, Client*>::iterator it = clients.begin(); it != clients.end(); ++it) {
        if (it->first != exclude_fd) {
            sendMessage(it->first, message);
        }
    }
}

void IRCServer::parseCommand(Client* client, const std::string& message) {
    std::string msg = message;
    size_t pos;
    while ((pos = msg.find("\r\n")) != std::string::npos) {
        std::string line = msg.substr(0, pos);
        msg.erase(0, pos + 2);

        if (line.empty()) continue;

        std::istringstream iss(line);
        std::string command;
        iss >> command;

        std::vector<std::string> params;
        std::string param;
        while (iss >> param) {
            params.push_back(param);
        }

        // Komutları büyük harfe çevir
        std::transform(command.begin(), command.end(), command.begin(), ::toupper);

        // Kullanıcı doğrulanmadıysa sadece PASS komutuna izin veriyoruz
        if (!client->isAuthenticated && command != "PASS") {
            sendMessage(client->fd, "ERROR :You must log in with PASS first\r\n");
            continue;
        }

        // Eğer kullanıcı adı setlenmediyse, NICK komutuna zorla
        if (client->nickname.empty() && command != "NICK" && command != "PASS") {
            sendMessage(client->fd, "ERROR :You must set a nickname first with NICK\r\n");
            continue;
        }

        // Komutları işliyoruz
        if (command == "PASS") {
            handlePASS(client, params);
        } else if (command == "NICK") {
            handleNICK(client, params);
        } else if (command == "USER") {
            handleUSER(client, params);
        } else if (command == "PING") {
            handlePING(client, params);
        } else if (command == "PRIVMSG") {
            handlePRIVMSG(client, params);
        } else if (command == "QUIT") {
            removeClient(client->fd);
        } else {
            // Bilinmeyen komut
            sendMessage(client->fd, "ERROR :Unknown command\r\n");
        }
    }
}

void IRCServer::handlePASS(Client* client, const std::vector<std::string>& params) {
    if (params.empty()) {
        sendMessage(client->fd, "ERROR :No password given\r\n");
        return;
    }

    if (params[0] == password) {
        client->isAuthenticated = true;
        sendMessage(client->fd, "NOTICE Auth :*** Password accepted\r\n");
    } else {
        sendMessage(client->fd, "ERROR :Invalid password\r\n");
        removeClient(client->fd);
    }
}

void IRCServer::handleNICK(Client* client, const std::vector<std::string>& params) {
    if (params.empty()) {
        sendMessage(client->fd, "ERROR :No nickname given\r\n");
        return;
    }

    std::string new_nickname = params[0];

    // Check if the nickname is already taken by another client
    for (const auto& pair : clients) {
        if (pair.second->nickname == new_nickname && pair.first != client->fd) {
            sendMessage(client->fd, "ERROR :Nickname is already in use\r\n");
            return;
        }
    }

    client->nickname = new_nickname;
    sendMessage(client->fd, "NOTICE Nick :*** Nickname set\r\n");
}


void IRCServer::handleUSER(Client* client, const std::vector<std::string>& params) {
    if (params.size() < 4) {
        sendMessage(client->fd, "ERROR :Not enough parameters\r\n");
        return;
    }

    client->username = params[0];
    sendMessage(client->fd, "NOTICE User :*** User registered\r\n");
}

void IRCServer::handlePING(Client* client, const std::vector<std::string>& params) {
    if (params.empty()) {
        sendMessage(client->fd, "ERROR :No PING token given\r\n");
        return;
    }

    std::string response = "PONG :" + params[0] + "\r\n";
    sendMessage(client->fd, response);
}

void IRCServer::handlePRIVMSG(Client* client, const std::vector<std::string>& params) {
    if (params.size() < 2) {
        sendMessage(client->fd, "ERROR :Not enough parameters\r\n");
        return;
    }

    std::string recipient = params[0];  // Hedef alıcı nick'i

    // Mesaj kısmını boşluklu olarak almak için tüm parametreleri birleştiriyoruz.
    std::string message = params[1];
    for (size_t i = 2; i < params.size(); ++i) {
        message += " " + params[i];
    }

    // Mesajın başına ':' karakteri eklemek gerekir
    if (message[0] != ':') {
        message = ":" + message;
    }

    // Alıcıyı bul
    Client* targetClient = nullptr;
    for (const auto& pair : clients) {
        if (pair.second->nickname == recipient) {
            targetClient = pair.second;
            break;
        }
    }

    if (targetClient) {
        // Mesajı hedef istemciye gönder
        std::string full_message = ":" + client->nickname + " PRIVMSG " + recipient + " " + message + "\r\n";
        sendMessage(targetClient->fd, full_message);
    } else {
        // Eğer alıcı bulunamazsa hata mesajı gönder
        sendMessage(client->fd, "ERROR :No such nick\r\n");
    }
}



// Main function
int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: ./ircserv <port> <password>" << std::endl;
        return EXIT_FAILURE;
    }

    std::string port = argv[1];
    std::string password = argv[2];

    IRCServer server(port, password);
    server.run();

    return EXIT_SUCCESS;
}

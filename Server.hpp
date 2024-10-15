#pragma once

#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <poll.h>
#include <fcntl.h>
#include <algorithm>
#include <cstring>
#include <cerrno>
#include <cstdlib>
#include <sstream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "Client.hpp"

#define MAX_CLIENTS 100
#define BUFFER_SIZE 512

#define RESET_COLOR "\033[0m"
#define RED_COLOR "\033[31m"
#define GREEN_COLOR "\033[32m"

class Server {
    private:
        int server_fd;
        std::string password;
        std::map<int, Client*> clients;
        std::vector<struct pollfd> poll_fds;

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
        void handleChannelMessage(const std::string &message) const;

    public:
        Server(const std::string& port_str, const std::string& password);
        ~Server();
        void run();

};
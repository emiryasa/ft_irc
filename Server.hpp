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
#include "Channel.hpp"

#define MAX_CLIENTS 100
#define BUFFER_SIZE 512

#define RESET_COLOR "\033[0m"
#define RED_COLOR "\033[31m"
#define GREEN_COLOR "\033[32m"

class Channel;

class Server {
    private:
        int server_fd;
        std::string password;
        std::map<int, Client*> clients;
        std::vector<struct pollfd> poll_fds;
        std::map<std::string, Channel*> channels;

        typedef void (Server::*CommandFunc)(Client*, const std::vector<std::string>&);
        typedef void (Server::*ChannelCommandFunc)(Client*, const std::vector<std::string>&);

        std::map<std::string, CommandFunc> command_map;
        std::map<std::string, ChannelCommandFunc> channel_command_map;

        void initServer(const std::string& port_str);
        void acceptNewClient();
        void removeClient(Client *client, const std::vector<std::string>& params);
        void handleClientMessage(int fd);
        void setNonBlocking(int fd);
        void createChannel(Client *client, const std::vector<std::string>& params);
        void joinChannel(Client *client, const std::vector<std::string>& params);
        void listChannels(Client *client, const std::vector<std::string>& params);
        void leaveChannel(Client* client, const std::vector<std::string>& params);
        void deleteChannel(Client *client, const std::vector<std::string>& params);
        void kickMemberFromChannel(Client *client, const std::vector<std::string>& params);
        void addOpToChannel(Client* client, const std::vector<std::string>& params);
        void parseCommand(Client* client, const std::string& message);
        void registerCommands();
        void handlePASS(Client*, const std::vector<std::string>& params);
        void handleNICK(Client* client, const std::vector<std::string>& params);
        void handleUSER(Client* client, const std::vector<std::string>& params);
        void handlePING(Client* client, const std::vector<std::string>& params);
        void handlePRIVMSG(Client* client, const std::vector<std::string>& params);

    public:
        Server(const std::string& port_str, const std::string& password);
        ~Server();
        void run();
        void sendMessage(int fd, const std::string& message);
};
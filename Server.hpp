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
        typedef void (Server::*ChannelCommandFunc)(Channel*, Client*, const std::vector<std::string>&);

        std::map<std::string, CommandFunc> command_map;
        std::map<std::string, ChannelCommandFunc> channel_command_map;

        // server functions
        void initServer(const std::string& port_str);
        void acceptNewClient();
        void removeClient(Client *client, const std::vector<std::string>& params);
        void handleClientMessage(int fd);
        void setNonBlocking(int fd);
        void parseCommand(Client* client, const std::string& message);
        void registerCommands();
    
        // channel commands
        void leaveChannel(Channel *channel, Client* client, const std::vector<std::string>& params);
        void deleteChannel(Channel *channel, Client* client, const std::vector<std::string>& params);
        void kickMemberFromChannel(Channel *channel, Client* client, const std::vector<std::string>& params);
        void listChannelMembers(Channel *channel, Client *client, const std::vector<std::string>& params);
        void addOpToChannel(Channel *channel, Client* client, const std::vector<std::string>& params);

        // server commands
        void handlePASS(Client*, const std::vector<std::string>& params);
        void handleNICK(Client* client, const std::vector<std::string>& params);
        void handleUSER(Client* client, const std::vector<std::string>& params);
        void handlePING(Client* client, const std::vector<std::string>& params);
        void handlePRIVMSG(Client* client, const std::vector<std::string>& params);
        void createChannel(Client *client, const std::vector<std::string>& params);
        void joinChannel(Client *client, const std::vector<std::string>& params);
        void listChannels(Client *client, const std::vector<std::string>& params);
        void sendWelcomeMessage(Client *client);
        const std::string Prefix(Client *client) const;

    public:
        Server(const std::string& port_str, const std::string& password);
        ~Server();
        void run();
        void sendMessage(int fd, const std::string& message);
};
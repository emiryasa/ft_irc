#pragma once

#include <iostream>
#include <string>
#include <set>
#include <vector>
#include "Client.hpp"
#include "Server.hpp"

class Server;

class Channel {
    private:
        std::string         _name;
        std::string         _password;
        std::set<Client*>   _members;
        std::set<Client*>   _ops;
        Server              *_server;
        std::vector<std::string> _blacklist;

    public:
        Channel();
        Channel(const std::string &name, Server *server);
        std::string getName() const;
        std::set<Client *> getMembers() const;
        std::string getPassword() const;
        void setPassword(const std::string &password);
        void addOp(Client *client);
        bool isOp(Client *client) const;
        bool isMember(Client *client) const;
        void leaveChannel(Client* client);
        void deleteChannel(Client *client);
        void broadcastMessage(const std::string &message, Client *client);
        void addMember(Client *client);
        void kickMember(Client *client, const std::string& nickname);
        void listMembers(Client *client);
        bool isBlacklisted(Client *client) const;
        ~Channel();
};

#pragma once

#include <iostream>
#include <string>
#include <set>
#include "Client.hpp"
#include "Server.hpp"

class Server;

class Channel {
    private:
        std::string         _name;
        std::string         _password;
        std::string         _topic;
        std::set<Client*>   _members;
        std::set<Client*>   _ops;
        Server              *_server;

    public:
        Channel();
        Channel(const std::string &name, Server *server);
        std::string getName() const;
        std::set<Client *> getMembers() const;
        std::string getTopic() const;
        std::string getPassword() const;
        void setPassword(const std::string &password);
        void setTopic(const std::string &topic);
        void addOp(Client *client);
        bool isOp(Client *client) const;
        bool isMember(Client *client) const;
        void leaveChannel(Client* client);
        void deleteChannel(Client *client);
        void changeTopic(const std::string &newTopic, Client *requester);
        void broadcastMessage(const std::string &message, Client *client);
        void addMember(Client *client);
        void kickMember(Client *client, const std::string& nickname);
        ~Channel();
};

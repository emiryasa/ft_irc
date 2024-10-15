#pragma once

#include <iostream>
#include <string>
#include <set>
#include "Client.hpp"


class Channel {
    public:
        std::string _name;
        // set kullanmamızın sebebi her bir üyenin özgün olması. bir üye kanaldayken tekrar katılamamalı.
        std::set<Client*> members;

    private:
        Channel();
        Channel(const std::string &name);
        void getName() const;
        void getMembers() const;
        void addMember(const Client *client);
        void kickMember(const Client *client);
        void sendMessageToChannel(const std::string &message);
        ~Channel();
};

// std::map<std::string, Channel*> channels;

// void createChannel(const std::string& name) {
//     if (channels.find(name) == channels.end()) {
//         channels[name] = new Channel(name);
//         std::cout << GREEN_COLOR << "Channel created: " << name << RESET_COLOR << std::endl;
//     }
// }

// void joinChannel(Client* client, const std::string& name) {
//     if (channels.find(name) != channels.end()) {
//         channels[name]->addMember(client);
//         std::cout << GREEN_COLOR << client->getNickname() << " joined channel: " << name << RESET_COLOR << std::endl;
//     } else {
//         sendMessage(client->getFd(), "ERROR :Channel does not exist\r\n");
//     }
// }

// if (command == "JOIN") {
//     if (params.empty()) {
//         sendMessage(client->getFd(), "ERROR :No channel name given\r\n");
//     } else {
//         joinChannel(client, params[0]);
//     }
// } else if (command == "CREATE") {
//     if (params.empty()) {
//         sendMessage(client->getFd(), "ERROR :No channel name given\r\n");
//     } else {
//         createChannel(params[0]);
//     }
// }

// void broadcastMessageToChannel(const std::string& channelName, const std::string& message, Client* sender) {
//     if (channels.find(channelName) != channels.end()) {
//         for (Client* member : channels[channelName]->getMembers()) {
//             if (member != sender) {
//                 sendMessage(member->getFd(), message);
//             }
//         }
//     }
// }

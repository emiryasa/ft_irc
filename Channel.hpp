#pragma once

#include <iostream>
#include <string>
#include <set>
#include "Client.hpp"


class Channel {
    private:
        std::string _name;
        std::string _password;
        std::set<Client*> _members;

    public:
        Channel();
        Channel(const std::string &name);
        std::string getName() const;
        std::set<Client *> getMembers() const;
        std::string getPassword() const;
        void setPassword(const std::string password);
        void addMember(Client *client);
        void kickMember(Client *client);
        ~Channel();
};

#pragma once

#include <iostream>
#include <string>
#include <set>
#include "Client.hpp"


class Channel {
    private:
        std::string _name;
        std::set<Client*> _members;

    public:
        Channel();
        Channel(const std::string &name);
        std::string getName() const;
        std::set<Client *> getMembers() const;
        void addMember(Client *client);
        void kickMember(Client *client);
        ~Channel();
};

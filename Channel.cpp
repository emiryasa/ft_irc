#include "Channel.hpp"

Channel::Channel() {}

Channel::Channel(const std::string &name): _name(name) {}

Channel::~Channel() {}

std::string Channel::getName() const {
    return _name;
}

std::set<Client*> Channel::getMembers() const {
    return _members;
}

std::string Channel::getPassword() const {
    return _password;
}

void Channel::setPassword(const std::string password) {
    _password = password;
}

void Channel::addMember(Client *client) {
    _members.insert(client);
}

void Channel::kickMember(Client *client) {
    _members.erase(client);
}

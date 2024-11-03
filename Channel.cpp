#include "Channel.hpp"

Channel::Channel() {}

Channel::Channel(const std::string &name, Server *server): _name(name), _server(server) {}

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

void Channel::setPassword(const std::string &password) {
    _password = password;
}

void Channel::addOp(Client *client) {
    _ops.insert(client);
}

bool Channel::isOp(Client *client) const{
    return _ops.find(client) != _ops.end();
}

void Channel::addMember(Client *client) {
    _members.insert(client);
}

void Channel::kickMember(Client* client, const std::string& nickname) {
    if (isOp(client)) {
    for (std::set<Client*>::iterator it = _members.begin(); it != _members.end(); ++it) {
        if ((*it)->getNickname() == nickname) {
            _server->sendMessage((*it)->getFd(), "You have been kicked from the channel\r\n");
            _members.erase(*it);
            _blacklist.push_back(nickname);
            std::cout << RED_COLOR << nickname << " has been kicked from the channel " << getName() << RESET_COLOR << std::endl;
            _server->sendMessage(client->getFd(), "SUCCESS :User has been kicked from the channel\r\n");
            return;
        }
    }
    _server->sendMessage(client->getFd(), "ERROR :User not found in this channel\r\n");
    } else {
        _server->sendMessage(client->getFd(), "ERROR :You are not op\r\n");
    }
}

void Channel::broadcastMessage(const std::string &message, Client *client) {
    for (std::set<Client *>::iterator it = _members.begin(); it != _members.end(); ++it) {
        if (client != *it) {
            std::string formatted_message = client->getNickname() + ": " + message;
            _server->sendMessage((*it)->getFd(), formatted_message);
        }
    }
}

bool Channel::isBlacklisted(Client *client) const {
    return std::find(_blacklist.begin(), _blacklist.end(), client->getNickname()) != _blacklist.end();
}

bool Channel::isMember(Client *client) const {
    return _members.find(client) != _members.end();
}

void Channel::leaveChannel(Client* client) {
        _members.erase(client);
    
        std::string message = client->getNickname() + " has left the channel " + _name + "\r\n";

        broadcastMessage(message, client);
}

void Channel::deleteChannel(Client *client) {
    if (isOp(client)) {
        std::string message = "Channel " + _name + " has been deleted by " + client->getNickname() + "\r\n";
        broadcastMessage(message, client);

        for (std::set<Client *>::iterator it = _members.begin(); it != _members.end(); ) {
            Client* member = *it;
            ++it;
            leaveChannel(member);
        }

        _server->removeChannel(this);

        std::cout << "Channel " << _name << " deleted." << std::endl;
        delete this;
    } else {
        _server->sendMessage(client->getFd(), "ERROR :You are not op\r\n");
    }
}

void Channel::listMembers(Client *client) {
    std::string member_list = "Members of channel " + _name + ":\r\n";
    for (std::set<Client *>::iterator it = _members.begin(); it != _members.end(); ++it) {
        member_list += (*it)->getNickname() + "\r\n";
    }
    _server->sendMessage(client->getFd(), member_list);
}

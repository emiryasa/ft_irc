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

std::string Channel::getTopic() const {
    return _topic;
}

std::string Channel::getPassword() const {
    return _password;
}

void Channel::setPassword(const std::string &password) {
    _password = password;
}

void Channel::setTopic(const std::string &topic) {
    _topic = topic;
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
            std::cout << RED_COLOR << nickname << " has been kicked from the channel." << RESET_COLOR << std::endl;
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

        for (std::set<Client *>::iterator it = _members.begin(); it != _members.end(); ++it) {
            leaveChannel(client);
        }

        delete this;
        std::cout << "Channel " << _name << " deleted." << std::endl;
    } else {
        _server->sendMessage(client->getFd(), "ERROR :You are not op\r\n");
    }
}

void Channel::changeTopic(const std::string &topic, Client *client) {
    if (isOp(client)) {
        setTopic(topic);
        broadcastMessage("Channel's topic changed to: " + topic, client);
    } else {
        _server->sendMessage(client->getFd(), "ERROR :You are not op\r\n");
    }
}
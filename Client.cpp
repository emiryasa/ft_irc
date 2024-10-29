#include "Client.hpp"

Client::Client(int fd): _fd(fd), _isAuthenticated(false), _isOperator(false) {}

int Client::getFd() const {
    return _fd;
}

const std::string &Client::getNickname() const {
    return _nickname;
}

const std::string &Client::getUsername() const {
    return _username;
}

const std::string &Client::getHostname() const {
    return _hostname;
}

const std::string &Client::getServername() const {
    return _servername;
}

const std::string &Client::getRealname() const {
    return _realname;
}

bool Client::getIsAuthenticated() const {
    return _isAuthenticated;
}

bool Client::getIsOperator() const {
    return _isOperator;
}

void Client::setFd(int fd) {
    _fd = fd;
}

void Client::setNickname(const std::string &nickname) {
    _nickname = nickname;
}

void Client::setUsername(const std::string &username) {
    _username = username;
}

void Client::setHostname(const std::string &hostname) {
    _hostname = hostname;
}

void Client::setServername(const std::string &servername) {
    _servername = servername;
}

void Client::setRealname(const std::string &realname) {
    _realname = realname;
}

void Client::setIsAuthenticated(bool isAuthenticated) {
    _isAuthenticated = isAuthenticated;
}

void Client::setIsOperator(bool isOperator) {
    _isOperator = isOperator;
}

Client::~Client() {}
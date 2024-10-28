#pragma once

#include <iostream>
#include <string>

class Client {
    private:
        int         _fd;
        std::string _nickname;
        std::string _username;
        bool        _isAuthenticated;
        bool        _isOperator;

    public:
        std::string buffer;
        Client(int fd);

        int getFd() const;
        const std::string &getNickname() const;
        const std::string &getUsername() const;
        bool getIsAuthenticated() const;
        bool getIsOperator() const;

        void setFd(int newFd);
        void setNickname(const std::string &newNickname);
        void setUsername(const std::string &newUsername);
        void setIsAuthenticated(bool authenticated);
        void setIsOperator(bool isOp);
    
        ~Client();
};
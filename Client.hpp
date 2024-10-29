#pragma once

#include <iostream>
#include <string>

class Client {
    private:
        int         _fd;
        std::string _nickname;
        std::string _username;
        std::string _hostname;
        std::string _servername;
        std::string _realname;
        bool        _isAuthenticated;
        bool        _isOperator;

    public:
        std::string buffer;
        Client(int fd);

        int getFd() const;
        const std::string &getNickname() const;
        const std::string &getUsername() const;
        const std::string &getHostname() const;
        const std::string &getServername() const;
        const std::string &getRealname() const;
        
        bool getIsAuthenticated() const;
        bool getIsOperator() const;

        void setFd(int newFd);
        void setNickname(const std::string &nickname);
        void setUsername(const std::string &username);
        void setHostname(const std::string &hostname);
        void setServername(const std::string &servername);
        void setRealname(const std::string &realname);
        void setIsAuthenticated(bool authenticated);
        void setIsOperator(bool isOp);
    
        ~Client();
};
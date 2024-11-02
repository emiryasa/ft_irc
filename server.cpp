#include "Server.hpp"

Server::Server(const std::string& port_str, const std::string& password): password(password) {
    initServer(port_str);
}

Server::~Server() {
    close(server_fd);
    for (std::map<int, Client*>::iterator it = clients.begin(); it != clients.end(); ++it) {
        close(it->first);
        delete it->second;
    }
    for (std::map<std::string, Channel*>::iterator it = channels.begin(); it != channels.end(); ++it) {
        delete it->second;
    }
}

void Server::setNonBlocking(int fd) {
    fcntl(fd, F_SETFL, O_NONBLOCK);
}

void Server::initServer(const std::string& port_str) {
    int port = std::atoi(port_str.c_str());

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        std::cerr << RED_COLOR << "Socket creation failed" << RESET_COLOR << std::endl;
        std::exit(EXIT_FAILURE);
    }

    setNonBlocking(server_fd);

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        std::cerr << RED_COLOR << "Set socket options failed" << RESET_COLOR << std::endl;
    }

    sockaddr_in address;
    std::memset(&address, 0, sizeof(address));

    address.sin_family = AF_INET; 

    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << RED_COLOR << "Bind failed" << RESET_COLOR << std::endl;
        std::exit(EXIT_FAILURE);
    }

    if (listen(server_fd, SOMAXCONN) < 0) {
        std::cerr << RED_COLOR << "Listen failed" << RESET_COLOR << std::endl;
        std::exit(EXIT_FAILURE);
    }

    struct pollfd server_pollfd;
    server_pollfd.fd = server_fd;
    server_pollfd.events = POLLIN;
    poll_fds.push_back(server_pollfd);

    registerCommands();

    std::cout << GREEN_COLOR << "Server listening on port " << port << RESET_COLOR << std::endl;
}

void Server::run() {
    while (true) {
        int poll_count = poll(&poll_fds[0], poll_fds.size(), -1);

        if (poll_count == -1) {
            std::cerr << RED_COLOR << "Poll error: " << strerror(errno) << RESET_COLOR << std::endl;
            std::exit(EXIT_FAILURE);
        }

        for (size_t i = 0; i < poll_fds.size(); ++i) {
            if (poll_fds[i].revents & POLLIN) {
                if (poll_fds[i].fd == server_fd) {
                    acceptNewClient();
                } else {
                    handleClientMessage(poll_fds[i].fd);
                }
            }
        }
    }
}

void Server::acceptNewClient() {
    sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);

    if (client_fd == -1) {
        std::cerr << RED_COLOR << "Accept failed" << RESET_COLOR << std::endl;
        return;
    }

    setNonBlocking(client_fd);

    if (clients.size() >= MAX_CLIENTS) {
        std::string msg = "ERROR :Server full\r\n";
        send(client_fd, msg.c_str(), msg.length(), 0);
        close(client_fd);
        return;
    }

    clients[client_fd] = new Client(client_fd);

    struct pollfd client_pollfd;
    client_pollfd.fd = client_fd;
    client_pollfd.events = POLLIN;
    poll_fds.push_back(client_pollfd);

    std::cout << GREEN_COLOR << "New client connected: FD " << client_fd << RESET_COLOR << std::endl;
    sendMessage(client_fd, "You must log in with PASS first\r\n");
}

void Server::removeClient(Client *client, const std::vector<std::string>& params) {
    (void)params;
    int fd = client->getFd();

    close(fd);
    delete clients[fd];
    clients.erase(fd);

    for (std::vector<struct pollfd>::iterator it = poll_fds.begin(); it != poll_fds.end(); ++it) {
        if (it->fd == fd) {
            poll_fds.erase(it);
            break;
        }
    }

    std::cout << RED_COLOR << "Client disconnected: FD " << fd << RESET_COLOR << std::endl;
}

void Server::handleClientMessage(int fd) {
    char buffer[BUFFER_SIZE];
    int bytes_received = recv(fd, buffer, BUFFER_SIZE - 1, 0);

    if (bytes_received <= 0) {
        Client* client = clients[fd];
        std::vector<std::string> params;
        removeClient(client, params);
        return;
    }
    
    buffer[bytes_received] = '\0';
    std::string message(buffer);

    std::cout << "Message received: " << buffer << std::endl;
    Client* client = clients[fd];

    parseCommand(client, message);
}

void Server::sendMessage(int fd, const std::string& message) {
    send(fd, message.c_str(), message.length(), 0);
}

void Server::sendWelcomeMessage(Client *client) {
    sendMessage(client->getFd(), ":" + client->getHostname() + " 001 " + client->getNickname() + " :Welcome to the Internet Relay Network " + client->getNickname() + "!" + "@" + client->getHostname() + "\r\n");
    sendMessage(client->getFd(), ":" + client->getHostname() + " 002 " + client->getNickname() + " :Your host is " + client->getHostname() + "\r\n");
    sendMessage(client->getFd(), ":" + client->getHostname() + " 003 " + client->getNickname() + " :This server was created\r\n");
}

void Server::registerCommands() {
    command_map["PASS"] = &Server::handlePASS;
    command_map["NICK"] = &Server::handleNICK;
    command_map["USER"] = &Server::handleUSER;
    command_map["PING"] = &Server::handlePING;
    command_map["CREATE"] = &Server::createChannel;
    command_map["JOIN"] = &Server::joinChannel;
    command_map["LIST"] = &Server::listChannels;
    command_map["PRIVMSG"] = &Server::handlePRIVMSG;
    command_map["QUIT"] = &Server::removeClient;

    channel_command_map["DELETE"] = &Server::deleteChannel;
    channel_command_map["LEAVE"] = &Server::leaveChannel;
    channel_command_map["ADDOP"] = &Server::addOpToChannel;
    channel_command_map["KICK"] = &Server::kickMemberFromChannel;
    channel_command_map["LSTMEMBERS"] = &Server::listChannelMembers;
}

void Server::parseCommand(Client* client, const std::string& message) {
    std::string msg = message;
    size_t pos;
    while ((pos = msg.find("\n")) != std::string::npos) {
        std::string line = msg.substr(0, pos);
        msg.erase(0, pos + 2);

        if (line.empty()) continue;

        std::istringstream iss(line);
        std::string command;
        iss >> command;

        std::vector<std::string> params;
        std::string param;
        while (iss >> param) {
            params.push_back(param);
        }

        if (!client->getIsAuthenticated() && command != "PASS") {
            sendMessage(client->getFd(), Prefix(client) + "You must log in with PASS first\r\n");
            continue;
        }

        if (client->getNickname().empty() && command != "NICK" && command != "PASS") {
            sendMessage(client->getFd(), Prefix(client) + "ERROR You must set a nickname first with NICK\r\n");
            continue;
        }

        if (client->getHostname().empty() && command != "USER" && command != "PASS" && command != "NICK") {
            sendMessage(client->getFd(), Prefix(client) + "ERROR You must set a username first with USER\r\n");
            continue;
        }

    for (std::map<std::string, Channel*>::iterator it = channels.begin(); it != channels.end(); ++it) {
        if (it->second->isMember(client)) {
            if (channel_command_map.find(command) != channel_command_map.end()) {
                (this->*channel_command_map[command])(it->second, client, params);
            }
            else {
                it->second->broadcastMessage(message, client);
            }
            return;
        }
    }

    if (command_map.find(command) != command_map.end()) {
        (this->*command_map[command])(client, params);
    } else {
        sendMessage(client->getFd(), Prefix(client) + "ERROR :Unknown command\r\n");
    }
    }
}

void Server::listChannels(Client *client, const std::vector<std::string>& params) {
    (void)params;

    if (channels.empty()) {
        sendMessage(client->getFd(), Prefix(client) + "ERRROR :No available channels.\r\n");
        return;
    }

    std::string channel_list = "Available channels:\r\n";
    for (std::map<std::string, Channel*>::iterator it = channels.begin(); it != channels.end(); ++it) {
        std::string channel_name = it->first;
        int member_count = it->second->getMembers().size();

        std::stringstream ss;
        ss << member_count;

        channel_list += channel_name + " (" + ss.str() + " members)\r\n";
    }

    sendMessage(client->getFd(), Prefix(client) + channel_list);
}

void Server::deleteChannel(Channel *channel, Client *client, const std::vector<std::string>& params) {
    (void)params;

    channel->deleteChannel(client);
}

void Server::removeChannel(Channel* channel) {
    channels.erase(channel->getName());
}

void Server::leaveChannel(Channel *channel, Client* client,  const std::vector<std::string>& params) {
    (void)params;

    channel->leaveChannel(client);
}

void Server::listChannelMembers(Channel *channel, Client *client, const std::vector<std::string>& params) {
    (void)params;

    channel->listMembers(client);
}

void Server::createChannel(Client *client, const std::vector<std::string>& params) {
    if (params.size() < 2) {
        sendMessage(client->getFd(), Prefix(client) + "Error: Usage: CREATE <channel name> <password>\r\n");
        return;
    }

    std::string name = params[0];
    std::string pass = params[1];

    if (channels.find(name) != channels.end()) {
        sendMessage(client->getFd(), Prefix(client) + "Error: Channel already exists.\r\n");
        return;
    }

    Channel* new_channel = new Channel(name, this);
    new_channel->setPassword(pass);
    new_channel->addMember(client);
    new_channel->addOp(client);
    channels[name] = new_channel;

    sendMessage(client->getFd(), Prefix(client) + "Channel created successfully: " + name + "\r\n");
}

void Server::joinChannel(Client *client, const std::vector<std::string>& params) {
    if (params.size() != 2) {
        sendMessage(client->getFd(), Prefix(client) + "Error Usage: JOIN <channel name> <password>\r\n");
        return;
    }

    std::string name = params[0];
    std::string pass = params[1];

    if (channels.find(name) == channels.end())
        sendMessage(client->getFd(), Prefix(client) + "ERROR :Channel does not exist\r\n");
    else {
        if (channels[name]->isBlacklisted(client)) {
            sendMessage(client->getFd(), Prefix(client) + "ERROR :You have been banned from this channel\r\n");
            return;
        }
        if (channels[name]->getPassword() == pass) {
            channels[name]->addMember(client);
            std::cout << GREEN_COLOR << client->getNickname() << " joined channel: " << name << RESET_COLOR << std::endl;
            channels[name]->broadcastMessage(client->getNickname() + " has joined the channel", client);
            sendMessage(client->getFd(), Prefix(client) + "SUCCESS :You have joined the channel\r\n");
        }
        else
            sendMessage(client->getFd(), Prefix(client) + "ERROR :Invalid password\r\n");
    }
}

void Server::kickMemberFromChannel(Channel *channel, Client* client, const std::vector<std::string>& params) {
    if (params.size() != 1) {
        sendMessage(client->getFd(), Prefix(client) + "ERROR :No nickname given\r\n");
        return;
    }

    std::string nickname = params[0];

    channel->kickMember(client, nickname);
}

void Server::addOpToChannel(Channel *channel, Client* client, const std::vector<std::string>& params) {
    if (params.size() != 1) {
        sendMessage(client->getFd(), Prefix(client) + "ERROR :No nickname given\r\n");
        return;
    }

    std::string nickname = params[0];
    if (channel->isOp(client)) {
        if (client->getNickname() == nickname) 
        {
            sendMessage(client->getFd(), Prefix(client) + "ERROR :You cannot make yourself op\r\n");
            return;
        }
        std::set<Client *> members = channel->getMembers();
        for (std::set<Client*>::iterator memberIt = members.begin(); memberIt != members.end(); ++memberIt) {

            if ((*memberIt)->getNickname() == nickname) {
                channel->addOp(*memberIt);
                sendMessage(client->getFd(), Prefix(client) + "SUCCESS :User has been made op\r\n");
                return;
            }
        }
        sendMessage(client->getFd(), Prefix(client) + "ERROR :User not found in this channel\r\n");
        return;
    }
    sendMessage(client->getFd(), Prefix(client) + "ERROR :You are not op\r\n");
}

void Server::handlePASS(Client* client, const std::vector<std::string>& params) {
    if (params.empty()) {
        sendMessage(client->getFd(), Prefix(client) + "ERROR :No password given\r\n");
        return;
    }

    if (params[0] == password) {
        client->setIsAuthenticated(true);
        sendMessage(client->getFd(), Prefix(client) + "SUCCESS :Auth Password accepted\r\n");
    } else {
        sendMessage(client->getFd(), Prefix(client) + "ERROR :Invalid password\r\n");
    }
}

void Server::handleNICK(Client* client, const std::vector<std::string>& params) {
    if (params.empty()) {
        sendMessage(client->getFd(), Prefix(client) + "ERROR :No nickname given\r\n");
        return;
    }

    std::string new_nickname = params[0];

    for (std::map<int, Client*>::const_iterator it = clients.begin(); it != clients.end(); ++it) {
        if (it->second->getNickname() == new_nickname && it->first != client->getFd()) {
            sendMessage(client->getFd(), "ERROR :Nickname is already in use\r\n");
            return;
        }
    }

    client->setNickname(new_nickname);
    sendMessage(client->getFd(), Prefix(client) + "SUCCESS :Nickname set to " + new_nickname + "\r\n");
}

void Server::handleUSER(Client* client, const std::vector<std::string>& params) {
    if (params.size() != 4) {
        sendMessage(client->getFd(), Prefix(client) + "ERROR : Usage: USER <username> <hostname> <servername> <realname>\r\n");
        return;
    }

    client->setUsername(params[0]);
    client->setHostname(params[1]);
    client->setServername(params[2]);
    client->setRealname(params[3]);
    
    sendMessage(client->getFd(), Prefix(client) + "SUCCESS :User registered\r\n");
    sendWelcomeMessage(client);
}

void Server::handlePING(Client* client, const std::vector<std::string>& params) {
    if (params.size() != 1) {
        sendMessage(client->getFd(), Prefix(client) + "ERROR :No PING token given\r\n");
        return;
    }

    std::string response = "PONG :" + params[0] + "\r\n";
    sendMessage(client->getFd(), Prefix(client) + response);
}

void Server::handlePRIVMSG(Client* client, const std::vector<std::string>& params) {
    if (params.size() < 2) {
        sendMessage(client->getFd(),Prefix(client) +  "ERROR :Not enough parameters\r\n");
        return;
    }

    std::string recipient = params[0];
    std::string message = params[1];

    for (size_t i = 2; i < params.size(); ++i) {
        message += " " + params[i];
    }

    Client* targetClient = NULL;
    for (std::map<int, Client*>::const_iterator it = clients.begin(); it != clients.end(); ++it) {
        if (it->second->getNickname() == recipient) {
            targetClient = it->second;
            break;
        }
    }

    if (targetClient) {
        std::string full_message = ":" + client->getNickname() + " PRIVMSG " + recipient + " " + message + "\r\n";
        sendMessage(targetClient->getFd(), Prefix(client) + full_message);
    } else {
        sendMessage(client->getFd(), Prefix(client) + "ERROR :No such nick\r\n");
    }
}

const std::string Server::Prefix(Client *client) const {
    return ":" + client->getNickname() + "!" + client->getUsername() + "@" + client->getHostname() + " ";
}

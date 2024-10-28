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
}

void Server::setNonBlocking(int fd) {
    fcntl(fd, F_SETFL, O_NONBLOCK);
}

void Server::initServer(const std::string& port_str) {
    int port = atoi(port_str.c_str());

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        std::cerr << RED_COLOR << "Socket creation failed" << RESET_COLOR << std::endl;
        exit(EXIT_FAILURE);
    }

    setNonBlocking(server_fd);

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in address;
    std::memset(&address, 0, sizeof(address));

    address.sin_family = AF_INET; 

    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << RED_COLOR << "Bind failed" << RESET_COLOR << std::endl;
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, SOMAXCONN) < 0) {
        std::cerr << RED_COLOR << "Listen failed" << RESET_COLOR << std::endl;
        exit(EXIT_FAILURE);
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
            exit(EXIT_FAILURE);
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

// void Server::parseChannelCommand(Channel *channel, Client* client, const std::string& message) {
//     std::string msg = message;
//     size_t pos;
//     while ((pos = msg.find("\n")) != std::string::npos) {
//         std::string line = msg.substr(0, pos);
//         msg.erase(0, pos + 2);

//         if (line.empty()) continue;

//         std::istringstream iss(line);
//         std::string command;
//         iss >> command;

//         std::vector<std::string> params;
//         std::string param;
//         while (iss >> param) {
//             params.push_back(param);
//         }
//         std::cout << "Command in Channel: " << command << std::endl;

//         if (command == "MSG") {
//             channel->broadcastMessage(message + "\r\n", client);
//         } else if (command == "DELETE") {
//             deleteChannel(client);
//         } else if (command == "LEAVE") {
//             leaveChannel(client);
//         } else if (command == "KICK") {
//             if (params.size() != 1) {
//                 sendMessage(client->getFd(), "ERROR :Usage: KICK <nickname>\r\n");
//                 return;
//             }
//             channel->kickMember(client, params[0]);
//         }
//         else {
//             sendMessage(client->getFd(), "ERROR :Unknown command\r\n");
//         }
//     }
// }

// void Server::parseCommand(Client* client, const std::string& message) {
//     std::string msg = message;
//     size_t pos;
//     while ((pos = msg.find("\n")) != std::string::npos) {
//         std::string line = msg.substr(0, pos);
//         msg.erase(0, pos + 2);

//         if (line.empty()) continue;

//         std::istringstream iss(line);
//         std::string command;
//         iss >> command;

//         std::vector<std::string> params;
//         std::string param;
//         while (iss >> param) {
//             params.push_back(param);
//         }
//         std::cout << command << std::endl;

//         if (!client->getIsAuthenticated() && command != "PASS") {
//             sendMessage(client->getFd(), "You must log in with PASS first\n");
//             continue;
//         }

//         if (client->getNickname().empty() && command != "NICK" && command != "PASS") {
//             sendMessage(client->getFd(), "ERROR You must set a nickname first with NICK\r\n");
//             continue;
//         }

//         if (command == "PASS") {
//             handlePASS(client, params);
//         } else if (command == "NICK") {
//             handleNICK(client, params);
//         } else if (command == "USER") {
//             handleUSER(client, params);
//         } else if (command == "PING") {
//             handlePING(client, params);
//         } else if (command == "CREATE") {
//             createChannel(client, params);
//         } else if (command == "JOIN") {
//             joinChannel(client, params);
//         } else if (command == "LIST") {
//             listChannels(client->getFd());
//         } else if (command == "PRIVMSG") {
//             handlePRIVMSG(client, params);
//         } else if (command == "QUIT") {
//             removeClient(client->getFd());
//         } else {
//             sendMessage(client->getFd(), "ERROR :Unknown command\r\n");
//         }
//     }
// }

void Server::registerCommands() {
    // Genel komutlar
    command_map["PASS"] = &Server::handlePASS;
    command_map["NICK"] = &Server::handleNICK;
    command_map["USER"] = &Server::handleUSER;
    command_map["PING"] = &Server::handlePING;
    command_map["CREATE"] = &Server::createChannel;
    command_map["JOIN"] = &Server::joinChannel;
    command_map["LIST"] = &Server::listChannels;
    command_map["PRIVMSG"] = &Server::handlePRIVMSG;
    command_map["QUIT"] = &Server::removeClient;

    // Kanal komutları
    channel_command_map["DELETE"] = &Server::deleteChannel;
    channel_command_map["LEAVE"] = &Server::leaveChannel;
    // channel_command_map["KICK"] = &Server::kickMemberFromChannel;
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
        std::cout << command << std::endl;

        if (!client->getIsAuthenticated() && command != "PASS") {
            sendMessage(client->getFd(), "You must log in with PASS first\n");
            continue;
        }

        if (client->getNickname().empty() && command != "NICK" && command != "PASS") {
            sendMessage(client->getFd(), "ERROR You must set a nickname first with NICK\r\n");
            continue;
        }

    // Kanal içerisindeki komutlar için.
    for (std::map<std::string, Channel*>::iterator it = channels.begin(); it != channels.end(); ++it) {
        if (it->second->isMember(client)) {
            if (channel_command_map.find(command) != channel_command_map.end()) {
                (this->*channel_command_map[command])(client);
            }
            else {
                it->second->broadcastMessage(message, client);
            }
            return;
        }
    }

    // Server komutları için.
    if (command_map.find(command) != command_map.end()) {
        (this->*command_map[command])(client, params);
    } else {
        sendMessage(client->getFd(), "ERROR :Unknown command\r\n");
    }
    }
}

void Server::listChannels(Client *client, const std::vector<std::string>& params) {
    (void)params;
    if (channels.empty()) {
        sendMessage(client->getFd(), "No available channels.\r\n");
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

    sendMessage(client->getFd(), channel_list);
}

void Server::deleteChannel(Client *client) {
    bool found = false;

    for (std::map<std::string, Channel*>::iterator it = channels.begin(); it != channels.end(); ++it) {
        Channel* channel = it->second;

        if (channel->isMember(client)) {
            channel->deleteChannel(client);
            found = true;
            break;
        }
    }

    if (!found) {
        sendMessage(client->getFd(), "ERROR :You are not in any channel\r\n");
    }
}

void Server::leaveChannel(Client* client) {
    bool found = false;

    for (std::map<std::string, Channel*>::iterator it = channels.begin(); it != channels.end(); ++it) {
        Channel* channel = it->second;

        if (channel->isMember(client)) {
            channel->leaveChannel(client);
            found = true;
            break;
        }
    }

    if (!found) {
        sendMessage(client->getFd(), "ERROR :You are not in any channel\r\n");
    }
}

void Server::createChannel(Client *client, const std::vector<std::string>& params) {
    if (params.size() < 2) {
        sendMessage(client->getFd(), "Error Usage: CREATE <channel name> <password>\r\n");
        return;
    }

    std::string name = params[0];
    std::string pass = params[1];

    if (channels.find(name) == channels.end()) {
        channels[name] = new Channel(name, this);
        channels[name]->setPassword(pass);
        channels[name]->addOp(client);
        std::cout << GREEN_COLOR << "Channel created: " << name << " password is " << pass <<  RESET_COLOR << std::endl;
    }
}

void Server::joinChannel(Client *client, const std::vector<std::string>& params) {
    if (params.size() < 2) {
        sendMessage(client->getFd(), "Error Usage: JOIN <channel name> <password>\r\n");
        return;
    }

    std::string name = params[0];
    std::string pass = params[1];

    if (channels.find(name) == channels.end())
        sendMessage(client->getFd(), "ERROR :Channel does not exist\r\n");
    else {
        if (channels[name]->getPassword() == pass) {
            channels[name]->addMember(client);
            std::cout << GREEN_COLOR << client->getNickname() << " joined channel: " << name << RESET_COLOR << std::endl;
        }
        else
            sendMessage(client->getFd(), "ERROR :Invalid password\r\n");
    }
}

void Server::handlePASS(Client* client, const std::vector<std::string>& params) {
    if (params.empty()) {
        sendMessage(client->getFd(), "ERROR :No password given\r\n");
        return;
    }

    if (params[0] == password) {
        client->setIsAuthenticated(true);
        sendMessage(client->getFd(), "Auth Password accepted\r\n");
    } else {
        sendMessage(client->getFd(), "ERROR :Invalid password\r\n");
    }
}

void Server::handleNICK(Client* client, const std::vector<std::string>& params) {
    if (params.empty()) {
        sendMessage(client->getFd(), "ERROR :No nickname given\r\n");
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
    sendMessage(client->getFd(), "NOTICE Nick :*** Nickname set\r\n");
}

void Server::handleUSER(Client* client, const std::vector<std::string>& params) {
    if (params.size() < 4) {
        sendMessage(client->getFd(), "ERROR :Not enough parameters\r\n");
        return;
    }

    client->setUsername(params[0]);
    sendMessage(client->getFd(), "NOTICE User :*** User registered\r\n");
}

void Server::handlePING(Client* client, const std::vector<std::string>& params) {
    if (params.empty()) {
        sendMessage(client->getFd(), "ERROR :No PING token given\r\n");
        return;
    }

    std::string response = "PONG :" + params[0] + "\r\n";
    sendMessage(client->getFd(), response);
}

void Server::handlePRIVMSG(Client* client, const std::vector<std::string>& params) {
    if (params.size() < 2) {
        sendMessage(client->getFd(), "ERROR :Not enough parameters\r\n");
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
        std::string full_message = client->getNickname() + " PRIVMSG " + "TO " + recipient + ": " + message + "\r\n";
        sendMessage(targetClient->getFd(), full_message);
    } else {
        sendMessage(client->getFd(), "ERROR :No such nick\r\n");
    }
}
#include "Server.hpp"

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: ./ircserv <port> <password>" << std::endl;
        return EXIT_FAILURE;
    }

    std::string port = argv[1];
    std::string password = argv[2];

    if (!Server::isNumber(port))
    {
        std::cerr << RED_COLOR << "Invalid port number" << RESET_COLOR << std::endl;
        std::exit(EXIT_FAILURE);
    } 
    double port_num = std::atof(port.c_str());
    if (port_num < 0 || port_num > PORT_LIMIT)
    {
        std::cerr << RED_COLOR << "Range of port limit outside!" << RESET_COLOR << std::endl;
        std::exit(EXIT_FAILURE);
    }

    Server server(port, password);
    server.run();

    return EXIT_SUCCESS;
}

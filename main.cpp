#include "Server.hpp"

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: ./ircserv <port> <password>" << std::endl;
        return EXIT_FAILURE;
    }

    std::string port = argv[1];
    std::string password = argv[2];

    Server server(port, password);
    server.run();

    return EXIT_SUCCESS;
}

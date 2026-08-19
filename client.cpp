#include "client.h"

// Helper to send a request (E.g., CHECK|username or TX|sender|receiver|amount) to the main server via TCP and receive the response
static std::string request_via_tcp(const std::string &request) {
    // Create a TCP client socket with fixed loopback IP address (127.0.0.1) and dynamic port number
    int sockfd = open_tcp_client_socket();

    // Construct a socket address structure of the main server
    sockaddr_in servaddr{};
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(TXCHAIN_HOST);
    servaddr.sin_port = htons(CLIENT_MAIN_TCP_PORT);

    // Establish a TCP connection to the main server and exit if there is an error
    if (connect(sockfd, reinterpret_cast<sockaddr*>(&servaddr), sizeof(servaddr)) < 0) {
        perror("connect");
        close(sockfd);
        return "";
    }

    // Send the request
    send_all_tcp(sockfd, request);

    // Disable sending on client socket to signal to main server that it is finished sending the request.
    shutdown(sockfd, SHUT_WR);

    // Wait and receive the response from the server
    std::string resp = recv_all_tcp(sockfd);

    // Close the socket after receiving the response
    close(sockfd);

    return resp;
}

// Takes either a CHECK request (for balance enquiry) or a TX request (for transferring txcoins) and sends it to the main server via TCP. Returns the response from the main server.
int main(int argc, char *argv[]) {
    std::cout << "The client is up and running." << std::endl;

    if (argc == 2) {
        // Construct the request string for balance checks
        std::string username = argv[1];
        std::string req = std::string("CHECK|") + username;
        std::cout << "\"" << username << "\" sent a balance enquiry request to the main server." << std::endl;

        // Send request string to main server
        std::string resp = request_via_tcp(req);
        std::cout << resp;
    } else if (argc == 4) {
        // Construct the request string for transfer requests
        std::string sender = argv[1];
        std::string receiver = argv[2];
        std::string amount = argv[3];
        std::string req = std::string("TX|") + sender + "|" + receiver + "|" + amount;
        std::cout << "\"" << sender << "\" has requested to transfer " << amount << " txcoins to \"" << receiver << "\"." << std::endl;

        // Send the request to main server
        std::string resp = request_via_tcp(req);
        std::cout << resp;
    }
    return 0;
}

#include "client.h"

// Helper to send a request to the main server via TCP and receive the response
static std::string request_via_tcp(const std::string &request) {
    int sockfd = open_tcp_client_socket();

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

    // Send the request, shut down the writing side of the socket, receive the response, and close the socket
    send_all_tcp(sockfd, request);
    shutdown(sockfd, SHUT_WR);
    std::string resp = recv_all_tcp(sockfd);
    close(sockfd);
    return resp;
}

// Prints responses to the console based on the type of request made by the user
int main(int argc, char *argv[]) {
    std::cout << "The client is up and running." << std::endl;

    // Handle CHECK requests for balance enquiry and TX requests for transferring txcoins
    if (argc == 2) {
        std::string username = argv[1];
        std::string req = std::string("CHECK|") + username;
        std::cout << "\"" << username << "\" sent a balance enquiry request to the main server." << std::endl;
        std::string resp = request_via_tcp(req);
        std::cout << resp;
    } else if (argc == 4) {
        std::string sender = argv[1];
        std::string receiver = argv[2];
        std::string amount = argv[3];
        std::string req = std::string("TX|") + sender + "|" + receiver + "|" + amount;
        std::cout << "\"" << sender << "\" has requested to transfer " << amount << " txcoins to \"" << receiver << "\"." << std::endl;
        std::string resp = request_via_tcp(req);
        std::cout << resp;
    }
    return 0;
}

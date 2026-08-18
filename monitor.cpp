#include "monitor.h"

// Sends TCP requests to the main server and prints the responses to the console
static std::string request_via_tcp(const std::string &request) {
    int sockfd = open_tcp_client_socket();

    sockaddr_in servaddr{};
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(TXCHAIN_HOST);
    servaddr.sin_port = htons(MONITOR_MAIN_TCP_PORT);

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
    std::cout << "The monitor is up and running." << std::endl;

    if (argc == 2 && std::string(argv[1]) == "TXLIST") {
        std::cout << "Monitor sent a sorted list request to the main server." << std::endl;
        std::string resp = request_via_tcp("TXLIST");
        (void)resp;
        std::cout << "Successfully received a sorted list of transactions from the main server." << std::endl;
    }
    return 0;
}

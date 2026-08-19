#include "monitor.h"

// Sends TCP requests to the main server and prints the responses to the console
static std::string request_via_tcp(const std::string &request) {
    // Create a TCP client socket with fixed loopback IP address (127.0.0.1) and dynamic port number
    int sockfd = open_tcp_client_socket();

    // Construct a socket address structure of the main server
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

// Takes only one request (TXLIST), which generates a sorted list of all transactions from the backend
int main(int argc, char *argv[]) {
    std::cout << "The monitor is up and running." << std::endl;

    if (argc == 2 && std::string(argv[1]) == "TXLIST") {
        std::cout << "Monitor sent a sorted list request to the main server." << std::endl;

        // Send TXLIST request string to mains server
        std::string resp = request_via_tcp("TXLIST");
        (void)resp;
        std::cout << "Successfully received a sorted list of transactions from the main server." << std::endl;
    }
    return 0;
}

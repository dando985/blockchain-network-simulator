#include "serverC.h"


static const int BACKEND_PORT = SERVERC_UDP_PORT;
static const char *BLOCK_FILE = SERVERC_BLOCK_FILE;

// Reads the entire block file and returns its content as a string
static std::string read_block_file() {
    std::ifstream ifs(BLOCK_FILE);
    if (!ifs.is_open()) {
        std::ofstream create(BLOCK_FILE, std::ios::app);
        create.close();
        return "";
    }
    std::ostringstream oss;
    oss << ifs.rdbuf();

    return oss.str();
}

// Appends a new line to the block file when a new transaction is added
static void append_block_line(const std::string &line) {
    std::ofstream ofs(BLOCK_FILE, std::ios::app);
    ofs << line;
    if (line.empty() || line.back() != '\n') {
        ofs << '\n';
    }
}

// Main function to set up the UDP server and handle incoming requests from the main server
int main() {
    // Initialize the UDP socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) { 
        perror("socket"); 
        exit(1); 
    }

    // Construct server C's IP address structure
    sockaddr_in servaddr{};
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(TXCHAIN_HOST);
    servaddr.sin_port = htons(BACKEND_PORT);

    // Bind socket and exit if there is an error
    if (bind(sockfd, reinterpret_cast<sockaddr*>(&servaddr), sizeof(servaddr)) < 0) {
        perror("bind");
        exit(1);
    }

    std::cout << "The ServerC is up and running using UDP on port " << BACKEND_PORT << "." << std::endl;

    while (true) {
        // Create receive buffer for incomming UDP datagram
        char buf[4096];

        // Store information about the sender to know where to send the response back to
        sockaddr_in cliaddr{};
        socklen_t len = sizeof(cliaddr);

        // Receive UDP data and store the sender's address
        ssize_t n = recvfrom(sockfd, buf, sizeof(buf), 0, reinterpret_cast<sockaddr*>(&cliaddr), &len);

        // Error handling
        if (n < 0) {
            if (errno == EINTR) continue;
            perror("recvfrom");
            continue;
        }

        // Convert the byte data into the request string
        std::string req(buf, buf + n);
        std::cout << "The ServerC received a request from the Main Server." << std::endl;

        // Handle GET_ALL and APPEND requests, and send appropriate responses back to the main server
        if (req == "GET_ALL") {
            std::string payload = read_block_file();
            udp_send_all_chunks(sockfd, cliaddr, "ALL", payload);
        } else if (req.rfind("APPEND|", 0) == 0) {
            std::string line = req.substr(7);
            append_block_line(line);
            udp_send_all_chunks(sockfd, cliaddr, "ACK", "OK");
        } else {
            udp_send_all_chunks(sockfd, cliaddr, "ACK", "ERR");
        }

        std::cout << "The ServerC finished sending the response to the Main Server." << std::endl;
    }

    close(sockfd);

    return 0;
}

#include "serverM.h"

// Create shorter local names for the TCP and UDP ports
static const int UDP_PORT = SERVERM_UDP_PORT;
static const int CLIENT_TCP_PORT = SERVERM_CLIENT_TCP_PORT;
static const int MONITOR_TCP_PORT = SERVERM_MONITOR_TCP_PORT;
static const int PORT_A = SERVERM_BACKEND_A_PORT;
static const int PORT_B = SERVERM_BACKEND_B_PORT;
static const int PORT_C = SERVERM_BACKEND_C_PORT;

// Helper to label backend servers by their letter based on their port number
static std::string backend_label(int port) {
    if (port == PORT_A) {
        return "A";
    }
    else if (port == PORT_B) {
        return "B";
    }
    else {
        return "C";
    }
}

// Helper to return backend server name based on their port number
static std::string backend_full_name(int port) {
    if (port == PORT_A) {
        return "Server A";
    }
    else if (port == PORT_B) {
        return "Server B";
    }
    else {
        return "Server C";
    }
}

// Helper to construct a sockaddr_in structure for UDP communication with backend server
static sockaddr_in make_udp_addr(int port) {
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(TXCHAIN_HOST);
    addr.sin_port = htons(port);

    return addr;
}

// Fetches transaction records from a backend server. Returns a vector of Tx records.
static std::vector<TxRecord> fetch_all_records_from_backend(int udp_sock, int port, bool log_messages) {
    if (log_messages) {
        std::cout << "The main server sent a request to server " << backend_label(port) << "." << std::endl;
    }

    // Send "GET_ALL" request to destination backend server and get its response
    std::string raw = udp_request_response(udp_sock, make_udp_addr(port), "GET_ALL");

    if (log_messages) {
        std::cout << "The main server received transactions from " << backend_full_name(port)
                  << " using UDP over port " << port << "." << std::endl;
    }

    return parse_records_text(raw);
}

// Collects transaction records from all backend servers
static std::vector<TxRecord> collect_all_records(int udp_sock, bool log_messages) {
    std::vector<TxRecord> all;
    for (int port : {PORT_A, PORT_B, PORT_C}) {
        auto part = fetch_all_records_from_backend(udp_sock, port, log_messages);

        // Aggregate backend server's response into one vector
        all.insert(all.end(), part.begin(), part.end());
    }

    return all;
}

// Appends a new transaction entry to a backend server. Returns true if the request was sucessful, false otherwise.
static bool append_transaction_to_backend(int udp_sock, int port, const std::string &encrypted_entry, bool log_messages) {
    if (log_messages) {
        std::cout << "The main server sent a request to server " << backend_label(port) << "." << std::endl;
    }

    // Sends "APPEND + {encrypted line}" to destination backend server and gets its response
    std::string resp = udp_request_response(udp_sock, make_udp_addr(port), std::string("APPEND|") + encrypted_entry);

    if (log_messages) {
        std::cout << "The main server received the feedback from server " << backend_label(port)
                  << " using UDP over port " << port << "." << std::endl;
    }

    // Check that the backend server's response is "OK"
    return resp.find("OK") != std::string::npos;
}


// Handles a CHECK request by computing the balance for the specified username across all backend TX records
static std::string handle_check(int udp_sock, const std::string &username) {
    auto all = collect_all_records(udp_sock, true);

    return build_response_for_check(all, username);
}

// Handles a TX request by validating the transaction details and computing the resulting balance for the sender after the transaction
static std::string handle_tx(int udp_sock, const std::string &sender, const std::string &receiver, const std::string &amount_text) {
    auto all = collect_all_records(udp_sock, false);
    bool success = false;
    double sender_after = 0.0;
    std::string reason;
    // Validate transaction details and confirm whether the transaction can be made successfully
    std::string response = build_response_for_tx(all, sender, receiver, amount_text, success, sender_after, reason);

    // Return failure message if transaction request fails
    if (!success) {
        return response;
    }

    // If the transaction is valid, create a new TxRecord
    int serial = max_serial(all) + 1;
    TxRecord rec;
    rec.serial = serial;
    rec.sender = sender;
    rec.receiver = receiver;
    rec.amount = amount_text;

    // Randomly select a backend server to append the new transaction entry
    std::vector<int> ports = {PORT_A, PORT_B, PORT_C};
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(0, 2);
    int target_port = ports[dist(gen)];

    // Encrypt the transaction details and append to the selected backend server
    std::ostringstream line;
    line << rec.serial << ' ' << encrypt_text(rec.sender) << ' ' << encrypt_text(rec.receiver) << ' ' << encrypt_text(rec.amount);
    append_transaction_to_backend(udp_sock, target_port, line.str(), true);

    return response;
}

// Saves the sorted list of transactions to a local file
static std::string handle_txlist(int udp_sock) {
    // Collect all records
    auto all = collect_all_records(udp_sock, false);

    // Sort the records by ascending serial number
    std::sort(all.begin(), all.end(), [](const TxRecord &a, const TxRecord &b) {
        return a.serial < b.serial;
    });

    // Store the decrypted records in "txchain.txt"
    save_records_to_file("txchain.txt", all, false);
    
    return "Successfully received a sorted list of transactions from the main server.";
}

// Accepts incoming TCP connection and returns the connected socket and the peer port number
static std::pair<int, int> accept_one(int listenfd) {
    // Construct client address structure
    sockaddr_in peer{};
    socklen_t len = sizeof(peer);

    // Create new server socket specifically for accepted TCP client
    int connfd = accept(listenfd, reinterpret_cast<sockaddr*>(&peer), &len);
    if (connfd < 0) {
        return {-1, -1};
    }
    return {connfd, ntohs(peer.sin_port)};
}

// Main function to set up the main server, listen for incoming connections from clients and monitor, and handle requests using select
int main() {
    // Initialize main server UDP socket
    int udp_sock = socket(AF_INET, SOCK_DGRAM, 0);

    // Exit if socket creation fails
    if (udp_sock < 0) {
        perror("socket"); 
        exit(1); 
    }

    // Build main server's UDP address
    sockaddr_in udp_addr{};
    udp_addr.sin_family = AF_INET;
    udp_addr.sin_addr.s_addr = inet_addr(TXCHAIN_HOST);
    udp_addr.sin_port = htons(UDP_PORT);

    // Bind socket and exit if there is an error
    if (bind(udp_sock, reinterpret_cast<sockaddr*>(&udp_addr), sizeof(udp_addr)) < 0) {
        perror("bind");
        exit(1);
    }

    // Create TCP server listening sockets for clients and monitor
    int client_listen = open_tcp_server_socket(CLIENT_TCP_PORT);
    int monitor_listen = open_tcp_server_socket(MONITOR_TCP_PORT);

    std::cout << "The main server is up and running." << std::endl;

    // Listen for incoming connections from clients and monitor
    while (true) {

        // Create file descriptor set to listen to both client and monitor sockets
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(client_listen, &readfds);
        FD_SET(monitor_listen, &readfds);
        int maxfd = std::max(client_listen, monitor_listen) + 1;
        // Monitor file descriptor set indefintely until one of the sockets becomes ready
        int rv = select(maxfd, &readfds, nullptr, nullptr, nullptr);

        if (rv < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("select");
            break;
        }

        // Check if client socket is ready
        if (FD_ISSET(client_listen, &readfds)) {

            // Accept incomming client connection
            std::pair<int, int> client_conn = accept_one(client_listen);
            // Create variable for client's file descriptor
            int connfd = client_conn.first;
            // Create variable for client's port
            int peer_port = client_conn.second;

            // Exit if there is an error accepting the connection
            if (connfd < 0) {
                continue;
            }

            // Handle CHECK requests for balance enquiry and TX requests for transferring txcoins
            std::string req = recv_all_tcp(connfd);
            // Process the request to get balance or transfer txcoins
            if (req.rfind("CHECK|", 0) == 0) {
                // Extract username from request string
                std::string username = req.substr(6);
                std::cout << "The main server received input=\"" << username
                          << "\" from the client using TCP over port " << peer_port << "." << std::endl;
                
                // Create string that checks for corresponding username's balance from backend servers
                std::string resp = handle_check(udp_sock, username);

                // Send response back to client
                send_all_tcp(connfd, resp);
                std::cout << "The main server sent the current balance to the client." << std::endl;
            } else if (req.rfind("TX|", 0) == 0) {

                // Find position of second separator (|)
                size_t p1 = req.find('|', 3);
                // Find position of third separator (|)
                size_t p2 = req.find('|', p1 == std::string::npos ? p1 : p1 + 1);

                if (p1 != std::string::npos && p2 != std::string::npos) {
                    // Extract sender, receiver, and send amount from string
                    std::string sender = req.substr(3, p1 - 3);
                    std::string receiver = req.substr(p1 + 1, p2 - p1 - 1);
                    std::string amount = req.substr(p2 + 1);
                    std::cout << "The main server received from \"" << sender << "\" to transfer " << amount
                              << " coins to \"" << receiver << "\" using TCP over port " << peer_port << "." << std::endl;

                    // Create a string that checks usernames and sender balance from backend servers, and appends new transaction line to backend server
                    std::string resp = handle_tx(udp_sock, sender, receiver, amount);

                    // Send response back to client
                    send_all_tcp(connfd, resp);
                    std::cout << "The main server sent the result of the transaction to the client." << std::endl;
                }
            }
            close(connfd);
        }

        // Check if monitor socket is ready
        if (FD_ISSET(monitor_listen, &readfds)) {
            // Accept incomming monitor connection
            std::pair<int, int> monitor_conn = accept_one(monitor_listen);

            // Create variable to store monitor's file descriptor
            int connfd = monitor_conn.first;
            // Create variable to store monitor's port number
            int peer_port = monitor_conn.second;

            // Exit if there is an error accepting the connection
            if (connfd < 0) {
                continue;
            }

            // Handle TXLIST request for sorted list of transactions
            std::string req = recv_all_tcp(connfd);
            if (req == "TXLIST") {
                std::cout << "The main server received a sorted list request from the monitor using TCP over port "
                          << peer_port << "." << std::endl;

                // Create response string after generating TX record file from backend servers
                std::string resp = handle_txlist(udp_sock);
                // Send response bnack to monitor
                send_all_tcp(connfd, resp);
            }
            close(connfd);
        }
    }

    // Close all sockets after main server stop listening
    close(client_listen);
    close(monitor_listen);
    close(udp_sock);

    return 0;
}
#ifndef COMMON_H
#define COMMON_H

// Netowrking Headers
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>

// C++ Standard Library Headers
#include <algorithm>
#include <cmath>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <vector>

// Hardcoded IP address used by every node in the network for local testing. In a real-world scenario, this would be replaced with a proper DNS or IP address.
static const char *TXCHAIN_HOST = "127.0.0.1";

// Structure to represent a transaction record
struct TxRecord {
    int serial = 0;
    std::string sender;
    std::string receiver;
    std::string amount;
};

// -------------------------------- Encryption and Decryption Utilities --------------------------------

// Trim whitespace from both ends of a string
inline std::string trim(const std::string &s) {
    // Find the first non-whitespace character
    size_t b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) {
        return "";
    }

    // Find the last non-whitespace character
    size_t e = s.find_last_not_of(" \t\r\n");

    // Return the substring between the first and last non-whitespace characters
    return s.substr(b, e - b + 1);
}

// Encryption shift logic for alphanumeric characters
inline char shift_char(char c, int shift) {

    // Shift lowercase letters
    if (c >= 'a' && c <= 'z') {
        int base = 'a';
        int x = (c - base + shift) % 26;
        if (x < 0) x += 26;
        return static_cast<char>(base + x);
    }

    // Shift uppercase letters
    if (c >= 'A' && c <= 'Z') {
        int base = 'A';
        int x = (c - base + shift) % 26;
        if (x < 0) x += 26;
        return static_cast<char>(base + x);
    }

    // Shift digits
    if (c >= '0' && c <= '9') {
        int base = '0';
        int x = (c - base + shift) % 10;
        if (x < 0) x += 10;
        return static_cast<char>(base + x);
    }
    
    // If the character is not alphanumeric, return it unchanged
    return c;
}

// Helper to perform per-letter shift in a string for encryption and decryption
inline std::string letter_shift(const std::string &text, int shift) {
    std::string out;

    // Reserve space in the output string to avoid multiple reallocations (better optimization)
    out.reserve(text.size());

    // Iterate through each character in the input text and apply the shift
    for (char c : text) {
        out.push_back(shift_char(c, shift));
    }

    return out;
}

// Encryption wrapper function that shifts letters by +3
inline std::string encrypt_text(const std::string &text) {
    return letter_shift(text, 3);
}

// Decryption wrapper function that shifts letters by -3
inline std::string decrypt_text(const std::string &text) {
    return letter_shift(text, -3);
}

// -------------------------------- Transaction Record Utilities --------------------------------

// Converts a double value to a string representation, removing unnecessary trailing zeros and decimal points. Needed to have the amount as a string in the block files
inline std::string format_amount(double value) {

    // Get nearest integer value
    double rounded = std::round(value);
    // If the rounded value is very close to the original value, return the integer representation as a string
    if (std::fabs(value - rounded) < 1e-9) {
        long long n = static_cast<long long>(rounded);
        return std::to_string(n);
    }

    // Otherwise, format the value with a fixed precision of 10 decimal places and remove trailing zeros and decimal points
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(10) << value;
    std::string s = oss.str();

    // Remove unnecessary trailing zeros until the last character is not a zero or until the last character is a decimal point
    while (!s.empty() && s.find('.') != std::string::npos && s.back() == '0') {
        s.pop_back();
    }

    // If the last character is a decimal point, remove it as well
    if (!s.empty() && s.back() == '.') {
        s.pop_back();
    }

    return s;
}

// Attempts to convert amount string to double. Stores the result in the output parameter and returns true if successful, false otherwise. This is used to validate the amount in transactions.
inline bool parse_amount(const std::string &s, double &out) {
    try {
        size_t idx = 0;
        // Convert string to double using std::stod, which throws an exception if the conversion fails
        out = std::stod(s, &idx);
        return idx == s.size();
    } catch (...) {
        return false;
    }
}

// Convert block file record line to TxRecord structure. Input line must follow the format: serial sender receiver amount. Stores the record in the output parameter and returns true if successful, false otherwise.
inline bool parse_record_line(const std::string &line, TxRecord &rec) {
    // Treat the line as an input stream and extract the fields into the TxRecord structure
    std::istringstream iss(line);

    // Attempt to read the serial, sender, receiver, and amount fields from the line. If any of these fail, return false. Note that decryption does not happen here; it is handled in the parse_records_text function after this function returns successfully.
    if (!(iss >> rec.serial >> rec.sender >> rec.receiver >> rec.amount)) {
        return false;
    }

    return true;
}

// Helper function to read all transaction records from a block file, decrypting them and returning a vector of TxRecord structures
inline std::vector<TxRecord> parse_records_text(const std::string &text) {
    std::vector<TxRecord> records;
    std::istringstream iss(text);
    std::string line;

    // Split the text into lines, trim whitespace, and parse each line into a TxRecord structure
    while (std::getline(iss, line)) {
        line = trim(line);
        if (line.empty()) continue;

        // Parse the line into a TxRecord structure and store in enc. If parsing fails, skip the line.
        TxRecord enc;
        if (!parse_record_line(line, enc)) continue;

        // Decrypt the fields of the TxRecord structure and add it to the records vector
        TxRecord rec;
        rec.serial = enc.serial;
        rec.sender = decrypt_text(enc.sender);
        rec.receiver = decrypt_text(enc.receiver);
        rec.amount = decrypt_text(enc.amount);

        // Add the decrypted TxRecord structure to the records vector
        records.push_back(rec);
    }

    return records;
}

// Convert TxRecord structure to a record string for block file storage. Note that the record string is not encrypted here; encryption is handled in the records_to_text function when saving records to a block file.
inline std::string record_to_line(const TxRecord &rec) {
    std::ostringstream oss;

    // Format the TxRecord fields into a single line string, separating them with spaces. This is used when saving records to a block file.
    oss << rec.serial << ' ' << rec.sender << ' ' << rec.receiver << ' ' << rec.amount;

    return oss.str();
}


// Converts a vector of TxRecord structures into one single text block. Optionally able to encrypt the fields of the TxRecord structures before converting them to text. This is used when saving records to a block file.
inline std::string records_to_text(const std::vector<TxRecord> &records, bool encrypt_fields) {
    std::ostringstream oss;

    for (size_t i = 0; i < records.size(); ++i) {
        // Create a temporary alias for the current TxRecord structure to make the code more readable
        const auto &r = records[i];

        // Format the TxRecord fields into a single line string, optionally encrypting the sender, receiver, and amount fields.
        oss << r.serial << ' '
            << (encrypt_fields ? encrypt_text(r.sender) : r.sender) << ' '
            << (encrypt_fields ? encrypt_text(r.receiver) : r.receiver) << ' '
            << (encrypt_fields ? encrypt_text(r.amount) : r.amount);

        // Add a newline character after each record, except for the last one, to ensure proper formatting in the output text block.
        if (i + 1 < records.size()) oss << '\n';
    }

    return oss.str();
}

// Reads the entire content of a file and returns it as a string
inline std::string load_file_text(const std::string &path) {
    std::ifstream ifs(path);

    // If the file cannot be opened, return an empty string
    if (!ifs.is_open()) {
        return "";
    }

    // Pass the file stream buffer to a string stream instead of reading line=by-line.
    std::ostringstream oss;
    oss << ifs.rdbuf();

    return oss.str();
}

// Wrapper to load transaction records from a block file. This function reads the entire content of the file, parses it into TxRecord structures, and returns a vector of these structures.
inline std::vector<TxRecord> load_records_from_file(const std::string &path) {
    return parse_records_text(load_file_text(path));
}

// Saves a vector of TxRecord structures to a block file with option to encrypt when new transactions are added
inline void save_records_to_file(const std::string &path, const std::vector<TxRecord> &records, bool encrypt_fields) {

    // Open the file in truncation mode to overwrite any existing content.
    std::ofstream ofs(path, std::ios::trunc);

    for (size_t i = 0; i < records.size(); ++i) {
        // Create a temporary alias for the current TxRecord structure to make the code more readable
        const auto &r = records[i];
        
        // Write the formatted TxRecord fields to the file, optionally encrypting the sender, receiver, and amount fields.
        ofs << r.serial << ' '
            << (encrypt_fields ? encrypt_text(r.sender) : r.sender) << ' '
            << (encrypt_fields ? encrypt_text(r.receiver) : r.receiver) << ' '
            << (encrypt_fields ? encrypt_text(r.amount) : r.amount);
        ofs << '\n';
    }
}

// -------------------------------- Computation Utilities --------------------------------

// Creates a unique set of all users
inline std::set<std::string> participant_set(const std::vector<TxRecord> &records) {
    std::set<std::string> out;
    for (const auto &r : records) {
        out.insert(r.sender);
        out.insert(r.receiver);
    }

    return out;
}

// Calculates the balance of a user by iterating through all transaction records
inline double compute_balance(const std::vector<TxRecord> &records, const std::string &username) {
    // Starting balance for all users
    double balance = 1000.0;

    // Iterate through all transaction records and update the balance based on sent and received amounts
    for (const auto &r : records) {
        double amount = 0.0;
        // If the amount cannot be parsed, skip this record and continue to the next one
        if (!parse_amount(r.amount, amount)) continue;

        // Subtract the amount from the balance if the user is the sender, and add the amount to the balance if the user is the receiver
        if (r.sender == username) balance -= amount;
        if (r.receiver == username) balance += amount;
    }

    return balance;
}

// Finds last serial number among all transaction records
inline int max_serial(const std::vector<TxRecord> &records) {
    int m = 0;
    for (const auto &r : records) {
        m = std::max(m, r.serial);
    }
    return m;
}

// -------------------------------- User Request Handlers --------------------------------

// Handles CHECK request by collecting all transaction records and computing the balance for the requested user
inline std::string build_response_for_check(const std::vector<TxRecord> &all_records, const std::string &username) {

    // Get set of all users in the network
    auto participants = participant_set(all_records);

    // Handle case where the requested user is not part of the network
    if (participants.find(username) == participants.end()) {
        std::ostringstream oss;
        oss << "\"" << username << "\" is not a part of the network.\n";

        return oss.str();
    }

    // Otherwise compute and return the balance for the requested user
    std::ostringstream oss;
    oss << "The current balance of \"" << username << "\" is : " << format_amount(compute_balance(all_records, username)) << " txcoins.\n";

    return oss.str();
}

// Handles TX request by validating the transaction details and computing the resulting balance for the sender after the transaction
inline std::string build_response_for_tx(const std::vector<TxRecord> &all_records,
                                         const std::string &sender,
                                         const std::string &receiver,
                                         const std::string &amount_text,
                                         bool &success,
                                         double &sender_balance_after,
                                         std::string &reason) {
    // Get set of all users in the network
    auto participants = participant_set(all_records);
    // Check if both sender and receiver exist in the network
    bool sender_exists = participants.find(sender) != participants.end();
    bool receiver_exists = participants.find(receiver) != participants.end();

    // Handle case where both users do not exist in the network
    if (!sender_exists && !receiver_exists) {
        success = false;
        reason = "both_missing";
        std::ostringstream oss;
        oss << "Unable to proceed with the transaction as \"" << sender << "\" and \"" << receiver << "\" are not part of the network.\n";
        return oss.str();
    }

    // Handle case where sender does not exist in the network
    if (!sender_exists) {
        success = false;
        reason = "sender_missing";
        std::ostringstream oss;
        oss << "Unable to proceed with the transaction as \"" << sender << "\" is not part of the network.\n";
        return oss.str();
    }

    // Handle case where receiver does not exist in the network
    if (!receiver_exists) {
        success = false;
        reason = "receiver_missing";
        std::ostringstream oss;
        oss << "Unable to proceed with the transaction as \"" << receiver << "\" is not part of the network.\n";
        return oss.str();
    }

    // Parse the amount, validate the sender's balance, and compute the resulting balance after the transaction.
    double amount = 0.0;
    if (!parse_amount(amount_text, amount)) amount = 0.0;
    // Compute sender's balance currently in the network before the transaction
    double sender_balance = compute_balance(all_records, sender);

    // Handle case where sender has insufficient balance for the transaction
    if (sender_balance + 1e-9 < amount) {
        success = false;
        reason = "insufficient";
        std::ostringstream oss;
        oss << "\"" << sender << "\" was unable to transfer " << amount_text << " txcoins to \"" << receiver << "\" because of insufficient balance.\n\n";
        oss << "The current balance of \"" << sender << "\" is : " << format_amount(sender_balance) << " txcoins.\n";

        return oss.str();
    }

    // Handle case where the transaction is valid and successful
    sender_balance_after = sender_balance - amount;
    success = true;
    reason = "success";
    std::ostringstream oss;
    oss << "\"" << sender << "\" successfully transferred " << amount_text << " txcoins to \"" << receiver << "\".\n\n";
    oss << "The current balance of \"" << sender << "\" is : " << format_amount(sender_balance_after) << " txcoins.\n";
    return oss.str();
}

// -------------------------------- TCP Networking Utilities --------------------------------

// Creates a TCP server socket, binds it to the specified port, and starts listening for incoming connections
inline int open_tcp_server_socket(int port) {
    // Create a TCP socket file descriptor using IPv4. Value will be a positive integer if successful, or -1 if there is an error.
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    // Exit if socket creation fails
    if (sockfd < 0) {
        perror("socket");
        exit(1);
    }

    // Allow reuse of the address to avoid any errors when restarting the server
    int yes = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    // Create a socket address structure and initialize it with the specified port and the hardcoded IP address. This structure will be used to bind the socket to the desired address and port.
    sockaddr_in servaddr{};
    servaddr.sin_family = AF_INET;
    // Convert the IP address from string format to binary format and assign it to the socket address structure
    servaddr.sin_addr.s_addr = inet_addr(TXCHAIN_HOST);
    // Convert the port number from host byte order to network byte order and assign it to the socket address structure
    servaddr.sin_port = htons(port);

    // Associates socket with the specified address and port number.
    if (bind(sockfd, reinterpret_cast<sockaddr*>(&servaddr), sizeof(servaddr)) < 0) {
        perror("bind");
        exit(1);
    }

    // Start listening for incoming connections with a backlog of 10 connections.
    if (listen(sockfd, 10) < 0) {
        perror("listen");
        exit(1);
    }

    return sockfd;
}

// Creates a TCP client socket and binds it to a dynamic port
inline int open_tcp_client_socket() {
    // Create a TCP socket file descriptor using IPv4. Value will be a positive integer if successful, or -1 if there is an error.
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    // Exit if socket creation fails
    if (sockfd < 0) {
        perror("socket");
        exit(1);
    }

    // Allow reuse of the address to avoid any errors when restarting the client
    int yes = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    // Create a socket address structure and initialize it with the hardcoded IP address and a dynamic port (0).
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    // Convert the IP address from string format to binary format and assign it to the socket address structure
    addr.sin_addr.s_addr = inet_addr(TXCHAIN_HOST);
    // Let operating system assign a dynamic port number by specifying 0. The actual port number can be retrieved later using getsockname().
    addr.sin_port = htons(0);

    // Bind the socket to the specified address and dynamic port number. Needed because we want to use a fixed loopback IP address. Bind is also used to later retrieve the dynamic port number assigned to the socket.
    if (bind(sockfd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        perror("bind");
        exit(1);
    }

    return sockfd;
}

// Returns the dynamic port number assigned to a TCP client socket
inline int get_local_tcp_port(int sockfd) {
    sockaddr_in addr{};
    socklen_t len = sizeof(addr);

    // Get the local address and port number assigned to the socket and exit if there is an error
    if (getsockname(sockfd, reinterpret_cast<sockaddr*>(&addr), &len) < 0) {
        return -1;
    }

    // Convert the port number from network byte order to host byte order and return it
    return ntohs(addr.sin_port);
}

// Keeps receiving data from a TCP socket until the connection is closed
inline std::string recv_all_tcp(int sockfd) {
    std::string out;
    char buf[4096];
    while (true) {
        ssize_t n = recv(sockfd, buf, sizeof(buf), 0);
        if (n < 0) {
            // If the recv call was interrupted by a signal, continue receiving data. Otherwise, exit the loop and return the data received so far.
            if (errno == EINTR) continue;
            break;
        }
        // If the connection is closed (recv returns 0), exit the loop and return the data received so far.
        if (n == 0) {
            break;
        }
        // Append the received data to the output string. The data is appended from the buffer starting at the beginning and extending for n bytes.
        out.append(buf, buf + n);
    }

    return out;
}

// Ensures that all data is sent over a TCP socket
inline void send_all_tcp(int sockfd, const std::string &data) {
    size_t sent = 0;
    while (sent < data.size()) {
        ssize_t n = send(sockfd, data.data() + sent, data.size() - sent, 0);
        if (n < 0) {
            if (errno == EINTR) continue;
            return;
        }
        sent += static_cast<size_t>(n);
    }
}

// Takes large text and splits it into lines, trimming whitespace and ignoring empty lines. This is used to process the content of block files and other text data.
inline std::vector<std::string> split_lines(const std::string &text) {
    std::vector<std::string> lines;
    std::istringstream iss(text);
    // Temporary variable to hold each line read from the input string stream
    std::string line;

    while (std::getline(iss, line)) {
        line = trim(line);
        if (!line.empty()) {
            lines.push_back(line);
        }
    }

    return lines;
}

// -------------------------------- UDP Networking Utilities --------------------------------

// Helper to create a mini packet with format: type|seq|total|payload. This is used to send large payloads in smaller chunks over UDP.
inline std::string chunk_packet(const std::string &type, int seq, int total, const std::string &payload) {
    std::ostringstream oss;
    oss << type << '|' << seq << '|' << total << '|' << payload;

    return oss.str();
}

// Parses a chunk packet into its components: type, sequence number, total chunks, and payload
inline bool parse_chunk_packet(const std::string &packet, std::string &type, int &seq, int &total, std::string &payload) {

    // Search for the 3 pipe (|) characters that separate the components of the packet. If any of them are not found, return false to indicate a parsing error.
    size_t p1 = packet.find('|');
    if (p1 == std::string::npos) {
        return false;
    }

    size_t p2 = packet.find('|', p1 + 1);
    if (p2 == std::string::npos) {
        return false;
    }

    size_t p3 = packet.find('|', p2 + 1);
    if (p3 == std::string::npos) {
        return false;
    }

    // Extract "type", "seq", "total", and "payload" from the packet string using the positions of the pipe characters.
    type = packet.substr(0, p1);
    try {
        seq = std::stoi(packet.substr(p1 + 1, p2 - p1 - 1));
        total = std::stoi(packet.substr(p2 + 1, p3 - p2 - 1));
    } catch (...) {
        return false;
    }
    payload = packet.substr(p3 + 1);

    return true;
}

// Sends a request via UDP and waits for the response
inline std::string udp_request_response(int sockfd, const sockaddr_in &dest, const std::string &request) {

    // Send the request packet to the destination and exit if there is an error
    if (sendto(sockfd, request.data(), request.size(), 0, reinterpret_cast<const sockaddr*>(&dest), sizeof(dest)) < 0) {
        perror("sendto");
        return "";
    }

    // Map that stores the sequence number to the corresponding payload chunk received from the destination. This is used to reconstruct the entire response payload in the correct order after receiving all chunks.
    std::map<int, std::string> chunks;
    int total = -1;

    // Keep receiving chunk packets until all chunks have been received, and then reassemble the payload in the correct order
    while (total == -1 || static_cast<int>(chunks.size()) < total) {
        // Buffer to store the received chunk packet. The size of the buffer is set to 2048 bytes, which should be sufficient for most chunk packets.
        char buf[2048];
        // Create a sockaddr_in structure to store the source address of the received packet. This structure will be filled in by the recvfrom call.
        sockaddr_in from{};
        // Variable to store the length of the source address structure. This is initialized to the size of the sockaddr_in structure and will be updated by the recvfrom call.
        socklen_t fromlen = sizeof(from);
        // Receive a chunk packet from the destination and store it in the buffer. The source address of the packet is stored in the "from" sockaddr_in structure, and the length of the source address is stored in "fromlen".
        ssize_t n = recvfrom(sockfd, buf, sizeof(buf), 0, reinterpret_cast<sockaddr*>(&from), &fromlen);

        // Catch any errors that occur during the recvfrom call. If the error is EINTR (interrupted system call), continue receiving. Otherwise, print an error message and break the loop.
        if (n < 0) {
            if (errno == EINTR) continue;
            perror("recvfrom");
            break;
        }
        
        // Convert the received buffer into a string for easier processing. The string is constructed from the buffer starting at the beginning and extending for n bytes, which is the number of bytes received.
        std::string packet(buf, buf + n);
        // Parse the chunk packet into its components: type, sequence number, total chunks, and payload
        std::string type, payload;
        int seq = 0, tot = 0;
        if (!parse_chunk_packet(packet, type, seq, tot, payload)) {
            continue;
        }
        // If the total number of chunks has not been set yet, set it to the total value from the first received chunk packet. This ensures that we know how many chunks to expect in total.
        if (total == -1) {
            total = tot;
        }

        chunks[seq] = payload;
    }

    // Reassemble the payload chunks in the ascending sequence order and return the complete response as a single string.
    std::string out;
    for (int i = 1; i <= total; ++i) {
        out += chunks[i];
    }

    return out;
}

// Takes a large payload and sends it in smaller chunks over UDP to the specified destination. Each chunk is sent with a sequence number and total number of chunks, allowing the receiver to reassemble the payload correctly.
inline void udp_send_all_chunks(int sockfd, const sockaddr_in &dest, const std::string &type, const std::string &payload) {
    // Divides payload into chunks of at most size 1100 bytes. The last chunk may be smaller if the payload size is not a multiple of 1100. The total number of chunks is calculated based on the payload size and the chunk size.
    const size_t CHUNK = 1100;
    // If the payload is empty, we still need to send one chunk with an empty payload. Otherwise, calculate the total number of chunks needed to send the entire payload.
    size_t total = payload.empty() ? 1 : ((payload.size() + CHUNK - 1) / CHUNK);

    // Loop through each chunk and send it to the destination. The sequence number starts at 1 and goes up to the total number of chunks. Each chunk is sent with a packet that includes the type, sequence number, total number of chunks, and the payload for that chunk.
    for (size_t i = 0; i < total; ++i) {
        std::string part;
        if (!payload.empty()) {
            // Determine start and last index of current chunk
            size_t start = i * CHUNK;
            size_t len = std::min(CHUNK, payload.size() - start);

            // Extract subpart of the payload.
            part = payload.substr(start, len);
        }
        // Send subpart of payload with sequence number and total number of chunks to the destination.
        sendto(sockfd, packet.data(), packet.size(), 0, reinterpret_cast<const sockaddr*>(&dest), sizeof(dest));
    }
}

#endif

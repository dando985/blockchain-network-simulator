# Distributed TxCoin Network

A C++ socket-programming project that implements a simplified distributed cryptocurrency transaction network using **TCP and UDP communication**.

The system consists of a **Main Server**, three **backend servers**, a **client**, and a **monitor**. Transaction records are distributed across the backend servers, while the Main Server coordinates balance inquiries, validates transfers, and reconstructs the complete transaction history.

## Architecture

```text
                         ┌─────────────────┐
                         │     Client      │
                         │                 │
                         │ Balance / TX    │
                         └────────┬────────┘
                                  │
                              TCP : 25407
                                  │
                                  ▼
                        ┌──────────────────┐
                        │   Main Server    │
                        │     serverM      │
                        │                  │
                        │ TCP : 25407      │
                        │ TCP : 26407      │
                        │ UDP : 24407      │
                        └────────┬─────────┘
                                 │
                                UDP
                    ┌────────────┼────────────┐
                    │            │            │
                    ▼            ▼            ▼
              ┌──────────┐ ┌──────────┐ ┌──────────┐
              │ Server A │ │ Server B │ │ Server C │
              │  21407   │ │  22407   │ │  23407   │
              └────┬─────┘ └────┬─────┘ └────┬─────┘
                   │            │            │
                   ▼            ▼            ▼
              block1.txt   block2.txt   block3.txt


                         ┌─────────────────┐
                         │     Monitor     │
                         └────────┬────────┘
                                  │
                              TCP : 26407
                                  │
                                  ▼
                              serverM
```

## Features

* TCP communication between clients and the Main Server
* TCP communication between the Monitor and the Main Server
* UDP communication between the Main Server and backend servers
* Distributed transaction storage across three backend servers
* Balance inquiries
* TxCoin transfers between users
* Transaction validation
* Automatic transaction serial-number generation
* Random backend selection for new transactions
* Simple Caesar-style encryption for stored transaction fields
* UDP response chunking and reassembly
* Generation of a sorted transaction ledger
* Support for multiple Main Server TCP services using `select()`

## Components

### Client

`client.cpp` provides the user-facing interface.

The client supports two operations:

#### Check a balance

```bash
./client <username>
```

Example:

```bash
./client Alice
```

The client sends a request in the form:

```text
CHECK|Alice
```

to the Main Server over TCP.

#### Transfer TxCoins

```bash
./client <sender> <receiver> <amount>
```

Example:

```bash
./client Alice Bob 50
```

The client sends:

```text
TX|Alice|Bob|50
```

to the Main Server.

The client does not directly communicate with any backend server.

---

### Main Server

`serverM.cpp` is the coordinator of the network.

It listens on:

| Purpose               | Protocol |    Port |
| --------------------- | -------- | ------: |
| Client requests       | TCP      | `25407` |
| Monitor requests      | TCP      | `26407` |
| Backend communication | UDP      | `24407` |

The Main Server is responsible for:

* receiving balance inquiries
* receiving transfer requests
* requesting transaction records from all backend servers
* reconstructing the distributed transaction history
* calculating balances
* validating transfers
* generating new transaction serial numbers
* randomly selecting a backend for new transactions
* encrypting new transaction fields before storage
* generating a sorted transaction ledger for the Monitor

The Main Server uses `select()` to monitor its client and monitor TCP listening sockets.

---

### Backend Servers

The network contains three backend servers:

| Backend  | UDP Port | Transaction File |
| -------- | -------: | ---------------- |
| Server A |  `21407` | `block1.txt`     |
| Server B |  `22407` | `block2.txt`     |
| Server C |  `23407` | `block3.txt`     |

Each backend server stores only a portion of the overall transaction history.

The Main Server communicates with the backends using UDP.

Backend servers support two primary requests.

#### `GET_ALL`

Requests all transaction records stored by the backend.

```text
GET_ALL
```

The backend reads its block file and sends the contents back to the Main Server.

#### `APPEND`

Adds a new encrypted transaction to the backend's block file.

```text
APPEND|<transaction>
```

After successfully storing the transaction, the backend returns an acknowledgement containing:

```text
OK
```

---

### Monitor

The Monitor communicates with the Main Server over TCP port `26407`.

It requests:

```text
TXLIST
```

The Main Server then:

1. requests records from Servers A, B, and C
2. combines the records
3. sorts them by transaction serial number
4. writes the resulting ledger to `txchain.txt`

---

## Transaction Format

A transaction is represented internally by:

```cpp
struct TxRecord {
    int serial;
    std::string sender;
    std::string receiver;
    std::string amount;
};
```

Conceptually, a transaction looks like:

```text
<serial> <sender> <receiver> <amount>
```

For example:

```text
15 Alice Bob 50
```

The serial number uniquely orders transactions in the network.

## Balance Calculation

Balances are reconstructed from transaction history rather than stored directly.

Each participant begins with an initial balance of:

```text
1000 TxCoins
```

For every transaction:

```text
sender   → amount is subtracted
receiver → amount is added
```

Conceptually:

```text
balance =
    1000
    - total coins sent
    + total coins received
```

Because transactions are distributed across all three backend servers, the Main Server must collect records from all three before calculating an accurate balance.

## Transaction Processing

For a request such as:

```bash
./client Alice Bob 50
```

the following sequence occurs:

```text
Client
   │
   │ TCP
   │ TX|Alice|Bob|50
   ▼
Main Server
   │
   ├──── UDP GET_ALL ────► Server A
   ├──── UDP GET_ALL ────► Server B
   └──── UDP GET_ALL ────► Server C
   │
   ▼
Combine transaction history
   │
   ▼
Validate sender, receiver,
amount, and sender balance
   │
   ├── Invalid ──► return error to client
   │
   └── Valid
         │
         ▼
Generate next serial number
         │
         ▼
Randomly select A, B, or C
         │
         ▼
Encrypt transaction fields
         │
         ▼
APPEND transaction via UDP
         │
         ▼
Backend block file
         │
         ▼
Return result to client
```

## Encryption

Transaction fields stored by the backend servers use a simple Caesar-style shift.

Letters are shifted by three positions:

```text
a → d
b → e
...
x → a
y → b
z → c
```

Uppercase letters are handled similarly.

Digits are also shifted by three:

```text
0 → 3
1 → 4
...
7 → 0
8 → 1
9 → 2
```

For example:

```text
Martin
```

becomes:

```text
Pduwlq
```

Encryption is performed before a new transaction is sent to a backend, while transaction records are decrypted when reconstructed for processing.

> This transformation is part of the project protocol and is not intended to provide real cryptographic security.

## UDP Message Handling

Because transaction data may be larger than a single application-level response chunk, the project implements a simple UDP chunking protocol.

Packets use the format:

```text
TYPE|SEQUENCE|TOTAL|PAYLOAD
```

For example:

```text
ALL|2|4|<payload>
```

indicates that the packet is chunk 2 of 4.

The receiver stores chunks by sequence number and reconstructs the complete payload in the correct order.

Backend responses use packet types such as:

```text
ALL
ACK
```

The current implementation primarily processes the reconstructed payload rather than using the response type for application-level decision making.

## TCP vs. UDP

The project intentionally demonstrates both major transport protocols.

### TCP

Used for:

```text
Client  ↔ Main Server
Monitor ↔ Main Server
```

TCP provides reliable, connection-oriented communication.

### UDP

Used for:

```text
Main Server ↔ Backend A
Main Server ↔ Backend B
Main Server ↔ Backend C
```

UDP provides connectionless datagram communication. The project implements its own chunk numbering and payload reconstruction for backend responses.

## Project Structure

```text
.
├── client.cpp
├── client.h
├── serverM.cpp
├── serverM.h
├── serverA.cpp
├── serverA.h
├── serverB.cpp
├── serverB.h
├── serverC.cpp
├── serverC.h
├── monitor.cpp
├── monitor.h
├── common.h
├── block1.txt
├── block2.txt
├── block3.txt
├── Makefile
└── README.md
```

### `common.h`

Contains shared functionality used throughout the project, including:

* transaction structures
* parsing utilities
* file utilities
* balance calculations
* transaction validation
* encryption/decryption
* TCP socket helpers
* UDP communication helpers
* UDP chunking and reconstruction

## Building

The project requires a C++ compiler and POSIX socket support.

On Linux/macOS, build the project using the provided `Makefile`:

```bash
make all
```

To remove compiled executables:

```bash
make clean
```

## Running

Start the servers before running clients or the monitor.

A typical startup sequence is:

```bash
./serverM
```

Then, in separate terminals:

```bash
./serverA
```

```bash
./serverB
```

```bash
./serverC
```

Once all servers are running, client requests can be made.

### Balance inquiry

```bash
./client Alice
```

### Transfer

```bash
./client Alice Bob 50
```

### Transaction list

Run the Monitor to request the sorted transaction ledger:

```bash
./monitor TXLIST
```

The resulting transaction history is written to:

```text
txchain.txt
```

## Network Configuration

All components communicate locally using:

```text
127.0.0.1
```

The configured ports are:

```text
Server A UDP       21407
Server B UDP       22407
Server C UDP       23407
Main Server UDP    24407
Client TCP         25407
Monitor TCP        26407
```

Make sure these ports are available before starting the programs.

## Key Concepts Demonstrated

This project demonstrates several systems and networking concepts:

* POSIX socket programming
* IPv4 networking
* TCP client/server communication
* UDP datagram communication
* socket binding
* dynamic client ports
* `send()` / `recv()`
* `sendto()` / `recvfrom()`
* `listen()` / `accept()`
* `select()` multiplexing
* network byte order
* application-layer protocol design
* distributed data storage
* file I/O
* transaction validation
* serialization and parsing
* UDP chunking and reassembly

## Notes

This project is an educational simulation of a distributed transaction network. The term "blockchain" refers to the distributed transaction-record architecture used by the assignment; the implementation does not provide the cryptographic consensus, hashing, proof mechanisms, or decentralized consensus protocols found in production blockchain systems.
